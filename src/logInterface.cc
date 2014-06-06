/*
 * LSST Data Management System
 * Copyright 2013 LSST Corporation.
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

#include "log/Log.h"
#include "log/logInterface.h"
#include <stdarg.h>

void lsst::log::configure_iface() {
    lsst::Log::configure();
}

void lsst::log::configure_iface(std::string const& filename) {
    lsst::Log::configure(filename);
}

std::string lsst::log::getDefaultLoggerName_iface(void) {
    return lsst::Log::getDefaultLoggerName();
}

void lsst::log::pushContext_iface(std::string const& name) {
    lsst::Log::pushContext(name);
}

void lsst::log::popContext_iface() {
    lsst::Log::popContext();
}

void lsst::log::MDC_iface(std::string const& key, std::string const& value) {
    lsst::Log::MDC(key, value);
}

void lsst::log::MDCRemove_iface(std::string const& key) {
    lsst::Log::MDCRemove(key);
}

void lsst::log::setLevel_iface(std::string const& loggername, int level) {
    lsst::Log::setLevel(loggername, level);
}

int lsst::log::getLevel_iface(std::string const& loggername) {
    return lsst::Log::getLevel(loggername);
}

bool lsst::log::isEnabledFor_iface(std::string const& loggername, int level) {
    return lsst::Log::isEnabledFor(loggername, level);
}

void lsst::log::forcedLog_iface(std::string const& loggername, int level,
                                std::string const& filename,
                                std::string const& funcname, int lineno,
                                std::string const& msg) {
    lsst::Log::getLogger(loggername)->forcedLog(
        log4cxx::Level::toLevel(level), msg.c_str(),
        log4cxx::spi::LocationInfo(filename.c_str(), funcname.c_str(), lineno)
    );
}


