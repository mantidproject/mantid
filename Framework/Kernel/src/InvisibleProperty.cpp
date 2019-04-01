// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#include "MantidKernel/InvisibleProperty.h"

namespace Mantid {
namespace Kernel {

/// Is the property to be shown in the GUI? Always false.
bool InvisibleProperty::isVisible(const IPropertyManager * /*algo*/) const {
  return false;
}

/// Make a copy of the present type of IPropertySettings
IPropertySettings *InvisibleProperty::clone() const {
  return new InvisibleProperty(*this);
}

} // namespace Kernel
} // namespace Mantid
