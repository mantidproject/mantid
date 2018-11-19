// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2015 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#ifndef MANTIDQTCUSTOMINTERFACESIDA_CONVFITDATAPRESENTER_H_
#define MANTIDQTCUSTOMINTERFACESIDA_CONVFITDATAPRESENTER_H_

#include "ConvFitModel.h"
#include "IndirectFitDataPresenter.h"

namespace MantidQt {
namespace CustomInterfaces {
namespace IDA {

class ConvFitAddWorkspaceDialog;

/**
  A presenter.
*/
class DLLExport ConvFitDataPresenter : public IndirectFitDataPresenter {
  Q_OBJECT
public:
  ConvFitDataPresenter(ConvFitModel *model, IndirectFitDataView *view);

private slots:
  void setModelResolution(const QString &name);

protected:
  void addModelData(const std::string &name) override;

private:
  void addDataToModel(IAddWorkspaceDialog const *dialog) override;
  std::unique_ptr<IAddWorkspaceDialog>
  getAddWorkspaceDialog(QWidget *parent) const override;
  void addWorkspace(ConvFitAddWorkspaceDialog const *dialog,
                    IndirectFittingModel *model);

  ConvFitModel *m_convModel;
};

} // namespace IDA
} // namespace CustomInterfaces
} // namespace MantidQt

#endif /* MANTIDQTCUSTOMINTERFACESIDA_CONVFITDATAPRESENTER_H_ */
