// -*- lsst-c++ -*-
%define log_DOCSTRING
"
Access to the classes from the log library
"
%enddef

%feature("autodoc", "1");
%module(package="lsst.log", docstring=log_DOCSTRING) logLib

%naturalvar;
%include "std_string.i"

%{
#include "lsst/log/logInterface.h"
%}

%include "lsst/log/logInterface.h"
