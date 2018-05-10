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
