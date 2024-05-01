// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2020 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include "DllOption.h"
#include "MantidQtWidgets/Common/FunctionModelSpectra.h"

#include <string>
#include <vector>

namespace MantidQt {
namespace MantidWidgets {

/*
 * Represents a workspace containing a number of spectra to be fitted.
 *
 * Holds a workspace name, and a Spectra object which can represent
 * a continuous or discontinuous spectra range.
 */
struct EXPORT_OPT_MANTIDQT_COMMON FunctionModelDataset {
public:
  FunctionModelDataset(std::string workspaceName, FunctionModelSpectra spectra);

  inline std::string datasetName() const noexcept { return m_workspaceName; }
  std::vector<std::string> domainNames() const;

  inline std::size_t numberOfSpectra() const noexcept { return m_spectra.size().value; }

private:
  std::string m_workspaceName;
  FunctionModelSpectra m_spectra;
};

} // namespace MantidWidgets
} // namespace MantidQt
