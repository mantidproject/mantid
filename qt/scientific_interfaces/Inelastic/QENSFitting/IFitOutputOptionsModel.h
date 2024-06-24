// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2019 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include "DllConfig.h"
#include "MantidAPI/WorkspaceGroup.h"

namespace MantidQt {
namespace CustomInterfaces {
namespace Inelastic {

using SpectrumToPlot = std::pair<std::string, std::size_t>;

class MANTIDQT_INELASTIC_DLL IFitOutputOptionsModel {
public:
  IFitOutputOptionsModel(){};
  virtual ~IFitOutputOptionsModel() = default;

  virtual void setResultWorkspace(Mantid::API::WorkspaceGroup_sptr groupWorkspace) = 0;
  virtual void setPDFWorkspace(Mantid::API::WorkspaceGroup_sptr groupWorkspace) = 0;
  virtual Mantid::API::WorkspaceGroup_sptr getResultWorkspace() const = 0;
  virtual Mantid::API::WorkspaceGroup_sptr getPDFWorkspace() const = 0;

  virtual void removePDFWorkspace() = 0;

  virtual bool isSelectedGroupPlottable(std::string const &selectedGroup) const = 0;
  virtual bool isResultGroupPlottable() const = 0;
  virtual bool isPDFGroupPlottable() const = 0;

  virtual std::vector<SpectrumToPlot> plotResult(std::string const &plotType) const = 0;
  virtual std::vector<SpectrumToPlot> plotPDF(std::string const &workspaceName, std::string const &plotType) const = 0;

  virtual void saveResult() const = 0;

  virtual std::vector<std::string> getWorkspaceParameters(std::string const &selectedGroup) const = 0;
  virtual std::vector<std::string> getPDFWorkspaceNames() const = 0;

  virtual bool isResultGroupSelected(std::string const &selectedGroup) const = 0;

  virtual void replaceFitResult(std::string const &inputName, std::string const &singleFitName,
                                std::string const &outputName) = 0;
};

} // namespace Inelastic
} // namespace CustomInterfaces
} // namespace MantidQt
