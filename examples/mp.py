#!/usr/bin/env python

import lsst.log as log
import multiprocessing as mp


def main():
    # Use logger "main"
    logger = log.getLogger("main")
    logger.info("In main")
    visits = [12345, 67890, 27182, 31415]
    pool = mp.Pool(processes=2)
    pool.map_async(a, [(visit, logger) for visit in visits])
    pool.close()
    pool.join()
    b(logger)
    log.info("Leaving main")


def a(args):
    visit, parentLogger = args
    # Set subcomponent to "a" (sets component to "main.a")
    logger = parentLogger.getChild("a")
    # Clean out any previous MDC for visit
    logger.MDCRemove("visit")
    # All subsequent log messages will have visit id
    logger.MDC("visit", str(visit))
    logger.info("In a, %d", visit)
    logger.debug("Debug message in a")
    b(logger)
    logger.info("Leaving a")


def b(parentLogger):
    # Set subcomponent to "b" (sets component to "main.a.b" or "main.b")
    logger = parentLogger.getChild("b")
    logger.info("In b")
    logger.debug("Testing; logged only when called by a")
    logger.info("Leaving b")


if __name__ == "__main__":
    log.configure("examples/log4cxx.properties")
    main()
