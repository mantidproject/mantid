// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2024 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once
#include "DllConfig.h"
#include "MantidAPI/AlgorithmRuntimeProps.h"

#include <optional>
#include <string>

namespace MantidQt {
namespace CustomInterfaces {
namespace ValidationUtils {

MANTID_SPECTROSCOPY_DLL bool groupingStrInRange(std::string const &customString, std::size_t const &spectraMin,
                                                std::size_t const &spectraMax);

MANTID_SPECTROSCOPY_DLL std::optional<std::string>
validateGroupingProperties(std::unique_ptr<Mantid::API::AlgorithmRuntimeProps> properties,
                           std::size_t const &spectraMin, std::size_t const &spectraMax);

} // namespace ValidationUtils
} // namespace CustomInterfaces
} // namespace MantidQt
