// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2019 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#ifndef MANTIDQTCUSTOMINTERFACESIDA_IINDIRECTFITOUTPUTOPTIONSMODEL_H_
#define MANTIDQTCUSTOMINTERFACESIDA_IINDIRECTFITOUTPUTOPTIONSMODEL_H_

#include "DllConfig.h"
#include "MantidAPI/WorkspaceGroup.h"

namespace MantidQt {
namespace CustomInterfaces {
namespace IDA {

using SpectrumToPlot = std::pair<std::string, std::size_t>;

class MANTIDQT_INDIRECT_DLL IIndirectFitOutputOptionsModel {
public:
  IIndirectFitOutputOptionsModel(){};
  virtual ~IIndirectFitOutputOptionsModel() = default;

  virtual void
  setResultWorkspace(Mantid::API::WorkspaceGroup_sptr groupWorkspace) = 0;
  virtual void
  setPDFWorkspace(Mantid::API::WorkspaceGroup_sptr groupWorkspace) = 0;
  virtual Mantid::API::WorkspaceGroup_sptr getResultWorkspace() const = 0;
  virtual Mantid::API::WorkspaceGroup_sptr getPDFWorkspace() const = 0;

  virtual void removePDFWorkspace() = 0;

  virtual bool
  isSelectedGroupPlottable(std::string const &selectedGroup) const = 0;
  virtual bool isResultGroupPlottable() const = 0;
  virtual bool isPDFGroupPlottable() const = 0;

  virtual void clearSpectraToPlot() = 0;
  virtual std::vector<SpectrumToPlot> getSpectraToPlot() const = 0;

  virtual void plotResult(std::string const &plotType) = 0;
  virtual void plotPDF(std::string const &workspaceName,
                       std::string const &plotType) = 0;

  virtual void saveResult() const = 0;

  virtual std::vector<std::string>
  getWorkspaceParameters(std::string const &selectedGroup) const = 0;
  virtual std::vector<std::string> getPDFWorkspaceNames() const = 0;

  virtual bool
  isResultGroupSelected(std::string const &selectedGroup) const = 0;

  virtual void replaceFitResult(std::string const &inputName,
                                std::string const &singleFitName,
                                std::string const &outputName) = 0;
};

} // namespace IDA
} // namespace CustomInterfaces
} // namespace MantidQt

#endif
