// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#include "MantidVatesSimpleGuiQtWidgets/AxisInformation.h"

namespace Mantid {
namespace Vates {
namespace SimpleGui {

AxisInformation::AxisInformation() {
  this->title = std::string("");
  this->minimum = -9999999.0;
  this->maximum = -9999999.0;
}
} // namespace SimpleGui
} // namespace Vates
} // namespace Mantid
