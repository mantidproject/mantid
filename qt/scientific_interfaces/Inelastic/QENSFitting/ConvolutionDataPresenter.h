// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2015 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include "ConvolutionDataView.h"
#include "ConvolutionModel.h"
#include "FitDataPresenter.h"

namespace MantidQt {
namespace CustomInterfaces {
namespace Inelastic {

class ConvolutionAddWorkspaceDialog;
class MANTIDQT_INELASTIC_DLL ConvolutionDataPresenter : public FitDataPresenter {
public:
  ConvolutionDataPresenter(IFitTab *tab, IDataModel *model, IFitDataView *view);

  bool addWorkspaceFromDialog(IAddWorkspaceDialog const *dialog) override;

protected:
  void addTableEntry(FitDomainIndex row) override;
};

} // namespace Inelastic
} // namespace CustomInterfaces
} // namespace MantidQt
