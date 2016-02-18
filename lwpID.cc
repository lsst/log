// -*- LSST-C++ -*-
/*
 * LSST Data Management System
 * Copyright 2016 LSST Corporation.
 *
 * This product includes software developed by the
 * LSST Project (http://www.lsst.org/).
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the LSST License Statement and
 * the GNU General Public License along with this program.  If not,
 * see <http://www.lsstcorp.org/LegalNotices/>.
 */

#if defined(__linux__)

// System Headers
#include <cstdint>
#include <sys/syscall.h>
#include <thread>
#include <unistd.h>

namespace lsst {
namespace log {

    thread_local uint64_t lwpID =
                          static_cast<uint64_t>(syscall(SYS_gettid));

}} // namespace lsst::qserv::util

#elif defined(__APPLE__)

// System Headers
#include <cstdint>
#include <pthread.h>
#include <thread>

namespace lsst {
namespace log {

    thread_local uint64_t lwpID =
           static_cast<uint64_t>(pthread_mach_thread_np(pthread_self()));

}} // namespace lsst::qserv::util

#else

// System Headers
#include <atomic>
#include <cstdint>
#include <thread>

namespace lsst {
namespace log {

   std::atomic_ullong threadNum(0);
   thread_local uint64_t lwpID = ++threadNum;

}} // namespace lsst::log

#endif

