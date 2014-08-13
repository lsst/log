#!/usr/bin/env python

# LSST Data Management System
# Copyright 2014 LSST Corporation.
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

import lsst.log as log
import os
import shutil
import sys
import tempfile
import unittest

class TestLog(unittest.TestCase):

    def setUp(self):
        self.tempDir = tempfile.mkdtemp()
        self.outputFilename = os.path.join(self.tempDir, "log.out")
        self.stdout = None

    def tearDown(self):
        shutil.rmtree(self.tempDir)

    def captureStdout(self):
        self.stdout = os.dup(1)
        os.close(1)
        os.open(self.outputFilename, os.O_WRONLY | os.O_CREAT | os.O_TRUNC)

    def configure(self, configuration):
        configFilename = os.path.join(self.tempDir, "log4cxx.properties")
        with open(configFilename, "w") as configFile:
            configFile.write(configuration.format(self.outputFilename))
        log.configure(configFilename)

    def check(self, reference):
        if self.stdout is not None:
            os.close(1)
            os.dup(self.stdout)
            os.close(self.stdout)
            self.stdout = None
        with open(self.outputFilename, 'r') as f:
            lines = [l.split(']')[-1] for l in f.readlines()]
            reflines = [l + "\n" for l in reference.split("\n") if l != ""]
            map(self.assertEqual, lines, reflines)


###############################################################################

    def testDefaultLogger(self):
        self.assertEqual(log.getDefaultLoggerName(), "")

    def testBasic(self):
        self.captureStdout()
        log.configure()
        log.trace("This is TRACE")
        log.info("This is INFO")
        log.debug("This is DEBUG")
        log.warn("This is WARN")
        log.error("This is ERROR")
        log.fatal("This is FATAL")
        log.info("Format %d %g %s", 3, 2.71828, "foo")
        self.check("""
 INFO root null - Initializing Logging System
 INFO root null - This is INFO
 DEBUG root null - This is DEBUG
 WARN root null - This is WARN
 ERROR root null - This is ERROR
 FATAL root null - This is FATAL
 INFO root null - Format 3 2.71828 foo
""")

    def testContext(self):
        self.captureStdout()
        log.configure()
        log.trace("This is TRACE")
        log.info("This is INFO")
        log.debug("This is DEBUG")
        with log.LogContext("component"):
            log.trace("This is TRACE")
            log.info("This is INFO")
            log.debug("This is DEBUG")
        log.trace("This is TRACE 2")
        log.info("This is INFO 2")
        log.debug("This is DEBUG 2")
        with log.LogContext("comp") as ctx:
            log.trace("This is TRACE 3")
            log.info("This is INFO 3")
            log.debug("This is DEBUG 3")
            ctx.setLevel(log.INFO)
            self.assertEqual(ctx.getLevel(), log.INFO)
            self.assert_(ctx.isEnabledFor(log.INFO))
            log.trace("This is TRACE 3a")
            log.info("This is INFO 3a")
            log.debug("This is DEBUG 3a")
            with log.LogContext("subcomp", log.TRACE) as ctx2:
                self.assertEqual(ctx2.getLevel(), log.TRACE)
                log.trace("This is TRACE 4")
                log.info("This is INFO 4")
                log.debug("This is DEBUG 4")
            log.trace("This is TRACE 5")
            log.info("This is INFO 5")
            log.debug("This is DEBUG 5")

        self.check("""
 INFO root null - Initializing Logging System
 INFO root null - This is INFO
 DEBUG root null - This is DEBUG
 INFO component null - This is INFO
 DEBUG component null - This is DEBUG
 INFO root null - This is INFO 2
 DEBUG root null - This is DEBUG 2
 INFO comp null - This is INFO 3
 DEBUG comp null - This is DEBUG 3
 INFO comp null - This is INFO 3a
 TRACE comp.subcomp null - This is TRACE 4
 INFO comp.subcomp null - This is INFO 4
 DEBUG comp.subcomp null - This is DEBUG 4
 INFO comp null - This is INFO 5
""")

    def testPattern(self):
        self.captureStdout()
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

        with log.LogContext("component"):
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

        self.check("""
INFO  root  testPattern (logTest.py:150) logTest.py(150) - This is INFO - {}
DEBUG root  testPattern (logTest.py:151) logTest.py(151) - This is DEBUG - {}
INFO  root  testPattern (logTest.py:158) logTest.py(158) - This is INFO 2 - {{x,3}{y,foo}{z,<class '__main__.TestLog'>}}
DEBUG root  testPattern (logTest.py:159) logTest.py(159) - This is DEBUG 2 - {{x,3}{y,foo}{z,<class '__main__.TestLog'>}}
INFO  component  testPattern (logTest.py:164) logTest.py(164) - This is INFO 3 - {{x,3}{y,foo}}
DEBUG component  testPattern (logTest.py:165) logTest.py(165) - This is DEBUG 3 - {{x,3}{y,foo}}
INFO  component  testPattern (logTest.py:168) logTest.py(168) - This is INFO 4 - {{y,foo}}
DEBUG component  testPattern (logTest.py:169) logTest.py(169) - This is DEBUG 4 - {{y,foo}}
INFO  root  testPattern (logTest.py:172) logTest.py(172) - This is INFO 5 - {{y,foo}}
DEBUG root  testPattern (logTest.py:173) logTest.py(173) - This is DEBUG 5 - {{y,foo}}
""")

    def testFileAppender(self):
        self.configure("""
log4j.rootLogger=DEBUG, FA
log4j.appender.FA=FileAppender
log4j.appender.FA.file={0}
log4j.appender.FA.layout=SimpleLayout
""")
        log.MDC("x", 3)
        with log.LogContext("component"):
            log.trace("This is TRACE")
            log.info("This is INFO")
            log.debug("This is DEBUG")
        log.MDCRemove("x")

        self.check("""
INFO - This is INFO
DEBUG - This is DEBUG
""")

    @unittest.skip("Handler not working yet")
    def testLogger(self):
        self.captureStdout()
        log.configure()
        import logging
        lgr = logging.getLogger()
        lgr.setLevel(logging.INFO)
        lgr.addHandler(log.LogHandler())
        lgr.warn("This is WARN")

        self.check("""
 INFO root null - Initializing Logging System
 INFO root null - This is INFO
""")


####################################################################################
def main():
    unittest.main()

if __name__ == "__main__":
    main()
