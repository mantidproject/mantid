// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2011 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#ifndef MANTID_PYTHONINTERFACE_NDARRAYTYPEINDEX_H_
#define MANTID_PYTHONINTERFACE_NDARRAYTYPEINDEX_H_

#include "MantidPythonInterface/core/DllConfig.h"

namespace Mantid {
namespace PythonInterface {
namespace Converters {

/**
 * Defines a mapping between C++ type given by
 * the template parameter and numpy type enum
 * NPY_TYPES.
 *
 * There is no general definition, only specialized
 * versions are defined. Each specialization should
 * contain a static const NPY_TYPES definition giving
 * the result of the mapping
 */
template <typename T> struct MANTID_PYTHONINTERFACE_CORE_DLL NDArrayTypeIndex {
  static int typenum;
  static char typecode;
};
} // namespace Converters
} // namespace PythonInterface
} // namespace Mantid

#endif /* MANTID_PYTHONINTERFACE_NDARRAYTYPEINDEX_H_*/
