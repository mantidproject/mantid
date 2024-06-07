// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2015 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include "ConvFitDataView.h"
#include "ConvFitModel.h"
#include "FitDataPresenter.h"

namespace MantidQt {
namespace CustomInterfaces {
namespace Inelastic {

class ConvFitAddWorkspaceDialog;
class MANTIDQT_INELASTIC_DLL ConvFitDataPresenter : public FitDataPresenter {
public:
  ConvFitDataPresenter(IFitTab *tab, IDataModel *model, IFitDataView *view);

  bool addWorkspaceFromDialog(IAddWorkspaceDialog const *dialog) override;

protected:
  void addTableEntry(FitDomainIndex row) override;
};

} // namespace Inelastic
} // namespace CustomInterfaces
} // namespace MantidQt
