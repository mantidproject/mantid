// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2024 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once
#include "MantidKernel/DllConfig.h"
#include "MantidKernel/EnumeratedString.h"
#include "MantidKernel/Property.h"
#include "PropertyWithValue.h"
#include <string>
#include <vector>

namespace Mantid {
namespace Kernel {

/** EnumStringProperty : TODO: DESCRIPTION
 */
template <typename T> class DLLExport EnumStringProperty : public PropertyWithValue<T> {

public:
  EnumStringProperty();
};

} // namespace Kernel
} // namespace Mantid
