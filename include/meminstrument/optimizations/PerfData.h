//===- meminstrument/PerfData.h - Read Performance Data ---------*- C++ -*-===//
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//
///
/// \file
/// Interpret previously collected performance data.
///
//===----------------------------------------------------------------------===//

#ifndef MEMINSTRUMENT_OPTIMIZATION_PERFDATA_H
#define MEMINSTRUMENT_OPTIMIZATION_PERFDATA_H

#include <string>

namespace meminstrument {

/// Given a module name, a function name and an index of an access, retrieve
/// the number of executions of the access from a database specified via the
/// `-mi-profile-db-path` cli option.
/// It returns 0 if no data is available for the given inputs.
/// This should be generated from the results of the rt_stat instrumentation.
///
/// This requires an installation of sqlite3; if this is not available, it will
/// return 0 in any case.
uint64_t getHotnessIndex(const std::string &ModuleName,
                         const std::string &FunctionName, uint64_t AccessId);

} // namespace meminstrument

#endif
