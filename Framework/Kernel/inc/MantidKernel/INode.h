// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2013 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

//----------------------------------------------------------------------
// Includes
//----------------------------------------------------------------------
#include "MantidKernel/ISaveable.h"

namespace Mantid {
namespace Kernel {
/** Helper class providing interface to ISAveable
 */
class MANTID_KERNEL_DLL INode {
public:
  virtual ~INode() {};
};
} // namespace Kernel
} // namespace Mantid
