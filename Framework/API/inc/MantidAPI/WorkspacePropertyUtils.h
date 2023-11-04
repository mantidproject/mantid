// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2023 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include "MantidAPI/IWorkspaceProperty.h"
#include "MantidKernel/PropertyWithValue.h"
#include "MantidKernel/System.h"

namespace Mantid {
namespace API {

void setPropertyModeForWorkspaceProperty(Mantid::Kernel::Property *prop, const PropertyMode::Type &optional);

}
} // namespace Mantid