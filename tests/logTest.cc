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
#include <string.h>
#include <stdio.h>
#include <thread>
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

// used to stringify line numbers to make some names below unique
#define STR(X) XSTR(X)
#define XSTR(X) #X

/* Compute line numbers in expected debug log messages
 *
 * This enforces consistency of string containing
 * expected log messages in case this source file
 * is modified
 *
 */
#define LOGS_DEBUG_LINENO(message, args) \
    LOGS_DEBUG(message); \
    lineno_helper(__LINE__, args);

/* Compute line numbers in expected info log messages
 */
#define LOGS_INFO_LINENO(message, args) \
    LOGS_INFO(message); \
    lineno_helper(__LINE__, args);

#define MDC_PID_KEY "PID"

struct LogFixture {
    std::string ofName;
    int fd = -1;
    enum Layout_t { LAYOUT_SIMPLE, LAYOUT_PATTERN, LAYOUT_COMPONENT, LAYOUT_MDC };

    LogFixture() {
        ofName = _tmpnam();
    }

    ~LogFixture() {
        _unlink_tmp(ofName, fd);
    }

    std::string _tmpnam()
    {
        // PATH_MAX should be more than enough to hold the temp dir and file name
        char cname[PATH_MAX];
        strncpy(cname, P_tmpdir "/logTest-XXXXXXXXX", sizeof(cname));
        cname[sizeof(cname)-1] = '\0';  // Just in case
        fd = mkstemp(cname);
        if (fd == -1) {
          throw std::runtime_error("Failed to create temporary file.");
        }
        return std::string (cname);
    }

    void _unlink_tmp(std::string& name, int _fd)
    {
        if (_fd != -1) {
            unlink(name.c_str());
            close(_fd);
      }
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
            case LAYOUT_MDC:
                config += "log4j.appender.FA.layout=PatternLayout\n"
                        "log4j.appender.FA.layout.ConversionPattern=%-5p - %m %X%n\n";
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

        LOGS_INFO_LINENO(msg, args);

        LOG_MDC_REMOVE(MDC_PID_KEY);

        // Add args for building expected log message
        args.push_back(msg);
        ss << getpid();
        args.push_back(ss.str());
    }

};


BOOST_FIXTURE_TEST_CASE(basic_stream, LogFixture) {
    configure(LAYOUT_SIMPLE);
    LOGS_TRACE("This is TRACE");
    LOGS_INFO("This is INFO");
    LOGS_DEBUG("This is DEBUG");
    LOGS_WARN("This is WARN");
    LOGS_ERROR("This is ERROR");
    LOGS_FATAL("This is FATAL");
    LOGS_INFO("Format " << 3 << " " << 2.71828 << " foo c++");
    check("INFO - This is INFO\n"
          "DEBUG - This is DEBUG\n"
          "WARN - This is WARN\n"
          "ERROR - This is ERROR\n"
          "FATAL - This is FATAL\n"
          "INFO - Format 3 2.71828 foo c++\n");
}


BOOST_FIXTURE_TEST_CASE(context_stream, LogFixture) {
    configure(LAYOUT_COMPONENT);

    LOGS_TRACE("This is TRACE");
    LOGS_INFO("This is INFO");
    LOGS_DEBUG("This is DEBUG");
    {
        LOG_CTX context("componentX");
        LOGS_TRACE("This is TRACE 1");
        LOGS_INFO("This is INFO 1");
        LOGS_DEBUG("This is DEBUG 1");
    }
    LOGS_TRACE("This is TRACE 2");
    LOGS_INFO("This is INFO 2");
    LOGS_DEBUG("This is DEBUG 2");
    {
        LOG_CTX context("compY");
        LOGS_TRACE("This is TRACE 3");
        LOGS_INFO("This is INFO 3");
        LOGS_DEBUG("This is DEBUG 3");
        LOG_SET_LVL(LOG_DEFAULT_NAME(), LOG_LVL_INFO);
        BOOST_CHECK_EQUAL(LOG_GET_LVL(LOG_DEFAULT_NAME()),
                          LOG_LVL_INFO);
        LOGS_TRACE("This is TRACE 3a");
        LOGS_INFO("This is INFO 3a");
        LOGS_DEBUG("This is DEBUG 3a");
        {
            LOG_CTX context("subcompZ");
            LOG_SET_LVL(LOG_DEFAULT_NAME(), LOG_LVL_TRACE);
            BOOST_CHECK_EQUAL(LOG_GET_LVL(LOG_DEFAULT_NAME()),
                              LOG_LVL_TRACE);
            LOGS_TRACE("This is TRACE 4");
            LOGS_INFO("This is INFO 4");
            LOGS_DEBUG("This is DEBUG 4");
        }
        LOGS_TRACE("This is TRACE 5");
        LOGS_INFO("This is INFO 5");
        LOGS_DEBUG("This is DEBUG 5");
    }

    check("INFO  root - This is INFO\n"
          "DEBUG root - This is DEBUG\n"
          "INFO  componentX - This is INFO 1\n"
          "DEBUG componentX - This is DEBUG 1\n"
          "INFO  root - This is INFO 2\n"
          "DEBUG root - This is DEBUG 2\n"
          "INFO  compY - This is INFO 3\n"
          "DEBUG compY - This is DEBUG 3\n"
          "INFO  compY - This is INFO 3a\n"
          "TRACE compY.subcompZ - This is TRACE 4\n"
          "INFO  compY.subcompZ - This is INFO 4\n"
          "DEBUG compY.subcompZ - This is DEBUG 4\n"
          "INFO  compY - This is INFO 5\n");
}


BOOST_FIXTURE_TEST_CASE(pattern_stream, LogFixture) {

    std::string expected_msg =
          "INFO  root pattern_stream test_method (tests/logTest.cc:%1%) tests/logTest.cc(%1%) - This is INFO - {}\n"
          "DEBUG root pattern_stream test_method (tests/logTest.cc:%2%) tests/logTest.cc(%2%) - This is DEBUG - {}\n"
          "INFO  root pattern_stream test_method (tests/logTest.cc:%3%) tests/logTest.cc(%3%) - This is INFO 2 - {{x,3}{y,foo}}\n"
          "DEBUG root pattern_stream test_method (tests/logTest.cc:%4%) tests/logTest.cc(%4%) - This is DEBUG 2 - {{x,3}{y,foo}}\n"
          "INFO  component pattern_stream test_method (tests/logTest.cc:%5%) tests/logTest.cc(%5%) - This is INFO 3 - {{x,3}{y,foo}}\n"
          "DEBUG component pattern_stream test_method (tests/logTest.cc:%6%) tests/logTest.cc(%6%) - This is DEBUG 3 - {{x,3}{y,foo}}\n"
          "INFO  component pattern_stream test_method (tests/logTest.cc:%7%) tests/logTest.cc(%7%) - This is INFO 4 - {{y,foo}}\n"
          "DEBUG component pattern_stream test_method (tests/logTest.cc:%8%) tests/logTest.cc(%8%) - This is DEBUG 4 - {{y,foo}}\n"
          "INFO  root pattern_stream test_method (tests/logTest.cc:%9%) tests/logTest.cc(%9%) - This is INFO 5 - {{y,foo}}\n"
          "DEBUG root pattern_stream test_method (tests/logTest.cc:%10%) tests/logTest.cc(%10%) - This is DEBUG 5 - {{y,foo}}\n";
    std::vector<std::string> args;

    configure(LAYOUT_PATTERN);

    LOGS_TRACE("This is TRACE");
    LOGS_INFO_LINENO("This is INFO", args);
    LOGS_DEBUG_LINENO("This is DEBUG", args);

    LOG_MDC("x", "3");
    LOG_MDC("y", "foo");

    LOGS_TRACE("This is TRACE 2");
    LOGS_INFO_LINENO("This is INFO 2", args);
    LOGS_DEBUG_LINENO("This is DEBUG 2", args);
    LOG_MDC_REMOVE("z");

    {
        LOG_CTX context("component");
        LOGS_TRACE("This is TRACE 3");
        LOGS_INFO_LINENO("This is INFO 3", args);
        LOGS_DEBUG_LINENO("This is DEBUG 3", args);
        LOG_MDC_REMOVE("x");
        LOGS_TRACE("This is TRACE 4");
        LOGS_INFO_LINENO("This is INFO 4", args);
        LOGS_DEBUG_LINENO("This is DEBUG 4", args);
    }
    LOGS_TRACE("This is TRACE 5");
    LOGS_INFO_LINENO("This is INFO 5", args);
    LOGS_DEBUG_LINENO("This is DEBUG 5", args);

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
        ofName = _tmpnam();

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
        _unlink_tmp(ofName, fd);
        exit(0);
    }

}

BOOST_FIXTURE_TEST_CASE(context1_stream, LogFixture) {
    configure(LAYOUT_COMPONENT);

    LOGS_INFO("default logger name is " << LOG_DEFAULT_NAME());
    LOG_PUSHCTX("component1");
    LOGS_INFO("default logger name is " << LOG_DEFAULT_NAME());
    LOG_PUSHCTX("component2");
    LOGS_INFO("default logger name is " << LOG_DEFAULT_NAME());
    LOG_POPCTX();
    LOGS_INFO("default logger name is " << LOG_DEFAULT_NAME());
    LOG_POPCTX();

    {
        LOG_CTX context1("component3");
        LOGS_INFO("default logger name is " << LOG_DEFAULT_NAME());
        {
            LOG_CTX context1("component4");
            LOGS_INFO("default logger name is " << LOG_DEFAULT_NAME());
        }
        LOGS_INFO("default logger name is " << LOG_DEFAULT_NAME());
    }
    LOGS_INFO("default logger name is " << LOG_DEFAULT_NAME());

    // unmatched POP will leave us at root logger
    LOG_POPCTX();
    LOGS_INFO("default logger name is " << LOG_DEFAULT_NAME());

    check("INFO  root - default logger name is \n"
          "INFO  component1 - default logger name is component1\n"
          "INFO  component1.component2 - default logger name is component1.component2\n"
          "INFO  component1 - default logger name is component1\n"
          "INFO  component3 - default logger name is component3\n"
          "INFO  component3.component4 - default logger name is component3.component4\n"
          "INFO  component3 - default logger name is component3\n"
          "INFO  root - default logger name is \n"
          "INFO  root - default logger name is \n");
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

BOOST_FIXTURE_TEST_CASE(mdc_init, LogFixture) {

    std::string expected_msg =
          "INFO  - main thread {{MDC_INIT,OK}}\n"
          "INFO  - thread 1 {{MDC_INIT,OK}}\n"
          "INFO  - thread 2 {{MDC_INIT,OK}}\n";

    configure(LAYOUT_MDC);

    auto fun = [](){ LOG_MDC("MDC_INIT", "OK"); };
    LOG_MDC_INIT(fun);

    LOGS_INFO("main thread");

    std::thread thread1([]() { LOGS_INFO("thread 1"); });
    thread1.join();

    std::thread thread2([]() { LOGS_INFO("thread 2"); });
    thread2.join();

    check(expected_msg);

    LOG_MDC_REMOVE("MDC_INIT");
}

BOOST_AUTO_TEST_CASE(lwp_id) {

    auto lwp1 = lsst::log::lwpID();
    auto lwp2 = lsst::log::lwpID();
    auto pid = getpid();

    BOOST_CHECK_EQUAL(lwp1, lwp2);
    // LWP should be the same as PID in the main thread
    // or it can be a small number on platforms not supporting LWP
    BOOST_CHECK(lwp1 == pid or lwp1 < 10);
}
