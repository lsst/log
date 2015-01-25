// -*- LSST-C++ -*-
/*
 * LSST Data Management System
 * Copyright 2013-2014 LSST Corporation.
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

/**
 * @file logInterface.cc
 * @brief Logging module interface for exporting via SWIG to Python.
 *
 * @author Bill Chickering
 * Contact: chickering@cs.stanford.edu
 *
 */

// System headers
#include <stdarg.h>

// Local headers
#include "lsst/log/Log.h"
#include "lsst/log/logInterface.h"

namespace lsst {
namespace log {

void configure_iface() {
    Log::configure();
}

void configure_iface(std::string const& filename) {
    Log::configure(filename);
}

void configure_prop_iface(std::string const& properties) {
    Log::configure_prop(properties);
}

std::string getDefaultLoggerName_iface(void) {
    return Log::getDefaultLoggerName();
}

void pushContext_iface(std::string const& name) {
    Log::pushContext(name);
}

void popContext_iface() {
    Log::popContext();
}

void MDC_iface(std::string const& key, std::string const& value) {
    Log::MDC(key, value);
}

void MDCRemove_iface(std::string const& key) {
    Log::MDCRemove(key);
}

void setLevel_iface(std::string const& loggername, int level) {
    Log::setLevel(loggername, level);
}

int getLevel_iface(std::string const& loggername) {
    return Log::getLevel(loggername);
}

bool isEnabledFor_iface(std::string const& loggername, int level) {
    return Log::isEnabledFor(loggername, level);
}

void forcedLog_iface(std::string const& loggername, int level,
                     std::string const& filename,
                     std::string const& funcname, int lineno,
                     std::string const& msg) {
    Log::getLogger(loggername).forcedLog(
        log4cxx::Level::toLevel(level), msg.c_str(),
        log4cxx::spi::LocationInfo(filename.c_str(), funcname.c_str(), lineno)
    );
}

}}  // namespace lsst::log
