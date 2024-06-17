/*
 * LSST Data Management System
 * Copyright 2008-2016  AURA/LSST.
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
 * see <https://www.lsstcorp.org/LegalNotices/>.
 */

#include "nanobind/nanobind.h"
#include "nanobind/stl/function.h"
#include "nanobind/stl/string.h"

#include "lsst/log/Log.h"

namespace nb = nanobind;

namespace lsst {
namespace log {

NB_MODULE(log, mod) {
    nb::class_<Log> cls(mod, "Log");

    /* Constructors */
    cls.def(nb::init<>());

    /* Members */
    cls.attr("TRACE") = nb::int_(5000);
    cls.attr("DEBUG") = nb::int_(10000);
    cls.attr("INFO") = nb::int_(20000);
    cls.attr("WARN") = nb::int_(30000);
    cls.attr("ERROR") = nb::int_(40000);
    cls.attr("FATAL") = nb::int_(50000);

    cls.def("isDebugEnabled", &Log::isDebugEnabled);
    cls.def("isErrorEnabled", &Log::isErrorEnabled);
    cls.def("isFatalEnabled", &Log::isFatalEnabled);
    cls.def("isInfoEnabled", &Log::isInfoEnabled);
    cls.def("isTraceEnabled", &Log::isTraceEnabled);
    cls.def("isWarnEnabled", &Log::isWarnEnabled);
    cls.def("getName", &Log::getName);
    cls.def("setLevel", &Log::setLevel);
    cls.def("getLevel", &Log::getLevel);
    cls.def("getEffectiveLevel", &Log::getEffectiveLevel);
    cls.def("isEnabledFor", &Log::isEnabledFor);
    cls.def("getChild", &Log::getChild);
    cls.def("logMsg", [](Log &log, int level, std::string const &filename, std::string const &funcname,
                         unsigned int lineno, std::string const &msg) {
        log.logMsg(log4cxx::Level::toLevel(level),
                   log4cxx::spi::LocationInfo(filename.c_str(), log4cxx::spi::LocationInfo::calcShortFileName(filename.c_str()), funcname.c_str(), lineno),
                   msg);
    });
    cls.def("lwpID", [](Log const& log) -> unsigned { return lsst::log::lwpID(); });

    cls.def_static("getDefaultLogger", Log::getDefaultLogger);
    cls.def_static("configure", (void (*)())Log::configure);
    cls.def_static("configure", (void (*)(std::string const&))Log::configure);
    cls.def_static("configure_prop", Log::configure_prop);
    cls.def_static("getLogger", (Log(*)(Log const&))Log::getLogger);
    cls.def_static("getLogger", (Log(*)(std::string const&))Log::getLogger);
    cls.def_static("MDC", Log::MDC);
    cls.def_static("MDCRemove", Log::MDCRemove);
    // DM-9708: work around for pybind11 is not needed for nanobind
    cls.def_static("MDCRegisterInit", &Log::MDCRegisterInit);
}

}  // log
}  // lsst
