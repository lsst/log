
# LSST Data Management System
# Copyright 2014-2017 LSST Corporation.
#
# This product includes software developed by the
# LSST Project (http://www.lsst.org/).
#
# This program is free software: you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation, either version 3 of the License, or
# (at your option) any later version.
#
# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.
#
# You should have received a copy of the LSST License Statement and
# the GNU General Public License along with this program.  If not,
# see <http://www.lsstcorp.org/LegalNotices/>.

"""
This tests the logging system in a variety of ways.
"""


import os
import shutil
import tempfile
import threading
import unittest
import logging
import lsst.log as log


class TestLog(unittest.TestCase):

    class StdoutCapture(object):
        """
        Context manager to redirect stdout to a file.
        """

        def __init__(self, filename):
            self.stdout = None
            self.outputFilename = filename

        def __enter__(self):
            self.stdout = os.dup(1)
            os.close(1)
            os.open(self.outputFilename, os.O_WRONLY | os.O_CREAT | os.O_TRUNC)

        def __exit__(self, type, value, traceback):
            if self.stdout is not None:
                os.close(1)
                os.dup(self.stdout)
                os.close(self.stdout)
                self.stdout = None

    def setUp(self):
        """Make a temporary directory and a log file in it."""
        self.tempDir = tempfile.mkdtemp()
        self.outputFilename = os.path.join(self.tempDir, "log.out")
        self.stdout = None

    def tearDown(self):
        """Remove the temporary directory and clean up Python forwarding."""
        log.doNotUsePythonLogging()
        shutil.rmtree(self.tempDir)

    def configure(self, configuration):
        """
        Create a configuration file in the temporary directory and populate
        it with the provided string.
        """
        log.configure_prop(configuration.format(self.outputFilename))

    def check(self, reference):
        """Compare the log file with the provided reference text."""
        with open(self.outputFilename, 'r') as f:
            # strip everything up to first ] to remove timestamp and thread ID
            lines = [line.split(']')[-1].rstrip("\n") for line in f.readlines()]
            reflines = [line for line in reference.split("\n") if line != ""]
            self.maxDiff = None
            self.assertListEqual(lines, reflines)

    def testDefaultLogger(self):
        """Check the default root logger name."""
        self.assertEqual(log.getDefaultLogger().getName(), "")

    def testBasic(self):
        """
        Test basic log output with default configuration.
        Since the default threshold is INFO, the DEBUG or TRACE
        message is not emitted.
        """
        with TestLog.StdoutCapture(self.outputFilename):
            log.configure()
            log.log(log.getDefaultLogger(), log.INFO, "This is INFO")
            log.info(u"This is unicode INFO")
            log.trace("This is TRACE")
            log.debug("This is DEBUG")
            log.warn("This is WARN")
            log.error("This is ERROR")
            log.fatal("This is FATAL")
            log.warning("Format %d %g %s", 3, 2.71828, "foo")
        self.check("""
root INFO: This is INFO
root INFO: This is unicode INFO
root WARN: This is WARN
root ERROR: This is ERROR
root FATAL: This is FATAL
root WARN: Format 3 2.71828 foo
""")

    def testBasicFormat(self):
        """
        Test basic log output with default configuration but using
        the f variants.
        Since the default threshold is INFO, the DEBUG or TRACE
        message is not emitted.
        """
        with TestLog.StdoutCapture(self.outputFilename):
            log.configure()
            log.logf(log.getDefaultLogger(), log.INFO,
                     "This is {{INFO}} Item 1: {item[1]}",
                     item=["a", "b", "c"])
            log.infof(u"This is {unicode} INFO")
            log.tracef("This is TRACE")
            log.debugf("This is DEBUG")
            log.warnf("This is WARN {city}", city="Tucson")
            log.errorf("This is ERROR {1}->{0}", 2, 1)
            log.fatalf("This is FATAL {1} out of {0} times for {place}",
                       4, 3, place="LSST")
            log.warnf("Format {} {} {}", 3, 2.71828, "foo")
        self.check("""
root INFO: This is {INFO} Item 1: b
root INFO: This is {unicode} INFO
root WARN: This is WARN Tucson
root ERROR: This is ERROR 1->2
root FATAL: This is FATAL 3 out of 4 times for LSST
root WARN: Format 3 2.71828 foo
""")

    def testPattern(self):
        """
        Test a complex pattern for log messages, including Mapped
        Diagnostic Context (MDC).
        """
        with TestLog.StdoutCapture(self.outputFilename):
            self.configure("""
log4j.rootLogger=DEBUG, CA
log4j.appender.CA=ConsoleAppender
log4j.appender.CA.layout=PatternLayout
log4j.appender.CA.layout.ConversionPattern=%-5p %c %C %M (%F:%L) %l - %m - %X%n
""")
            log.trace("This is TRACE")
            log.info("This is INFO")
            log.debug("This is DEBUG")

            log.MDC("x", 3)
            log.MDC("y", "foo")
            log.MDC("z", TestLog)

            log.trace("This is TRACE 2")
            log.info("This is INFO 2")
            log.debug("This is DEBUG 2")
            log.MDCRemove("z")

            log.trace("This is TRACE 3")
            log.info("This is INFO 3")
            log.debug("This is DEBUG 3")
            log.MDCRemove("x")
            log.trace("This is TRACE 4")
            log.info("This is INFO 4")
            log.debug("This is DEBUG 4")

            log.trace("This is TRACE 5")
            log.info("This is INFO 5")
            log.debug("This is DEBUG 5")

            log.MDCRemove("y")

        # Use format to make line numbers easier to change.
        self.check("""
INFO  root  testPattern (test_log.py:{0[0]}) test_log.py({0[0]}) - This is INFO - {{}}
DEBUG root  testPattern (test_log.py:{0[1]}) test_log.py({0[1]}) - This is DEBUG - {{}}
INFO  root  testPattern (test_log.py:{0[2]}) test_log.py({0[2]}) - This is INFO 2 - {{{{x,3}}{{y,foo}}{{z,<class '{1}.TestLog'>}}}}
DEBUG root  testPattern (test_log.py:{0[3]}) test_log.py({0[3]}) - This is DEBUG 2 - {{{{x,3}}{{y,foo}}{{z,<class '{1}.TestLog'>}}}}
INFO  root  testPattern (test_log.py:{0[4]}) test_log.py({0[4]}) - This is INFO 3 - {{{{x,3}}{{y,foo}}}}
DEBUG root  testPattern (test_log.py:{0[5]}) test_log.py({0[5]}) - This is DEBUG 3 - {{{{x,3}}{{y,foo}}}}
INFO  root  testPattern (test_log.py:{0[6]}) test_log.py({0[6]}) - This is INFO 4 - {{{{y,foo}}}}
DEBUG root  testPattern (test_log.py:{0[7]}) test_log.py({0[7]}) - This is DEBUG 4 - {{{{y,foo}}}}
INFO  root  testPattern (test_log.py:{0[8]}) test_log.py({0[8]}) - This is INFO 5 - {{{{y,foo}}}}
DEBUG root  testPattern (test_log.py:{0[9]}) test_log.py({0[9]}) - This is DEBUG 5 - {{{{y,foo}}}}
""".format([x + 157 for x in (0, 1, 8, 9, 13, 14, 17, 18, 21, 22)], __name__))  # noqa E501 line too long

    def testMDCPutPid(self):
        """
        Test add of PID Mapped Diagnostic Context (MDC).
        """
        pid = os.fork()
        try:

            log.MDC("PID", os.getpid())
            self.configure("""
log4j.rootLogger=DEBUG, CA
log4j.appender.CA=ConsoleAppender
log4j.appender.CA.layout=PatternLayout
log4j.appender.CA.layout.ConversionPattern=%-5p PID:%X{{PID}} %c %C %M (%F:%L) %l - %m%n
""")  # noqa E501 line too long
            self.assertGreaterEqual(pid, 0, "Failed to fork")

            msg = "This is INFO"
            if pid == 0:
                self.tempDir = tempfile.mkdtemp()
                self.outputFilename = os.path.join(self.tempDir,
                                                   "log-child.out")
                msg += " in child process"
            elif pid > 0:
                child_pid, child_status = os.wait()
                self.assertEqual(child_status, 0,
                                 "Child returns incorrect code")
                msg += " in parent process"

            with TestLog.StdoutCapture(self.outputFilename):
                log.info(msg)
                line = 226  # line number for previous line
        finally:
            log.MDCRemove("PID")

        # Use format to make line numbers easier to change.
        self.check("""
INFO  PID:{1} root  testMDCPutPid (test_log.py:{0}) test_log.py({0}) - {2}
""".format(line, os.getpid(), msg))

        # don't pass other tests in child process
        if pid == 0:
            os._exit(0)

    def testFileAppender(self):
        """Test configuring logging to go to a file."""
        self.configure("""
log4j.rootLogger=DEBUG, FA
log4j.appender.FA=FileAppender
log4j.appender.FA.file={0}
log4j.appender.FA.layout=SimpleLayout
""")
        log.MDC("x", 3)
        log.trace("This is TRACE")
        log.info("This is INFO")
        log.debug("This is DEBUG")
        log.MDCRemove("x")

        self.check("""
INFO - This is INFO
DEBUG - This is DEBUG
""")

    def testPythonLogging(self):
        """Test logging through the Python logging interface."""
        with TestLog.StdoutCapture(self.outputFilename):
            lgr = logging.getLogger()
            lgr.setLevel(logging.INFO)
            log.configure()
            with self.assertLogs(level="INFO") as cm:
                # Force the lsst.log handler to be applied as well as the
                # unittest log handler
                lgr.addHandler(log.LogHandler())
                lgr.info("This is INFO")
                lgr.debug("This is DEBUG")
                lgr.warning("This is %s", "WARNING")
                # message can be arbitrary Python object
                lgr.info(((1, 2), (3, 4)))
                lgr.info({1: 2})

            # Confirm that Python logging also worked
            self.assertEqual(len(cm.output), 4, f"Got output: {cm.output}")
            logging.shutdown()

        self.check("""
root INFO: This is INFO
root WARN: This is WARNING
root INFO: ((1, 2), (3, 4))
root INFO: {1: 2}
""")

    def testMdcInit(self):

        expected_msg = \
            "INFO  - main thread {{MDC_INIT,OK}}\n" + \
            "INFO  - thread 1 {{MDC_INIT,OK}}\n" + \
            "INFO  - thread 2 {{MDC_INIT,OK}}\n"

        with TestLog.StdoutCapture(self.outputFilename):

            self.configure("""
log4j.rootLogger=DEBUG, CA
log4j.appender.CA=ConsoleAppender
log4j.appender.CA.layout=PatternLayout
log4j.appender.CA.layout.ConversionPattern=%-5p - %m %X%n
""")

            def fun():
                log.MDC("MDC_INIT", "OK")
            log.MDCRegisterInit(fun)

            log.info("main thread")

            thread = threading.Thread(target=lambda: log.info("thread 1"))
            thread.start()
            thread.join()

            thread = threading.Thread(target=lambda: log.info("thread 2"))
            thread.start()
            thread.join()

        self.check(expected_msg)

        log.MDCRemove("MDC_INIT")

    def testMdcUpdate(self):
        """Test for overwriting MDC.
        """

        expected_msg = \
            "INFO  - Message one {}\n" \
            "INFO  - Message two {{LABEL,123456}}\n" \
            "INFO  - Message three {{LABEL,654321}}\n" \
            "INFO  - Message four {}\n"

        with TestLog.StdoutCapture(self.outputFilename):

            self.configure("""
log4j.rootLogger=DEBUG, CA
log4j.appender.CA=ConsoleAppender
log4j.appender.CA.layout=PatternLayout
log4j.appender.CA.layout.ConversionPattern=%-5p - %m %X%n
""")

            log.info("Message one")

            log.MDC("LABEL", "123456")
            log.info("Message two")

            log.MDC("LABEL", "654321")
            log.info("Message three")

            log.MDCRemove("LABEL")
            log.info("Message four")

        self.check(expected_msg)

    def testLwpID(self):
        """Test log.lwpID() method."""
        lwp1 = log.lwpID()
        lwp2 = log.lwpID()

        self.assertEqual(lwp1, lwp2)

    def testLogger(self):
        """
        Test log object.
        """
        with TestLog.StdoutCapture(self.outputFilename):
            log.configure()
            logger = log.Log.getLogger("b")
            self.assertEqual(logger.getName(), "b")
            logger.trace("This is TRACE")
            logger.info("This is INFO")
            logger.debug("This is DEBUG")
            logger.warn("This is WARN")
            logger.error("This is ERROR")
            logger.fatal("This is FATAL")
            logger.warn("Format %d %g %s", 3, 2.71828, "foo")
        self.check("""
b INFO: This is INFO
b WARN: This is WARN
b ERROR: This is ERROR
b FATAL: This is FATAL
b WARN: Format 3 2.71828 foo
""")

    def testLoggerLevel(self):
        """
        Test levels of Log objects
        """
        with TestLog.StdoutCapture(self.outputFilename):
            self.configure("""
log4j.rootLogger=TRACE, CA
log4j.appender.CA=ConsoleAppender
log4j.appender.CA.layout=PatternLayout
log4j.appender.CA.layout.ConversionPattern=%-5p %c (%F)- %m%n
""")
            self.assertEqual(log.Log.getLevel(log.Log.getDefaultLogger()),
                             log.TRACE)
            logger = log.Log.getLogger("a.b")
            self.assertEqual(logger.getName(), "a.b")
            logger.trace("This is TRACE")
            logger.setLevel(log.INFO)
            self.assertEqual(logger.getLevel(), log.INFO)
            self.assertEqual(log.Log.getLevel(logger), log.INFO)
            logger.debug("This is DEBUG")
            logger.info("This is INFO")
            logger.fatal("Format %d %g %s", 3, 2.71828, "foo")

            logger = log.Log.getLogger("a.b.c")
            self.assertEqual(logger.getName(), "a.b.c")
            logger.trace("This is TRACE")
            logger.debug("This is DEBUG")
            logger.warn("This is WARN")
            logger.error("This is ERROR")
            logger.fatal("This is FATAL")
            logger.info("Format %d %g %s", 3, 2.71828, "foo")
        self.check("""
TRACE a.b (test_log.py)- This is TRACE
INFO  a.b (test_log.py)- This is INFO
FATAL a.b (test_log.py)- Format 3 2.71828 foo
WARN  a.b.c (test_log.py)- This is WARN
ERROR a.b.c (test_log.py)- This is ERROR
FATAL a.b.c (test_log.py)- This is FATAL
INFO  a.b.c (test_log.py)- Format 3 2.71828 foo
""")

    def testMsgWithPercentS(self):
        """Test logging messages containing %s (DM-7509)
        """
        with TestLog.StdoutCapture(self.outputFilename):
            log.configure()
            logger = log.Log()
            logger.info("INFO with %s")
            logger.trace("TRACE with %s")
            logger.debug("DEBUG with %s")
            logger.warn("WARN with %s")
            logger.error("ERROR with %s")
            logger.fatal("FATAL with %s")
            logger.logMsg(log.DEBUG, "foo", "bar", 5, "DEBUG with %s")
        self.check("""
root INFO: INFO with %s
root WARN: WARN with %s
root ERROR: ERROR with %s
root FATAL: FATAL with %s
root DEBUG: DEBUG with %s
""")

    def testForwardToPython(self):
        """Test that `lsst.log` log messages can be forwarded to `logging`."""
        log.configure()

        # Without forwarding we only get python logger messages captured
        with self.assertLogs(level="WARNING") as cm:
            log.warn("lsst.log warning message that will not be forwarded to Python")
            logging.warning("Python logging message that will be captured")
        self.assertEqual(len(cm.output), 1)

        log.usePythonLogging()

        # With forwarding we get 2 logging messages captured
        with self.assertLogs(level="WARNING") as cm:
            log.warn("This is a warning from lsst log meant for python logging")
            logging.warning("Python warning log message to be captured")
        self.assertEqual(len(cm.output), 2)

        loggername = "newlogger"
        log2 = log.Log.getLogger(loggername)
        with self.assertLogs(level="INFO", logger=loggername):
            log2.info("Info message to non-root lsst logger")

        # Check that debug and info are working properly
        # This test should return a single log message
        with self.assertLogs(level="INFO", logger=loggername) as cm:
            log2.info("Second INFO message to non-root lsst logger")
            log.debug("Debug message to root lsst logger")

        self.assertEqual(len(cm.output), 1, f"Got output: {cm.output}")

        logging.shutdown()

    def testLogLoop(self):
        """Test that Python log forwarding works even if Python logging has
        been forwarded to lsst.log"""

        log.configure()

        # Note that assertLogs causes a specialists Python logging handler
        # to be added.

        # Set up some Python loggers
        loggername = "testLogLoop"
        lgr = logging.getLogger(loggername)
        lgr.setLevel(logging.INFO)
        rootlgr = logging.getLogger()
        rootlgr.setLevel(logging.INFO)

        # Declare that we are using the Python logger and that this will
        # not cause a log loop if we also are forwarding Python logging to
        # lsst.log
        log.usePythonLogging()

        # Ensure that we can log both in lsst.log and Python
        rootlgr.addHandler(log.LogHandler())

        # All three of these messages go through LogHandler
        # The first two because they have the handler added explicitly, the
        # the final one because the lsst.log logger is forwarded to the
        # ROOT Python logger which has the LogHandler registered.

        with open(self.outputFilename, "w") as fd:
            # Adding a StreamHandler will cause the LogHandler to no-op
            streamHandler = logging.StreamHandler(stream=fd)
            rootlgr.addHandler(streamHandler)

            # Do not use assertLogs since that messes with handlers
            lgr.info("INFO message: Python child logger, lsst.log.LogHandler + PythonLogging")
            rootlgr.info("INFO message: Python root logger, lsst.log.logHandler + PythonLogging")

            # This will use a ROOT python logger which has a LogHandler attached
            log.info("INFO message: lsst.log root logger, PythonLogging")

            rootlgr.removeHandler(streamHandler)

        self.check("""
INFO message: Python child logger, lsst.log.LogHandler + PythonLogging
INFO message: Python root logger, lsst.log.logHandler + PythonLogging
INFO message: lsst.log root logger, PythonLogging""")

        with open(self.outputFilename, "w") as fd:
            # Adding a StreamHandler will cause the LogHandler to no-op
            streamHandler = logging.StreamHandler(stream=fd)
            rootlgr.addHandler(streamHandler)

            # Do not use assertLogs since that messes with handlers
            lgr.info("INFO message: Python child logger, lsst.log.LogHandler + PythonLogging")
            rootlgr.info("INFO message: Python root logger, lsst.log.logHandler + PythonLogging")

            # This will use a ROOT python logger which has a LogHandler attached
            log.info("INFO message: lsst.log root logger, PythonLogging")

            rootlgr.removeHandler(streamHandler)

        self.check("""
INFO message: Python child logger, lsst.log.LogHandler + PythonLogging
INFO message: Python root logger, lsst.log.logHandler + PythonLogging
INFO message: lsst.log root logger, PythonLogging""")

        with self.assertLogs(level="INFO") as cm:
            rootlgr.info("Python log message forward to lsst.log")
            log.info("lsst.log message forwarded to Python")

        self.assertEqual(len(cm.output), 2, f"Got output: {cm.output}")

        logging.shutdown()

    def testForwardToPythonContextManager(self):
        """Test that `lsst.log` log messages can be forwarded to `logging`
        using context manager"""
        log.configure()

        # Without forwarding we only get python logger messages captured
        with self.assertLogs(level="WARNING") as cm:
            log.warning("lsst.log: not forwarded")
            logging.warning("Python logging: captured")
        self.assertEqual(len(cm.output), 1)

        # Temporarily turn on forwarding
        with log.UsePythonLogging():
            with self.assertLogs(level="WARNING") as cm:
                log.warn("lsst.log: forwarded")
                logging.warning("Python logging: also captured")
            self.assertEqual(len(cm.output), 2)

        # Verify that forwarding is disabled
        self.assertFalse(log.Log.UsePythonLogging)

    def testForwardToPythonAppender(self):
        """Test that `log4cxx` appender forwards it all to logging"""
        self.configure("""
log4j.rootLogger=DEBUG, PyLog
log4j.appender.PyLog = org.apache.log4j.PyLogAppender
""")
        with self.assertLogs(level="WARNING") as cm:
            log.warn("lsst.log: forwarded")
            logging.warning("Python logging: also captured")
        self.assertEqual(len(cm.output), 2)

        # check that MDC is stored in LogRecord
        log.MDC("LABEL", "some.task")
        with self.assertLogs(level="WARNING") as cm:
            log.warn("lsst.log: forwarded")
        log.MDCRemove("LABEL")
        self.assertEqual(len(cm.records), 1)
        print(f"{cm.records[0]=}")
        self.assertEqual(cm.records[0].MDC, {"LABEL": "some.task"})
        self.assertEqual(cm.records[0].msg, "lsst.log: forwarded")

    def testForwardToPythonAppenderWithMDC(self):
        """Test that `log4cxx` appender forwards it all to logging and modifies
        message with MDC info"""
        self.configure("""
log4j.rootLogger=DEBUG, PyLog
log4j.appender.PyLog = org.apache.log4j.PyLogAppender
log4j.appender.PyLog.MessagePattern = %m (LABEL=%X{{LABEL}})
""")
        log.MDC("LABEL", "some.task")
        with self.assertLogs(level="WARNING") as cm:
            log.warn("lsst.log: forwarded")
        log.MDCRemove("LABEL")
        self.assertEqual(len(cm.records), 1)
        self.assertEqual(cm.records[0].MDC, {"LABEL": "some.task"})
        self.assertEqual(cm.records[0].msg, "lsst.log: forwarded (LABEL=some.task)")

    def testForwardToPythonAppenderFormatMDC(self):
        """Test that we can format `log4cxx` MDC on Python side"""

        # remember old record factory
        old_factory = logging.getLogRecordFactory()

        # configure things using convenience method
        log.configure_pylog_MDC("INFO")

        with self.assertLogs(level="WARNING") as cm:
            log.warn("lsst.log: forwarded 1")
            log.MDC("LABEL", "task1")
            log.warn("lsst.log: forwarded 2")
            log.MDC("LABEL-X", "task2")
            log.warn("lsst.log: forwarded 3")
            logging.warning("Python logging: also captured")
            log.MDCRemove("LABEL")
            log.MDCRemove("LABEL-X")
        self.assertEqual(len(cm.records), 4)

        # restore factory
        logging.setLogRecordFactory(old_factory)

        # %-style formatting, only works on whole MDC
        formatter = logging.Formatter(fmt="%(levelname)s:%(name)s:%(message)s", style="%")
        self.assertEqual(formatter.format(cm.records[0]), "WARNING:root:lsst.log: forwarded 1")
        formatter = logging.Formatter(fmt="%(levelname)s:%(name)s:%(message)s:%(MDC)s", style="%")
        self.assertEqual(formatter.format(cm.records[0]), "WARNING:root:lsst.log: forwarded 1:{}")
        self.assertEqual(formatter.format(cm.records[1]),
                         "WARNING:root:lsst.log: forwarded 2:{LABEL=task1}")
        self.assertEqual(formatter.format(cm.records[2]),
                         "WARNING:root:lsst.log: forwarded 3:{LABEL=task1, LABEL-X=task2}")
        self.assertEqual(formatter.format(cm.records[3]), "WARNING:root:Python logging: also captured:{}")

        # format-style formatting, without MDC first
        formatter = logging.Formatter(fmt="{levelname}:{name}:{message}", style="{")
        self.assertEqual(formatter.format(cm.records[0]), "WARNING:root:lsst.log: forwarded 1")

        # format-style formatting, with full MDC
        formatter = logging.Formatter(fmt="{levelname}:{name}:{message}:{MDC}", style="{")
        self.assertEqual(formatter.format(cm.records[0]), "WARNING:root:lsst.log: forwarded 1:{}")
        self.assertEqual(formatter.format(cm.records[1]),
                         "WARNING:root:lsst.log: forwarded 2:{LABEL=task1}")
        self.assertEqual(formatter.format(cm.records[2]),
                         "WARNING:root:lsst.log: forwarded 3:{LABEL=task1, LABEL-X=task2}")
        self.assertEqual(formatter.format(cm.records[3]), "WARNING:root:Python logging: also captured:{}")

        # format-style, using index access to MDC items, works for almost any
        # item names
        formatter = logging.Formatter(fmt="{levelname}:{name}:{message}:{MDC[LABEL-X]}", style="{")
        self.assertEqual(formatter.format(cm.records[0]), "WARNING:root:lsst.log: forwarded 1:")
        self.assertEqual(formatter.format(cm.records[1]), "WARNING:root:lsst.log: forwarded 2:")
        self.assertEqual(formatter.format(cm.records[2]),
                         "WARNING:root:lsst.log: forwarded 3:task2")
        self.assertEqual(formatter.format(cm.records[3]), "WARNING:root:Python logging: also captured:")

    def testLevelTranslator(self):
        """Test LevelTranslator class
        """
        # correspondence between levels, logging has no TRACE but we accept
        # small integer in its place
        levelMap = ((log.TRACE, 5),
                    (log.DEBUG, logging.DEBUG),
                    (log.INFO, logging.INFO),
                    (log.WARN, logging.WARNING),
                    (log.ERROR, logging.ERROR),
                    (log.FATAL, logging.FATAL))
        for logLevel, loggingLevel in levelMap:
            self.assertEqual(log.LevelTranslator.lsstLog2logging(logLevel), loggingLevel)
            self.assertEqual(log.LevelTranslator.logging2lsstLog(loggingLevel), logLevel)

    def testChildLogger(self):
        """Check the getChild logger method."""
        logger = log.getDefaultLogger()
        self.assertEqual(logger.getName(), "")
        logger1 = logger.getChild("child1")
        self.assertEqual(logger1.getName(), "child1")
        logger2 = logger1.getChild("child2")
        self.assertEqual(logger2.getName(), "child1.child2")
        logger2a = logger1.getChild(".child2")
        self.assertEqual(logger2a.getName(), "child1.child2")
        logger3 = logger2.getChild(" .. child3")
        self.assertEqual(logger3.getName(), "child1.child2.child3")
        logger3a = logger1.getChild("child2.child3")
        self.assertEqual(logger3a.getName(), "child1.child2.child3")


if __name__ == "__main__":
    unittest.main()
