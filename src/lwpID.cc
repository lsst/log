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

// System Headers
#include <iostream>
#include <string>
#if defined(__linux__)
#include <sys/syscall.h>
#include <unistd.h>
#elif defined(__APPLE__)
#include <pthread.h>
#else
#include <atomic>
#endif

namespace lsst {
namespace log {
namespace detail {

unsigned lwpID() {

#if defined(__linux__)

    // On Linux have to do syscall
    auto lwp = syscall(SYS_gettid);

#elif defined(__APPLE__)

    // OSX has a special Pthreads function to find out PID
    auto lwp = pthread_mach_thread_np(pthread_self());

#else

    // On all other system just generate incremental number and call it LWP
    static std::atomic<unsigned> threadNum(0);
    thread_local static auto lwp = ++threadNum;

#endif

    return static_cast<unsigned>(lwp);
}

}}} // namespace lsst::log::detail
