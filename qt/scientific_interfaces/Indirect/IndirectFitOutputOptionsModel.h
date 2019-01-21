// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#ifndef MANTID_CUSTOMINTERFACES_INDIRECTFITOUTPUTOPTIONSMODEL_H_
#define MANTID_CUSTOMINTERFACES_INDIRECTFITOUTPUTOPTIONSMODEL_H_

#include "DllConfig.h"
#include "MantidAPI/MatrixWorkspace.h"
#include "MantidAPI/WorkspaceGroup.h"
#include "MantidQtWidgets/Common/PythonRunner.h"

#include <boost/pointer_cast.hpp>

namespace MantidQt {
namespace CustomInterfaces {
namespace IDA {

using SpectrumToPlot = std::pair<std::string, std::size_t>;

class MANTIDQT_INDIRECT_DLL IndirectFitOutputOptionsModel {
public:
  IndirectFitOutputOptionsModel();
  ~IndirectFitOutputOptionsModel() = default;

  void setResultWorkspace(Mantid::API::WorkspaceGroup_sptr groupWorkspace);
  void setPDFWorkspace(Mantid::API::WorkspaceGroup_sptr groupWorkspace);

  void removePDFWorkspace();

  bool isResultGroupPlottable() const;
  bool isPDFGroupPlottable() const;

  void clearSpectraToPlot();
  std::vector<SpectrumToPlot> getSpectraToPlot() const;

  void plotResult(std::string const &plotType);
  void plotPDF(std::string const &workspaceName, std::string const &plotType);

  void saveResult() const;

  std::vector<std::string>
  getWorkspaceParameters(std::string const &selectedGroup) const;
  std::vector<std::string> getPDFWorkspaceNames() const;

  bool isResultGroupSelected(std::string const &selectedGroup) const;

private:
  void plotResult(Mantid::API::WorkspaceGroup_const_sptr groupWorkspace,
                  std::string const &plotType);
  void plotAll(Mantid::API::WorkspaceGroup_const_sptr groupWorkspace);
  void plotAll(Mantid::API::MatrixWorkspace_const_sptr workspace);
  void plotAllSpectra(Mantid::API::MatrixWorkspace_const_sptr workspace);
  void plotParameter(Mantid::API::WorkspaceGroup_const_sptr groupWorkspace,
                     std::string const &parameter);
  void plotParameter(Mantid::API::MatrixWorkspace_const_sptr workspace,
                     std::string const &parameter);
  void plotParameterSpectrum(Mantid::API::MatrixWorkspace_const_sptr workspace,
                             std::string const &parameter);

  void plotPDF(Mantid::API::MatrixWorkspace_const_sptr workspace,
               std::string const &plotType);

  Mantid::API::WorkspaceGroup_sptr m_resultGroup;
  Mantid::API::WorkspaceGroup_sptr m_pdfGroup;
  std::vector<SpectrumToPlot> m_spectraToPlot;
};

} // namespace IDA
} // namespace CustomInterfaces
} // namespace MantidQt

#endif
