// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2019 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#ifndef MPLCPP_ERRORBARCONTAINER_H
#define MPLCPP_ERRORBARCONTAINER_H

#include "MantidQtWidgets/Common/Python/Object.h"
#include "MantidQtWidgets/MplCpp/DllConfig.h"

namespace MantidQt {
namespace Widgets {
namespace MplCpp {

class MANTID_MPLCPP_DLL ErrorbarContainer
    : public Common::Python::InstanceHolder {
public:
  explicit ErrorbarContainer(Common::Python::Object pyInstance);
};

} // namespace MplCpp
} // namespace Widgets
} // namespace MantidQt

#endif // MPLCPP_ERRORBARCONTAINER_H
