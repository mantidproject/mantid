// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#include "MantidQtWidgets/MplCpp/ErrorbarContainer.h"

namespace MantidQt {
namespace Widgets {
namespace MplCpp {

ErrorbarContainer::ErrorbarContainer(Python::Object pyInstance)
    : Python::InstanceHolder(std::move(pyInstance)) {}

} // namespace MplCpp
} // namespace Widgets
} // namespace MantidQt
