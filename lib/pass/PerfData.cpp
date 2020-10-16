//===---------------------------------------------------------------------===///
///
/// \file TODO doku
///
//===----------------------------------------------------------------------===//

#include "meminstrument/pass/PerfData.h"

#include "meminstrument/Definitions.h"

#include "llvm/ADT/Statistic.h"
#include "llvm/Support/raw_ostream.h"

#include "meminstrument/pass/Util.h"

STATISTIC(FailingHotnessLookUps, "The # of failing hotness lookups");

#include "llvm/Support/CommandLine.h"

namespace {

llvm::cl::opt<std::string>
    DBPathOpt("mi-profile-db-path",
              llvm::cl::desc("path to a meminstrument profile database"),
              llvm::cl::init("") // default
    );

} // namespace

#if HAS_SQLITE3

#include <sqlite3.h>
#include <sstream>

using namespace meminstrument;
using namespace llvm;

namespace {

struct QueryResult {
  uint64_t value;
  bool did_something;
};

static int callback(void *data, int numCols, char **fields, char **) {
  QueryResult *qr = (QueryResult *)data;
  assert(numCols == 1);
  std::istringstream iss(fields[0]);
  iss >> qr->value;
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

uint64_t getHotnessIndex(const std::string &ModuleName,
                         const std::string &FunctionName, uint64_t AccessId) {
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
    std::stringstream ss;
    ss << "SELECT mid FROM modulenames"
       << "WHERE mname==\"" << ModuleName << "\" "
       << "LIMIT 1";
    auto stmt = ss.str();

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

  std::stringstream ss;
  ss << "SELECT value FROM data "
     << "WHERE mid==\"" << mid << "\" "
     << "AND fname==\"" << FunctionName << "\" "
     << "AND aid==" << AccessId << " "
     << "LIMIT 1";
  auto stmt = ss.str();

  QueryResult qr;
  queryValue(db, stmt, qr);

  if (!qr.did_something) {
    ++FailingHotnessLookUps;
    dbgs() << "[mi_perf] Failing lookup: " << stmt.c_str() << "\n";
  }

  return qr.value;
}

#else

using namespace meminstrument;
using namespace llvm;

uint64_t getHotnessIndex(const std::string &, const std::string &, uint64_t) {
  ++FailingHotnessLookUps;

  static bool first_time = true;
  if (first_time) {
    dbgs() << "Trying to use performance data without sqlite3, this has no "
              "results.\n";
  }

  first_time = false;
  return 0;
}

#endif
