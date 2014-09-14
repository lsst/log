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
#include <unistd.h>

// Local headers
#include "lsst/log/Log.h"

#define BOOST_TEST_MODULE Log_1
#define BOOST_TEST_DYN_LINK
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wunused-variable"
#include "boost/test/unit_test.hpp"
#pragma clang diagnostic pop

struct LogFixture {
    std::string ofName;
    enum Layout_t { LAYOUT_SIMPLE, LAYOUT_PATTERN, LAYOUT_COMPONENT };

    LogFixture() {
        ofName = std::tmpnam(NULL);
    }

    ~LogFixture() {
        unlink(ofName.c_str());
    }

    void configure(Layout_t layout) {
        std::string config = "log4j.rootLogger=DEBUG, FA\n"
                "log4j.appender.FA=FileAppender\n" 
                "log4j.appender.FA.file=" + ofName + "\n";
        switch (layout) {
            case LAYOUT_SIMPLE:
                config += "log4j.appender.FA.layout=SimpleLayout\n";
                break;
            case LAYOUT_PATTERN:
                config += "log4j.appender.FA.layout=PatternLayout\n"
                        "log4j.appender.FA.layout.ConversionPattern=%-5p %c %C %M (%F:%L) %l - %m - %X%n\n";
                break;
            case LAYOUT_COMPONENT:
                config += "log4j.appender.FA.layout=PatternLayout\n"
                        "log4j.appender.FA.layout.ConversionPattern=%-5p %c - %m%n\n";
                break;
        }
        LOG_CONFIG_PROP(config);
    }

    void check(const std::string& expected) {
        std::ifstream t(ofName.c_str());
        std::string received((std::istreambuf_iterator<char>(t)),
                             std::istreambuf_iterator<char>());
        BOOST_CHECK_EQUAL(expected, received);
    }    
};


BOOST_FIXTURE_TEST_CASE(basic, LogFixture) {
    configure(LAYOUT_SIMPLE);
    LOGF_TRACE("This is TRACE");
    LOGF_INFO("This is INFO");
    LOGF_DEBUG("This is DEBUG");
    LOGF_WARN("This is WARN");
    LOGF_ERROR("This is ERROR");
    LOGF_FATAL("This is FATAL");
    LOGF_INFO("Format %1% %2% %3%" % 3 % 2.71828 % "foo c++");
    check("INFO - This is INFO\n"
          "DEBUG - This is DEBUG\n"
          "WARN - This is WARN\n"
          "ERROR - This is ERROR\n"
          "FATAL - This is FATAL\n"
          "INFO - Format 3 2.71828 foo c++\n");
}


BOOST_FIXTURE_TEST_CASE(context, LogFixture) {
    configure(LAYOUT_SIMPLE);
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
    check("INFO - This is INFO\n"
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


BOOST_FIXTURE_TEST_CASE(pattern, LogFixture) {
    configure(LAYOUT_PATTERN);

    LOGF_TRACE("This is TRACE");
    LOGF_INFO("This is INFO");
    LOGF_DEBUG("This is DEBUG");

    LOG_MDC("x", "3");
    LOG_MDC("y", "foo");

    LOGF_TRACE("This is TRACE 2");
    LOGF_INFO("This is INFO 2");
    LOGF_DEBUG("This is DEBUG 2");
    LOG_MDC_REMOVE("z");

    {
        LOG_CTX context("component");
        LOGF_TRACE("This is TRACE 3");
        LOGF_INFO("This is INFO 3");
        LOGF_DEBUG("This is DEBUG 3");
        LOG_MDC_REMOVE("x");
        LOGF_TRACE("This is TRACE 4");
        LOGF_INFO("This is INFO 4");
        LOGF_DEBUG("This is DEBUG 4");
    }
    LOGF_TRACE("This is TRACE 5");
    LOGF_INFO("This is INFO 5");
    LOGF_DEBUG("This is DEBUG 5");

    LOG_MDC_REMOVE("y");

    check("INFO  root pattern test_method (tests/logTest.cc:159) tests/logTest.cc(159) - This is INFO - {}\n"
          "DEBUG root pattern test_method (tests/logTest.cc:160) tests/logTest.cc(160) - This is DEBUG - {}\n"
          "INFO  root pattern test_method (tests/logTest.cc:166) tests/logTest.cc(166) - This is INFO 2 - {{x,3}{y,foo}}\n"
          "DEBUG root pattern test_method (tests/logTest.cc:167) tests/logTest.cc(167) - This is DEBUG 2 - {{x,3}{y,foo}}\n"
          "INFO  component pattern test_method (tests/logTest.cc:173) tests/logTest.cc(173) - This is INFO 3 - {{x,3}{y,foo}}\n"
          "DEBUG component pattern test_method (tests/logTest.cc:174) tests/logTest.cc(174) - This is DEBUG 3 - {{x,3}{y,foo}}\n"
          "INFO  component pattern test_method (tests/logTest.cc:177) tests/logTest.cc(177) - This is INFO 4 - {{y,foo}}\n"
          "DEBUG component pattern test_method (tests/logTest.cc:178) tests/logTest.cc(178) - This is DEBUG 4 - {{y,foo}}\n"
          "INFO  root pattern test_method (tests/logTest.cc:181) tests/logTest.cc(181) - This is INFO 5 - {{y,foo}}\n"
          "DEBUG root pattern test_method (tests/logTest.cc:182) tests/logTest.cc(182) - This is DEBUG 5 - {{y,foo}}\n");
}

BOOST_FIXTURE_TEST_CASE(context1, LogFixture) {
    configure(LAYOUT_COMPONENT);

    LOGF_INFO("default logger name is '%1%'" % LOG_DEFAULT_NAME());
    LOG_PUSHCTX("component1");
    LOGF_INFO("default logger name is '%1%'" % LOG_DEFAULT_NAME());
    LOG_PUSHCTX("component2");
    LOGF_INFO("default logger name is '%1%'" % LOG_DEFAULT_NAME());
    LOG_POPCTX();
    LOGF_INFO("default logger name is '%1%'" % LOG_DEFAULT_NAME());
    LOG_POPCTX();

    {
        LOG_CTX context1("component3");
        LOGF_INFO("default logger name is '%1%'" % LOG_DEFAULT_NAME());
        {
            LOG_CTX context1("component4");
            LOGF_INFO("default logger name is '%1%'" % LOG_DEFAULT_NAME());
        }
        LOGF_INFO("default logger name is '%1%'" % LOG_DEFAULT_NAME());
    }
    LOGF_INFO("default logger name is '%1%'" % LOG_DEFAULT_NAME());

    // unmatched POP will leave us at root logger
    LOG_POPCTX();
    LOGF_INFO("default logger name is '%1%'" % LOG_DEFAULT_NAME());
    
    check("INFO  root - default logger name is ''\n"
          "INFO  component1 - default logger name is 'component1'\n"
          "INFO  component1.component2 - default logger name is 'component1.component2'\n"
          "INFO  component1 - default logger name is 'component1'\n"
          "INFO  component3 - default logger name is 'component3'\n"
          "INFO  component3.component4 - default logger name is 'component3.component4'\n"
          "INFO  component3 - default logger name is 'component3'\n"
          "INFO  root - default logger name is ''\n"
          "INFO  root - default logger name is ''\n");
}
