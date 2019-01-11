// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2011 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#ifndef MANTID_PYTHONINTERFACE_CLONEMATRIXWORKSPACE_H_
#define MANTID_PYTHONINTERFACE_CLONEMATRIXWORKSPACE_H_

#include <vector>

#include <boost/python/object.hpp> //Safer way to include Python.h

namespace Mantid {
namespace API {
class MatrixWorkspace;
}

namespace PythonInterface {
//** @name Numpy clones of data*/
///{
/// Create a numpy array from the X values of the given workspace reference
PyObject *cloneX(API::MatrixWorkspace &self);
/// Create a numpy array from the Y values of the given workspace reference
PyObject *cloneY(API::MatrixWorkspace &self);
/// Create a numpy array from the E values of the given workspace reference
PyObject *cloneE(API::MatrixWorkspace &self);
/// Create a numpy array from the E values of the given workspace reference
PyObject *cloneDx(API::MatrixWorkspace &self);
///@}
} // namespace PythonInterface
} // namespace Mantid

#endif /* MANTID_PYTHONINTERFACE_CLONEMATRIXWORKSPACE_H_ */
