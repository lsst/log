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
This is a unittest for the Log class.

@author  Jacek Becla, SLAC
"""

import lsst.log as newlog
import os
import tempfile
import unittest

log4jTemplate = """
log4j.rootLogger=DEBUG, QLogger
log4j.appender.QLogger=org.apache.log4j.FileAppender
log4j.appender.QLogger.File=OUTPUTDIR/testLogFile.log
log4j.appender.QLogger.layout=org.apache.log4j.PatternLayout
log4j.appender.QLogger.layout.ConversionPattern=%d [%t] %-5p %c{2} %M (%F:%L) - %m%n
"""

class TestUtils(unittest.TestCase):

    @classmethod
    def setUp(self):
        self.dirPath = tempfile.mkdtemp()
        self.log4jFileName = "%s/log4cxx.log4j" % self.dirPath
        contents = log4jTemplate.replace("OUTPUTDIR", self.dirPath)
        log4jFile = open(self.log4jFileName, 'w')
        log4jFile.write(contents)
        log4jFile.close()
        newlog.configure(self.log4jFileName)

    @classmethod
    def tearDownClass(self):
        os.remove(self.log4jFileName)
        os.remove("%s/testLogFile.log" % self.dirPath)
        os.rmdir(self.dirPath)

    def test1(self):
        newlog.trace("This is TRACE")
        newlog.info("This is INFO")
        newlog.debug("This is DEBUG")

        hasTrace = False
        hasInfo = False
        hasDebug = False
        f = open("%s/testLogFile.log" % self.dirPath, 'r')
        for line in f.readlines():
            if "TRACE" in line: hasTrace = True
            if "INFO" in line: hasInfo = True
            if "DEBUG" in line: hasDebug = True
        f.close()

        self.assertFalse(hasTrace)
        self.assertTrue(hasInfo)
        self.assertTrue(hasDebug)


####################################################################################
def main():
    unittest.main()

if __name__ == "__main__":
    main()
