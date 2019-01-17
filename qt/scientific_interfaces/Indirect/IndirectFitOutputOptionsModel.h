// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#ifndef MANTID_CUSTOMINTERFACES_INDIRECTFITOUTPUTOPTIONSMODEL_H_
#define MANTID_CUSTOMINTERFACES_INDIRECTFITOUTPUTOPTIONSMODEL_H_

#include "DllConfig.h"

namespace MantidQt {
namespace CustomInterfaces {
namespace IDA {

class MANTIDQT_INDIRECT_DLL IndirectFitOutputOptionsModel {
public:
  IndirectFitOutputOptionsModel();
  ~IndirectFitOutputOptionsModel() = default;

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
};

} // namespace IDA
} // namespace CustomInterfaces
} // namespace MantidQt

#endif
