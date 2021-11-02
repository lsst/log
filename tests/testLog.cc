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
        strncpy(cname, P_tmpdir "/testLog-XXXXXXXXX", sizeof(cname));
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

BOOST_AUTO_TEST_CASE(child_logger) {
    auto log1 = LOG_GET_CHILD("", "child1");
    BOOST_TEST(log1.getName() == "child1");
    auto log2 = LOG_GET_CHILD(log1, "child2");
    BOOST_TEST(log2.getName() == "child1.child2");
    auto log2a = LOG_GET_CHILD(log1, ".child2");
    BOOST_TEST(log2a.getName() == "child1.child2");
    auto log3 = LOG_GET_CHILD(log2, " .. child3");
    BOOST_TEST(log3.getName() == "child1.child2.child3");
    auto log3a = LOG_GET_CHILD(log1, "child2.child3");
    BOOST_TEST(log3a.getName() == "child1.child2.child3");
}

BOOST_FIXTURE_TEST_CASE(pattern_stream, LogFixture) {

    std::string expected_msg =
          "INFO  root pattern_stream test_method (%1%:%2%) %1%(%2%) - This is INFO - {}\n"
          "DEBUG root pattern_stream test_method (%1%:%3%) %1%(%3%) - This is DEBUG - {}\n"
          "INFO  root pattern_stream test_method (%1%:%4%) %1%(%4%) - This is INFO 2 - {{x,3}{y,foo}}\n"
          "DEBUG root pattern_stream test_method (%1%:%5%) %1%(%5%) - This is DEBUG 2 - {{x,3}{y,foo}}\n"
          "INFO  root pattern_stream test_method (%1%:%6%) %1%(%6%) - This is INFO 3 - {{x,3}{y,foo}}\n"
          "DEBUG root pattern_stream test_method (%1%:%7%) %1%(%7%) - This is DEBUG 3 - {{x,3}{y,foo}}\n"
          "INFO  root pattern_stream test_method (%1%:%8%) %1%(%8%) - This is INFO 4 - {{y,foo}}\n"
          "DEBUG root pattern_stream test_method (%1%:%9%) %1%(%9%) - This is DEBUG 4 - {{y,foo}}\n"
          "INFO  root pattern_stream test_method (%1%:%10%) %1%(%10%) - This is INFO 5 - {{y,foo}}\n"
          "DEBUG root pattern_stream test_method (%1%:%11%) %1%(%11%) - This is DEBUG 5 - {{y,foo}}\n"
          "INFO  root pattern_stream test_method (%1%:%12%) %1%(%12%) - This is INFO 6 - {{y,foo}{z,zzz}}\n"
          "INFO  root pattern_stream test_method (%1%:%13%) %1%(%13%) - This is INFO 7 - {{y,foo}}\n"
          "INFO  root pattern_stream test_method (%1%:%14%) %1%(%14%) - This is INFO 8 - {{q,qqq}{y,foo}}\n"
          "INFO  root pattern_stream test_method (%1%:%15%) %1%(%15%) - This is INFO 9 - {{y,foo}}\n";
    std::vector<std::string> args = { __FILE__ };

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

    {
        LOG_MDC_SCOPE("z", "zzz");
        LOGS_INFO_LINENO("This is INFO 6", args);
    }
    LOGS_INFO_LINENO("This is INFO 7", args);

    {
        // test move semantics
        auto func = []() { return lsst::log::LogMDCScope("q", "qqq"); };
        auto mdc_scope = func();
        LOGS_INFO_LINENO("This is INFO 8", args);
    }
    LOGS_INFO_LINENO("This is INFO 9", args);

    LOG_MDC_REMOVE("y");

    expected_msg = format_range(expected_msg, args);

    check(expected_msg);

}

BOOST_FIXTURE_TEST_CASE(MDCPutPid, LogFixture) {

    std::string msg;
    std::string expected_msg = "INFO  root LogFixture pid_log_helper (%1%:%2%) "
                               "%1%(%2%) - %3% - "
                               "{{" MDC_PID_KEY ",%4%}}\n";
    std::vector<std::string> args = { __FILE__ };
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

    unsigned lwp1 = lsst::log::lwpID();
    unsigned lwp2 = lsst::log::lwpID();

    BOOST_CHECK_EQUAL(lwp1, lwp2);
}

BOOST_FIXTURE_TEST_CASE(logger, LogFixture) {
    configure(LAYOUT_SIMPLE);
    std::string loggerName = "a";
    lsst::log::Log logger = lsst::log::Log::getLogger(loggerName);
    LOG_SET_LVL(loggerName, LOG_LVL_INFO);
    LOG(loggerName, LOG_LVL_INFO, "This is INFO 1");
    LOG(logger, LOG_LVL_INFO, "This is INFO 2");
    LOGS(loggerName, LOG_LVL_INFO, "This is INFO 3");
    LOGS(logger, LOG_LVL_INFO, "This is INFO 4");
    LOGL_TRACE(loggerName, "This is TRACE");
    LOGL_TRACE(logger, "This is TRACE");
    LOGL_INFO(loggerName, "This is INFO");
    LOGL_INFO(logger, "This is INFO");
    LOGL_DEBUG(loggerName, "This is DEBUG");
    LOGL_DEBUG(logger, "This is DEBUG");
    LOGL_WARN(loggerName, "This is WARN");
    LOGL_WARN(logger, "This is WARN");
    LOGL_ERROR(loggerName, "This is ERROR");
    LOGL_ERROR(logger, "This is ERROR");
    LOGL_FATAL(loggerName, "This is FATAL %d %.4f %s", 65, 42.123, "logging");
    LOGL_FATAL(logger, "This is FATAL %d %.4f %s", 65, 42.123, "logging");
    LOGLS_TRACE(loggerName, "This is TRACE");
    LOGLS_TRACE(logger, "This is TRACE");
    LOGLS_INFO(loggerName, "This is INFO");
    LOGLS_INFO(logger, "This is INFO");
    LOGLS_DEBUG(loggerName, "This is DEBUG");
    LOGLS_DEBUG(logger, "This is DEBUG");
    LOGLS_WARN(loggerName, "This is WARN and the logger name is " << logger.getName());
    LOGLS_WARN(logger, "This is WARN and the logger name is " << logger.getName());
    LOGLS_ERROR(loggerName, "This is ERROR");
    LOGLS_ERROR(logger, "This is ERROR");
    LOGLS_FATAL(loggerName, "This is FATAL " << 43 << " logging");
    LOGLS_FATAL(logger, "This is FATAL " << 43 << " logging");
    LOGLS_INFO(logger, "Format " << 3 << " " << 2.71828 << " foo c++");
    check("INFO - This is INFO 1\n"
          "INFO - This is INFO 2\n"
          "INFO - This is INFO 3\n"
          "INFO - This is INFO 4\n"
          "INFO - This is INFO\n"
          "INFO - This is INFO\n"
          "WARN - This is WARN\n"
          "WARN - This is WARN\n"
          "ERROR - This is ERROR\n"
          "ERROR - This is ERROR\n"
          "FATAL - This is FATAL 65 42.1230 logging\n"
          "FATAL - This is FATAL 65 42.1230 logging\n"
          "INFO - This is INFO\n"
          "INFO - This is INFO\n"
          "WARN - This is WARN and the logger name is a\n"
          "WARN - This is WARN and the logger name is a\n"
          "ERROR - This is ERROR\n"
          "ERROR - This is ERROR\n"
          "FATAL - This is FATAL 43 logging\n"
          "FATAL - This is FATAL 43 logging\n"
          "INFO - Format 3 2.71828 foo c++\n");
}
