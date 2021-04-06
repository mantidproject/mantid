// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2019 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include "IIndirectFitOutputOptionsModel.h"

#include "DllConfig.h"
#include "MantidAPI/IAlgorithm.h"
#include "MantidAPI/MatrixWorkspace.h"
#include "MantidAPI/WorkspaceGroup.h"

#include <boost/pointer_cast.hpp>

namespace MantidQt {
namespace CustomInterfaces {
namespace IDA {

class MANTIDQT_INDIRECT_DLL IndirectFitOutputOptionsModel : public IIndirectFitOutputOptionsModel {
public:
  IndirectFitOutputOptionsModel();
  virtual ~IndirectFitOutputOptionsModel() override = default;

  void setResultWorkspace(Mantid::API::WorkspaceGroup_sptr groupWorkspace) override;
  void setPDFWorkspace(Mantid::API::WorkspaceGroup_sptr groupWorkspace) override;
  Mantid::API::WorkspaceGroup_sptr getResultWorkspace() const override;
  Mantid::API::WorkspaceGroup_sptr getPDFWorkspace() const override;

  void removePDFWorkspace() override;

  bool isSelectedGroupPlottable(std::string const &selectedGroup) const override;
  bool isResultGroupPlottable() const override;
  bool isPDFGroupPlottable() const override;

  void clearSpectraToPlot() override;
  std::vector<SpectrumToPlot> getSpectraToPlot() const override;

  void plotResult(std::string const &plotType) override;
  void plotPDF(std::string const &workspaceName, std::string const &plotType) override;

  void saveResult() const override;

  std::vector<std::string> getWorkspaceParameters(std::string const &selectedGroup) const override;
  std::vector<std::string> getPDFWorkspaceNames() const override;

  bool isResultGroupSelected(std::string const &selectedGroup) const override;

  void replaceFitResult(std::string const &inputName, std::string const &singleFitName,
                        std::string const &outputName) override;

private:
  void plotResult(const Mantid::API::WorkspaceGroup_const_sptr &groupWorkspace, std::string const &plotType);
  void plotAll(const Mantid::API::WorkspaceGroup_const_sptr &groupWorkspace);
  void plotAll(const Mantid::API::MatrixWorkspace_const_sptr &workspace);
  void plotAllSpectra(const Mantid::API::MatrixWorkspace_const_sptr &workspace);
  void plotParameter(const Mantid::API::WorkspaceGroup_const_sptr &groupWorkspace, std::string const &parameter);
  void plotParameter(const Mantid::API::MatrixWorkspace_const_sptr &workspace, std::string const &parameter);
  void plotParameterSpectrum(const Mantid::API::MatrixWorkspace_const_sptr &workspace, std::string const &parameter);

  void plotPDF(const Mantid::API::MatrixWorkspace_const_sptr &workspace, std::string const &plotType);

  void replaceFitResult(const Mantid::API::MatrixWorkspace_sptr &inputWorkspace,
                        const Mantid::API::MatrixWorkspace_sptr &singleFitWorkspace, std::string const &outputName);
  void setOutputAsResultWorkspace(const Mantid::API::IAlgorithm_sptr &algorithm);
  void setResultWorkspace(std::string const &groupName);

  Mantid::API::WorkspaceGroup_sptr m_resultGroup;
  Mantid::API::WorkspaceGroup_sptr m_pdfGroup;
  std::vector<SpectrumToPlot> m_spectraToPlot;
};

} // namespace IDA
} // namespace CustomInterfaces
} // namespace MantidQt
