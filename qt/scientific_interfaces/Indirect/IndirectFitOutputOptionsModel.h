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

// struct PlotWorkspaces {
//  PlotWorkspaces() : m_resultGroup(), m_pdfGroup() {}
//  PlotWorkspaces(Mantid::API::WorkspaceGroup_sptr resultGroup,
//                 Mantid::API::WorkspaceGroup_sptr pdfGroup)
//      : m_resultGroup(resultGroup), m_pdfGroup(pdfGroup) {}
//
// private:
//  Mantid::API::WorkspaceGroup_sptr m_resultGroup;
//  Mantid::API::WorkspaceGroup_sptr m_pdfGroup;
//};

class MANTIDQT_INDIRECT_DLL IndirectFitOutputOptionsModel {
public:
  IndirectFitOutputOptionsModel();
  ~IndirectFitOutputOptionsModel() = default;

  void setResultWorkspace(Mantid::API::WorkspaceGroup_sptr group);
  void setPDFWorkspace(Mantid::API::WorkspaceGroup_sptr group);

  void removePDFWorkspace();

  bool isResultGroupPlottable() const;
  bool isPDFGroupPlottable() const;

  void clearSpectraToPlot();
  std::vector<SpectrumToPlot> getSpectraToPlot() const;

  void plotResult(std::string const &plotType);
  void saveResult() const;

  std::vector<std::string> getActiveWorkspaceParameters() const;

private:
  void plotResult(std::string const &plotType,
                  Mantid::API::WorkspaceGroup_sptr resultGroup);
  void plotAll(Mantid::API::WorkspaceGroup_sptr resultGroup);
  void plotAll(Mantid::API::MatrixWorkspace_sptr workspace);
  void plotAllSpectra(Mantid::API::MatrixWorkspace_sptr workspace);
  void plotParameter(Mantid::API::WorkspaceGroup_sptr resultGroup,
                     std::string const &parameter);
  void plotParameter(Mantid::API::MatrixWorkspace_sptr workspace,
                     std::string const &parameter);
  void plotParameterSpectrum(Mantid::API::MatrixWorkspace_sptr workspace,
                             std::string const &parameter);

  Mantid::API::WorkspaceGroup_sptr m_resultGroup;
  Mantid::API::WorkspaceGroup_sptr m_pdfGroup;
  Mantid::API::WorkspaceGroup_sptr m_activeGroup;
  std::vector<SpectrumToPlot> m_spectraToPlot;
};

} // namespace IDA
} // namespace CustomInterfaces
} // namespace MantidQt

#endif
