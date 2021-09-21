//===- PerfData.cpp - Read Performance Data -------------------------------===//
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//

#include "meminstrument/optimizations/PerfData.h"

#include "meminstrument/Definitions.h"
#include "meminstrument/pass/Util.h"

#include "llvm/ADT/Statistic.h"
#include "llvm/ADT/StringRef.h"
#include "llvm/Support/CommandLine.h"
#include "llvm/Support/raw_ostream.h"

STATISTIC(FailingHotnessLookUps, "The # of failing hotness lookups");

using namespace meminstrument;
using namespace llvm;

namespace {

cl::opt<std::string>
    DBPathOpt("mi-profile-db-path",
              cl::desc("path to a meminstrument profile database"),
              cl::init("") // default
    );

} // namespace

#if HAS_SQLITE3

#include <sqlite3.h>

namespace {

struct QueryResult {
  uint64_t value;
  bool did_something;
};

static int callback(void *data, int numCols, char **fields, char **) {
  QueryResult *qr = (QueryResult *)data;
  assert(numCols == 1);
  StringRef num(fields[0]);
  num.getAsInteger(10, qr->value);
  qr->did_something = true;
  return 0;
}

void queryValue(sqlite3 *db, std::string &query, QueryResult &qr) {
  qr.value = 0;
  qr.did_something = false;

  char *zErrMsg = nullptr;

  int rc = 0;

  rc = sqlite3_exec(db, query.c_str(), callback, &qr, &zErrMsg);
  if (rc != SQLITE_OK) {
    errs() << "SQL error: " << zErrMsg << "\n";
    sqlite3_free(zErrMsg);
  }
}

} // namespace

namespace meminstrument {

uint64_t getHotnessIndex(StringRef ModuleName, StringRef FunctionName,
                         uint64_t AccessId) {
  static sqlite3 *db = nullptr;
  static bool failed = false;

  static bool found_mid = false;
  static uint64_t mid = 0;

  if (failed) {
    return 0;
  }

  int rc = 0;

  if (db == nullptr) {
    rc = sqlite3_open(DBPathOpt.c_str(), &db);
    if (rc) {
      ++FailingHotnessLookUps;
      errs() << "Can't open database: " << sqlite3_errmsg(db) << "\n";
      failed = true;
      return 0;
    }
  }

  if (!found_mid) {
    std::string stmt;
    raw_string_ostream ss(stmt);
    ss << "SELECT mid FROM modulenames"
       << "WHERE mname==\"" << ModuleName << "\" "
       << "LIMIT 1";

    QueryResult qr;
    queryValue(db, stmt, qr);

    if (!qr.did_something) {
      ++FailingHotnessLookUps;
      dbgs() << "[mi_perf] Failing Module lookup: " << stmt.c_str() << "\n";
      failed = true;
      return 0;
    }
    mid = qr.value;
    found_mid = true;
  }

  std::string stmt;
  raw_string_ostream ss(stmt);
  ss << "SELECT value FROM data "
     << "WHERE mid==\"" << mid << "\" "
     << "AND fname==\"" << FunctionName << "\" "
     << "AND aid==" << AccessId << " "
     << "LIMIT 1";

  QueryResult qr;
  queryValue(db, stmt, qr);

  if (!qr.did_something) {
    ++FailingHotnessLookUps;
    dbgs() << "[mi_perf] Failing lookup: " << stmt.c_str() << "\n";
  }

  return qr.value;
}

} // namespace meminstrument

#else

namespace meminstrument {

uint64_t getHotnessIndex(StringRef, StringRef, uint64_t) {
  ++FailingHotnessLookUps;

  static bool first_time = true;
  if (first_time) {
    dbgs() << "Trying to use performance data without sqlite3, this has no "
              "results.\n";
  }

  first_time = false;
  return 0;
}

} // namespace meminstrument
#endif
