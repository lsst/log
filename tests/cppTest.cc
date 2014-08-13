/* 
 * LSST Data Management System
 * Copyright 2008, 2009, 2010 LSST Corporation.
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
 
#include "lsst/log/Log.h"

#define BOOST_TEST_MODULE Log_1
#define BOOST_TEST_DYN_LINK
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wunused-variable"
#include "boost/test/unit_test.hpp"
#pragma clang diagnostic pop

BOOST_AUTO_TEST_SUITE(LogSuite)

BOOST_AUTO_TEST_CASE(basic) {
    char const* subject = "important stuff";
    LOG("myLogger", LOG_LVL_INFO, "Here is some information about %s.", subject);
    LOGF("myLogger", LOG_LVL_INFO, "Here is more information about %s." % subject);
    LOG_DEBUG("My debugging statement.");

    LOG_LOGGER logger = LOG_GET("myLogger");
    LOG(logger, LOG_LVL_WARN, "Here is a warning.");

    // check here
}

BOOST_AUTO_TEST_CASE(context) {
}

BOOST_AUTO_TEST_SUITE_END()
