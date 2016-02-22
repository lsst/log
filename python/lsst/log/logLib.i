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

namespace std {
  template <class Func>
  class function {
  };
}

%template(VoidFunc) std::function<void()>;

%{
// warpper for callable object to track lifetime
class callable_wrapper {
public:
    callable_wrapper(PyObject* callable) : _callable(callable) {}
    void operator()() { PyObject_CallObject(_callable, nullptr); }
private:
    swig::SwigPtr_PyObject _callable;
};
%}

// this should be for std::function<void()> but I can't convince SWIG
// to understand that syntax. Because we only have one type of function
// we can use more general std::function
%typemap(in) std::function {
    if (!PyCallable_Check($input)) {
        PyErr_SetString(PyExc_TypeError, "argument is not a callable");
        SWIG_fail;
    }
    $1 = std::function<void()>(callable_wrapper($input));
}

%include "lsst/log/logInterface.h"
