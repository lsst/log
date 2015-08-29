// -*- lsst-c++ -*-
%define log_DOCSTRING
"
Access to the classes from the log library
"
%enddef

%feature("autodoc", "1");
%module(package="lsst.log", docstring=log_DOCSTRING) logLib

%{
#include "lsst/log/logInterface.h"
%}

%include "lsst/p_lsstSwig.i"

%lsst_exceptions()
%include "lsst/pex/exceptions/exceptionsLib.i"

%include "lsst/log/logInterface.h"
