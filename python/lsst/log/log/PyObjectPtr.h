/*
 * This file is part of log.
 *
 * Developed for the LSST Data Management System.
 * This product includes software developed by the LSST Project
 * (https://www.lsst.org).
 * See the COPYRIGHT file at the top-level directory of this distribution
 * for details of code ownership.
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
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
 */

#ifndef LSST_LOG_PYOBJECTPTR_H
#define LSST_LOG_PYOBJECTPTR_H

// Python header has to be first to avoid compilation warnings
#include "Python.h"

#include <cstddef>

namespace lsst::log::detail {

/**
 *  Smart pointer class for PyObject instances.
 */
class PyObjectPtr {
public:

    /**
     * Wraps a "borrowed" object reference, reference counter is incremented.
     */
    static PyObjectPtr from_borrowed(PyObject* object) {
        Py_XINCREF(object);
        return PyObjectPtr(object);
    }

    /**
     * Construct a pointer from a regular "new" object reference.
     */
    explicit PyObjectPtr(PyObject* object = nullptr) : m_object(object) {}

    // copy constructor, increments ref. counter
    PyObjectPtr(PyObjectPtr const& other) : m_object(other.m_object) {
        Py_XINCREF(m_object);
    }

    // move constructor, steals reference
    PyObjectPtr(PyObjectPtr&& other) : m_object(other.m_object) {
        other.m_object = nullptr;
    }

    // Decrement ref. counter
    ~PyObjectPtr() { Py_CLEAR(m_object); }

    PyObjectPtr& operator=(PyObjectPtr const& other) {
        Py_CLEAR(m_object);
        m_object = other.m_object;
        Py_XINCREF(m_object);
        return *this;
    }

    PyObjectPtr& operator=(PyObjectPtr&& other) {
        Py_CLEAR(m_object);
        m_object = other.m_object;
        other.m_object = nullptr;
        return *this;
    }

    PyObjectPtr& operator=(PyObject* object) {
        Py_CLEAR(m_object);
        m_object = object;
        Py_XINCREF(m_object);
        return *this;
    }

    // can be converted to regular pointer
    operator PyObject*() const { return m_object; }

    // release ownership
    PyObject* release() {
        auto object = m_object;
        m_object = nullptr;
        return object;
    }

    // This returns a reference so you can take a pointer of a pointer, needed
    // for some API methods.
    PyObject*& get() { return m_object; }

    // compare to nullptr
    bool operator==(nullptr_t) const { return m_object == nullptr; }
    bool operator!=(nullptr_t) const { return m_object != nullptr; }

private:

    PyObject* m_object = nullptr;
};

} // namespace lsst::log::detail

#endif // LSST_LOG_PYOBJECTPTR_H
