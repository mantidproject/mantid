// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2019 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include "IFitOutputOptionsModel.h"

#include "DllConfig.h"
#include "MantidAPI/IAlgorithm.h"
#include "MantidAPI/MatrixWorkspace.h"
#include "MantidAPI/WorkspaceGroup.h"

#include <boost/pointer_cast.hpp>

namespace MantidQt {
namespace CustomInterfaces {
namespace Inelastic {

class MANTIDQT_INELASTIC_DLL FitOutputOptionsModel : public IFitOutputOptionsModel {
public:
  FitOutputOptionsModel();
  virtual ~FitOutputOptionsModel() override = default;

  void setResultWorkspace(Mantid::API::WorkspaceGroup_sptr groupWorkspace) override;
  void setPDFWorkspace(Mantid::API::WorkspaceGroup_sptr groupWorkspace) override;
  Mantid::API::WorkspaceGroup_sptr getResultWorkspace() const override;
  Mantid::API::WorkspaceGroup_sptr getPDFWorkspace() const override;

  void removePDFWorkspace() override;

  bool isSelectedGroupPlottable(std::string const &selectedGroup) const override;
  bool isResultGroupPlottable() const override;
  bool isPDFGroupPlottable() const override;

  std::vector<SpectrumToPlot> plotResult(std::string const &plotType) const override;
  std::vector<SpectrumToPlot> plotPDF(std::string const &workspaceName, std::string const &plotType) const override;

  void saveResult() const override;

  std::vector<std::string> getWorkspaceParameters(std::string const &selectedGroup) const override;
  std::vector<std::string> getPDFWorkspaceNames() const override;

  bool isResultGroupSelected(std::string const &selectedGroup) const override;

  void replaceFitResult(std::string const &inputName, std::string const &singleFitName,
                        std::string const &outputName) override;

private:
  void plotResult(std::vector<SpectrumToPlot> &spectraToPlot,
                  const Mantid::API::WorkspaceGroup_const_sptr &groupWorkspace, std::string const &plotType) const;
  void plotAll(std::vector<SpectrumToPlot> &spectraToPlot,
               const Mantid::API::WorkspaceGroup_const_sptr &groupWorkspace) const;
  void plotAll(std::vector<SpectrumToPlot> &spectraToPlot,
               const Mantid::API::MatrixWorkspace_const_sptr &workspace) const;
  void plotAllSpectra(std::vector<SpectrumToPlot> &spectraToPlot,
                      const Mantid::API::MatrixWorkspace_const_sptr &workspace) const;
  void plotParameter(std::vector<SpectrumToPlot> &spectraToPlot,
                     const Mantid::API::WorkspaceGroup_const_sptr &groupWorkspace, std::string const &parameter) const;
  void plotParameter(std::vector<SpectrumToPlot> &spectraToPlot,
                     const Mantid::API::MatrixWorkspace_const_sptr &workspace, std::string const &parameter) const;
  void plotParameterSpectrum(std::vector<SpectrumToPlot> &spectraToPlot,
                             const Mantid::API::MatrixWorkspace_const_sptr &workspace,
                             std::string const &parameter) const;

  void plotPDF(std::vector<SpectrumToPlot> &spectraToPlot, const Mantid::API::MatrixWorkspace_const_sptr &workspace,
               std::string const &plotType) const;

  void replaceFitResult(const Mantid::API::MatrixWorkspace_sptr &inputWorkspace,
                        const Mantid::API::MatrixWorkspace_sptr &singleFitWorkspace, std::string const &outputName);
  void setOutputAsResultWorkspace(const Mantid::API::IAlgorithm_sptr &algorithm);
  void setResultWorkspace(std::string const &groupName);

  Mantid::API::WorkspaceGroup_sptr m_resultGroup;
  Mantid::API::WorkspaceGroup_sptr m_pdfGroup;
};

} // namespace Inelastic
} // namespace CustomInterfaces
} // namespace MantidQt
