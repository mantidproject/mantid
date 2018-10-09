// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2015 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#ifndef MANTIDQTCUSTOMINTERFACESIDA_JUMPFITDATAPRESENTER_H_
#define MANTIDQTCUSTOMINTERFACESIDA_JUMPFITDATAPRESENTER_H_

#include "IndirectFitDataPresenter.h"
#include "JumpFitAddWorkspaceDialog.h"
#include "JumpFitModel.h"

#include <QComboBox>
#include <QSpacerItem>

namespace MantidQt {
namespace CustomInterfaces {
namespace IDA {

/**
  A presenter.
*/
class DLLExport JumpFitDataPresenter : public IndirectFitDataPresenter {
  Q_OBJECT
public:
  JumpFitDataPresenter(JumpFitModel *model, IndirectFitDataView *view,
                       QComboBox *cbParameterType, QComboBox *cbParameter,
                       QLabel *lbParameterType, QLabel *lbParameter);

private slots:
  void hideParameterComboBoxes();
  void showParameterComboBoxes();
  void updateAvailableParameters();
  void updateAvailableParameters(int typeIndex);
  void updateParameterSelectionEnabled();
  void setParameterLabel(const QString &parameter);
  void setDialogParameterNames(JumpFitAddWorkspaceDialog *dialog,
                               const std::string &workspace);
  void setDialogParameterNames(JumpFitAddWorkspaceDialog *dialog,
                               int parameterType);
  void setActiveParameterType(int type);
  void updateActiveDataIndex();
  void setSingleModelSpectrum(int index);

private:
  void setAvailableParameters(const std::vector<std::string> &parameters);
  void addDataToModel(IAddWorkspaceDialog const *dialog) override;
  void dialogExecuted(IAddWorkspaceDialog const *dialog,
                      QDialog::DialogCode result) override;
  std::unique_ptr<IAddWorkspaceDialog>
  getAddWorkspaceDialog(QWidget *parent) const override;
  void updateParameterOptions(JumpFitAddWorkspaceDialog *dialog);
  void updateParameterTypes(JumpFitAddWorkspaceDialog *dialog);
  std::vector<std::string> getParameterTypes(std::size_t dataIndex) const;
  void addWorkspace(IndirectFittingModel *model, const std::string &name);
  void setModelSpectrum(int index);

  int m_activeParameterType;
  std::size_t m_dataIndex;

  QComboBox *m_cbParameterType;
  QComboBox *m_cbParameter;
  QLabel *m_lbParameterType;
  QLabel *m_lbParameter;
  JumpFitModel *m_jumpModel;
};

} // namespace IDA
} // namespace CustomInterfaces
} // namespace MantidQt

#endif /* MANTIDQTCUSTOMINTERFACESIDA_JUMPFITDATAPRESENTER_H_ */
