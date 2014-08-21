#!/usr/bin/env python

import lsst.log as log
import multiprocessing as mp

def main():
    # Set component to "main" (from "root")
    with log.LogContext("main"):
        log.info("In main")
        visits = [12345, 67890, 27182, 31415]
        pool = mp.Pool(processes=2)
        pool.map_async(a, visits)
        pool.close()
        pool.join()
        b()
        log.info("Leaving main")

def a(visit):
    # Set subcomponent to "a" (sets component to "main.a")
    with log.LogContext("a"):
        # Clean out any previous MDC for visit
        log.MDCRemove("visit")
        # All subsequent log messages will have visit id
        log.MDC("visit", visit)
        log.info("In a, %d", visit)
        log.debug("Debug message in a")
        b()
        log.info("Leaving a")

def b():
    # Set subcomponent to "b" (sets component to "main.a.b" or "main.b")
    with log.LogContext("b"):
        log.info("In b")
        log.debug("Testing; logged only when called by a")
        log.info("Leaving b")

if __name__ == "__main__":
    log.configure("examples/log4cxx.properties")
    main()
