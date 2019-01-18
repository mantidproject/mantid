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

#include <boost/pointer_cast.hpp>

#include "IndirectTab.h"

namespace MantidQt {
namespace CustomInterfaces {
namespace IDA {

class MANTIDQT_INDIRECT_DLL IndirectFitOutputOptionsModel {
public:
  // IndirectFitOutputOptionsModel(std::unique_ptr<IndirectFitAnalysisTab> tab);
  IndirectFitOutputOptionsModel();
  ~IndirectFitOutputOptionsModel() = default;

  void setActivePlotWorkspace(Mantid::API::WorkspaceGroup_sptr workspace);

  void plotResult(std::string const &plotType);

private:
  void plotAll(Mantid::API::WorkspaceGroup_sptr workspaces);
  void plotParameter(Mantid::API::WorkspaceGroup_sptr workspaces,
                     std::string const &parameter);
  void plotAll(Mantid::API::MatrixWorkspace_sptr workspace);
  void plotParameter(Mantid::API::MatrixWorkspace_sptr workspace,
                     std::string const &parameterToPlot);
  void plotSpectrum(Mantid::API::MatrixWorkspace_sptr workspace,
                    std::string const &parameterToPlot);
  void plotSpectrum(Mantid::API::MatrixWorkspace_sptr workspace);

  Mantid::API::WorkspaceGroup_sptr m_plotWorkspace;
  std::vector<std::string> m_fitParameters;
  std::string m_parameter;
  // std::unique_ptr<IndirectFitAnalysisTab> m_tab;
};

} // namespace IDA
} // namespace CustomInterfaces
} // namespace MantidQt

#endif
