#pragma once

#include <string>
#include <sstream>
#include <sqlite3.h>

#include "llvm/ADT/Statistic.h"
#include "llvm/Support/CommandLine.h"
#include "llvm/Support/raw_ostream.h"
#include "meminstrument/pass/Util.h"

STATISTIC(FailingHotnessLookUps, "The # of failing hotness lookups");

namespace {

struct QueryResult {
  uint64_t value;
  bool did_something;
};

static int callback(void *data, int numCols, char **fields, char **){
  QueryResult *qr = (QueryResult*)data;
  assert(numCols == 1);
  std::istringstream iss(fields[0]);
  iss >> qr->value;
  qr->did_something = true;
  return 0;
}

llvm::cl::opt<std::string>
    DBPathOpt("mi-profile-db-path",
                           llvm::cl::desc("path to a meminstrument profile database"),
                           llvm::cl::init("") // default
    );

uint64_t getHotnessIndex(const std::string &ModuleName, const std::string &FunctionName, uint64_t AccessId) {
  static sqlite3 *db = nullptr;
  static bool failed = false;

  if (failed) {
    return 0;
  }

  int rc = 0;

  if (db == nullptr) {
    rc = sqlite3_open(DBPathOpt.c_str(), &db);
    if (rc) {
        llvm::errs() << "Can't open database: " << sqlite3_errmsg(db) << "\n";
        failed = true;
        return 0;
    }
  }

  std::stringstream ss;
  ss << "SELECT value FROM data "
     << "WHERE mname==\"" << ModuleName << "\" "
     << "AND fname==\"" << FunctionName << "\" "
     << "AND aid==" << AccessId << " "
     << "LIMIT 1";
  auto stmt = ss.str();

  char *zErrMsg = nullptr;

  QueryResult qr;

  rc = sqlite3_exec(db, stmt.c_str(), callback, &qr, &zErrMsg);
  if (rc != SQLITE_OK) {
      llvm::errs() << "SQL error: " << zErrMsg << "\n";
      sqlite3_free(zErrMsg);
  }
  if (! qr.did_something) {
    ++FailingHotnessLookUps;
    llvm::dbgs() << "[mi_perf] Failing lookup: " << stmt.c_str() << "\n";
  }

  return qr.value;
}
}
