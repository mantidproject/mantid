// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#include "MantidKernel/MDAxisValidator.h"

namespace Mantid {
namespace Kernel {

//----------------------------------------------------------------------------------------------
/** Constructor
 *  @param axes Vector containing MD axes to validate
 *  @param nDimensions Number of dimensions of input workspace for algorithm
 *  @param checkIfEmpty Whether validator will check if the axes vector is empty
 */
MDAxisValidator::MDAxisValidator(const std::vector<int> &axes,
                                 const size_t nDimensions,
                                 const bool checkIfEmpty)
    : m_axes(axes), m_wsDimensions(nDimensions), m_emptyCheck(checkIfEmpty) {}

/**
 * @brief Checks the MD axes given against the given number of dimensions of the
 * input workspace.
 *
 * @returns A map with validation warnings, to be used in an algorithm's
 * validateInputs()
 */
std::map<std::string, std::string> MDAxisValidator::validate() const {
  std::map<std::string, std::string> invalidProperties;

  // Empty check if required
  // (Some algorithms have special handling for an empty axes vector, e.g.
  // TransposeMD, so don't need an error here).
  if (m_emptyCheck) {
    if (m_axes.empty()) {
      invalidProperties.insert(
          std::make_pair("Axes", "No index was specified."));
    }
  }

  // Make sure that there are fewer axes specified than exist on the workspace
  if (m_axes.size() > m_wsDimensions) {
    invalidProperties.emplace(
        "Axes", "More axes specified than dimensions available in the input");
  }

  // Ensure that the axes selection is within the number of dimensions of the
  // workspace
  if (!m_axes.empty()) {
    auto it = std::max_element(m_axes.begin(), m_axes.end());
    auto largest = static_cast<size_t>(*it);
    if (largest >= m_wsDimensions) {
      invalidProperties.insert(
          std::make_pair("Axes", "One of the axis indexes specified indexes a "
                                 "dimension outside the real dimension range"));
    }
  }

  return invalidProperties;
}

} // namespace Kernel
} // namespace Mantid
