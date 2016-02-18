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
#ifndef LSST_LOG_LWPID_H
#define LSST_LOG_LWPID_H

// System headers
#include <cstdint>
#include <thread>

namespace lsst {
namespace log {

//-----------------------------------------------------------------------------
//! @brief Provide access to the light weight process (LWP) identifier.
//!
//! This interface provides the LWP number in Linux and MacOS. It is a
//! monotonically increasing unique number for all other operating systems.
//! The variable defined below holds the LWP number. Use the define macro to
//! isolate your code from future changes in the LWP number interface.
//-----------------------------------------------------------------------------

extern thread_local uint64_t lwpID;

#define LWP_ID lsst::log::lwpID

}} // namespace lsst::log

#endif // LSST_LOG_LWPID_H

