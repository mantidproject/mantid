// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#include "MantidQtWidgets/MplCpp/ErrorbarContainer.h"

namespace MantidQt::Widgets {
namespace Python = Common::Python;
namespace MplCpp {

ErrorbarContainer::ErrorbarContainer(Python::Object pyInstance) : Python::InstanceHolder(std::move(pyInstance)) {}

} // namespace MplCpp
} // namespace MantidQt::Widgets
