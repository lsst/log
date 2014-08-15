/* 
 * LSST Data Management System
 * Copyright 2014 LSST Corporation.
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
 
// System headers
#include <cstdio>
#include <fstream>
#include <iostream>
#include <string>

// Local headers
#include "lsst/log/Log.h"

#define BOOST_TEST_MODULE Log_1
#define BOOST_TEST_DYN_LINK
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wunused-variable"
#include "boost/test/unit_test.hpp"
#pragma clang diagnostic pop

BOOST_AUTO_TEST_SUITE(LogSuite)

struct LogFixture {
    LogFixture() {
        std::string cfName = std::tmpnam(NULL);
        ofName = std::tmpnam(NULL);
        std::ofstream f;
        f.open(cfName.c_str());
        f << "log4j.rootLogger=DEBUG, FA\n"
          << "log4j.appender.FA=FileAppender\n" 
          << "log4j.appender.FA.file=" << ofName << "\n"
          << "log4j.appender.FA.layout=SimpleLayout\n";
        f.close();
        LOG_CONFIG(cfName);
    }

    void check(std::string expected) {
        std::ifstream t(ofName.c_str());
        std::string received((std::istreambuf_iterator<char>(t)),
                             std::istreambuf_iterator<char>());
        BOOST_CHECK_EQUAL(expected, received);
    }    
    std::string ofName;
};

BOOST_AUTO_TEST_CASE(basic) {
    LogFixture f;
    LOGF_TRACE("This is TRACE");
    LOGF_INFO("This is INFO");
    LOGF_DEBUG("This is DEBUG");
    LOGF_WARN("This is WARN");
    LOGF_ERROR("This is ERROR");
    LOGF_FATAL("This is FATAL");
    LOGF_INFO("Format %1% %2% %3%" % 3 % 2.71828 % "foo c++");
    f.check("INFO - This is INFO\n"
            "DEBUG - This is DEBUG\n"
            "WARN - This is WARN\n"
            "ERROR - This is ERROR\n"
            "FATAL - This is FATAL\n"
            "INFO - Format 3 2.71828 foo c++\n");
}

BOOST_AUTO_TEST_CASE(context) {
    LogFixture f;
    LOGF_TRACE("This is TRACE");
    LOGF_INFO("This is INFO");
    LOGF_DEBUG("This is DEBUG");
    {
        LOG_CTX context("component");
        LOGF_TRACE("This is TRACE");
        LOGF_INFO("This is INFO");
        LOGF_DEBUG("This is DEBUG");
    }
    LOGF_TRACE("This is TRACE 2");
    LOGF_INFO("This is INFO 2");
    LOGF_DEBUG("This is DEBUG 2");
    {
        LOG_CTX context("comp");
        LOGF_TRACE("This is TRACE 3");
        LOGF_INFO("This is INFO 3");
        LOGF_DEBUG("This is DEBUG 3");
        LOG_SET_LVL(LOG_DEFAULT_NAME(), LOG_LVL_INFO);
        BOOST_CHECK_EQUAL(LOG_GET_LVL(LOG_DEFAULT_NAME()), 
                          LOG_LVL_INFO);
        LOGF_TRACE("This is TRACE 3a");
        LOGF_INFO("This is INFO 3a");
        LOGF_DEBUG("This is DEBUG 3a");
        {
            LOG_CTX context("subcomp");
            LOG_SET_LVL(LOG_DEFAULT_NAME(), LOG_LVL_TRACE);
            BOOST_CHECK_EQUAL(LOG_GET_LVL(LOG_DEFAULT_NAME()), 
                              LOG_LVL_TRACE);
            LOGF_TRACE("This is TRACE 4");
            LOGF_INFO("This is INFO 4");
            LOGF_DEBUG("This is DEBUG 4");
            LOGF_TRACE("This is TRACE 5");
            LOGF_INFO("This is INFO 5");
            LOGF_DEBUG("This is DEBUG 5");
        }
    }
    f.check("INFO - This is INFO\n"
            "DEBUG - This is DEBUG\n"
            "INFO - This is INFO\n"
            "DEBUG - This is DEBUG\n"
            "INFO - This is INFO 2\n"
            "DEBUG - This is DEBUG 2\n"
            "INFO - This is INFO 3\n"
            "DEBUG - This is DEBUG 3\n"
            "INFO - This is INFO 3a\n"
            "TRACE - This is TRACE 4\n"
            "INFO - This is INFO 4\n"
            "DEBUG - This is DEBUG 4\n"
            "TRACE - This is TRACE 5\n"
            "INFO - This is INFO 5\n"
            "DEBUG - This is DEBUG 5\n");
}

BOOST_AUTO_TEST_SUITE_END()
