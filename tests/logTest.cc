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
#include <stdexcept>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>
#include <vector>

// Third-party headers
#include <boost/format.hpp>

// Local headers
#include "lsst/log/Log.h"

#define BOOST_TEST_MODULE Log_1
#define BOOST_TEST_DYN_LINK
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wunused-variable"
#include "boost/test/unit_test.hpp"
#pragma clang diagnostic pop

/* Compute line numbers in expected debug log messages
 *
 * This enforces consistency of string containing
 * expected log messages in case this source file
 * is modified
 *
 */
#define LOGF_DEBUG_LINENO(message, args) \
    LOGF_DEBUG(message); \
    lineno_helper(__LINE__, args);

/* Compute line numbers in expected info log messages
 */
#define LOGF_INFO_LINENO(message, args) \
    LOGF_INFO(message); \
    lineno_helper(__LINE__, args);

#define MDC_PID_KEY "PID"

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

    std::string format_range(const std::string& format_string, const std::vector<std::string>& args)
    {
        boost::format f(format_string);
        for (std::vector<std::string>::const_iterator it = args.begin(); it != args.end(); ++it) {
            f % *it;
        }
        return f.str();
    }

    void check(const std::string& expected) {
        std::ifstream t(ofName.c_str());
        std::string received((std::istreambuf_iterator<char>(t)),
                             std::istreambuf_iterator<char>());
        BOOST_CHECK_EQUAL(expected, received);
    }

    void lineno_helper(const long& line, std::vector<std::string>& args) {
        // cast is to work around a compiler problem; remove when upgrading
        args.push_back(std::to_string(static_cast<long long>(line)));
    }

    void pid_log_helper(const std::string& msg, std::vector<std::string>& args) {

        std::stringstream ss;

        configure(LAYOUT_PATTERN);
        // cast is to work around a compiler problem; remove when upgrading
        LOG_MDC(MDC_PID_KEY, std::to_string(static_cast<long long>(getpid())));

        LOGF_INFO_LINENO(msg, args);

        LOG_MDC_REMOVE(MDC_PID_KEY);

        // Add args for building expected log message
        args.push_back(msg);
        ss << getpid();
        args.push_back(ss.str());
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

    std::string expected_msg =
          "INFO  root pattern test_method (tests/logTest.cc:%1%) tests/logTest.cc(%1%) - This is INFO - {}\n"
          "DEBUG root pattern test_method (tests/logTest.cc:%2%) tests/logTest.cc(%2%) - This is DEBUG - {}\n"
          "INFO  root pattern test_method (tests/logTest.cc:%3%) tests/logTest.cc(%3%) - This is INFO 2 - {{x,3}{y,foo}}\n"
          "DEBUG root pattern test_method (tests/logTest.cc:%4%) tests/logTest.cc(%4%) - This is DEBUG 2 - {{x,3}{y,foo}}\n"
          "INFO  component pattern test_method (tests/logTest.cc:%5%) tests/logTest.cc(%5%) - This is INFO 3 - {{x,3}{y,foo}}\n"
          "DEBUG component pattern test_method (tests/logTest.cc:%6%) tests/logTest.cc(%6%) - This is DEBUG 3 - {{x,3}{y,foo}}\n"
          "INFO  component pattern test_method (tests/logTest.cc:%7%) tests/logTest.cc(%7%) - This is INFO 4 - {{y,foo}}\n"
          "DEBUG component pattern test_method (tests/logTest.cc:%8%) tests/logTest.cc(%8%) - This is DEBUG 4 - {{y,foo}}\n"
          "INFO  root pattern test_method (tests/logTest.cc:%9%) tests/logTest.cc(%9%) - This is INFO 5 - {{y,foo}}\n"
          "DEBUG root pattern test_method (tests/logTest.cc:%10%) tests/logTest.cc(%10%) - This is DEBUG 5 - {{y,foo}}\n";
    std::vector<std::string> args;

    configure(LAYOUT_PATTERN);

    LOGF_TRACE("This is TRACE");
    LOGF_INFO_LINENO("This is INFO", args);
    LOGF_DEBUG_LINENO("This is DEBUG", args);

    LOG_MDC("x", "3");
    LOG_MDC("y", "foo");

    LOGF_TRACE("This is TRACE 2");
    LOGF_INFO_LINENO("This is INFO 2", args);
    LOGF_DEBUG_LINENO("This is DEBUG 2", args);
    LOG_MDC_REMOVE("z");

    {
        LOG_CTX context("component");
        LOGF_TRACE("This is TRACE 3");
        LOGF_INFO_LINENO("This is INFO 3", args);
        LOGF_DEBUG_LINENO("This is DEBUG 3", args);
        LOG_MDC_REMOVE("x");
        LOGF_TRACE("This is TRACE 4");
        LOGF_INFO_LINENO("This is INFO 4", args);
        LOGF_DEBUG_LINENO("This is DEBUG 4", args);
    }
    LOGF_TRACE("This is TRACE 5");
    LOGF_INFO_LINENO("This is INFO 5", args);
    LOGF_DEBUG_LINENO("This is DEBUG 5", args);

    LOG_MDC_REMOVE("y");

    expected_msg = format_range(expected_msg, args);

    check(expected_msg);

}

BOOST_FIXTURE_TEST_CASE(MDCPutPid, LogFixture) {

    std::string msg;
    std::string expected_msg = "INFO  root LogFixture pid_log_helper (tests/logTest.cc:%1%) "
                               "tests/logTest.cc(%1%) - %2% - "
                               "{{" MDC_PID_KEY ",%3%}}\n";
    std::vector<std::string> args;
    pid_t pid = fork();

    BOOST_CHECK_MESSAGE(pid >= 0, "fork() failed!");
    if (pid == 0) {
        msg = "This is INFO in child process";

        // create a log file dedicated to child process
        ofName = std::tmpnam(NULL);

        pid_log_helper(msg, args);
    }
    else if (pid > 0) {
        msg = "This is INFO in parent process";

        pid_log_helper(msg, args);

        int status;

        int w;
        do {
          w = wait(&status);
        } while (w == -1 && errno == EINTR);
        if (w == -1) {
            perror("waitpid");
            exit(EXIT_FAILURE);
        }
        BOOST_CHECK_EQUAL(0, status);


    }
    expected_msg = format_range(expected_msg, args);

    check(expected_msg);

    if (pid == 0) {
        unlink(ofName.c_str());
        exit(0);
    }

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

BOOST_FIXTURE_TEST_CASE(context_exc, LogFixture) {
    configure(LAYOUT_COMPONENT);

    // multi-level context will result in exception
    BOOST_CHECK_THROW(LOG_PUSHCTX("x.y"), std::invalid_argument);
}

BOOST_FIXTURE_TEST_CASE(dm_1186, LogFixture) {
    // Test for properly-behaved macro, see DM-1186. MAin point here is that
    // it must compile without error without curly braces after if/else.
    configure(LAYOUT_SIMPLE);

    bool dummy = true;
    if (dummy)
    	LOG_INFO("This is INFO");
    else
    	LOG_WARN("This is WARN");

    check("INFO - This is INFO\n");
}
