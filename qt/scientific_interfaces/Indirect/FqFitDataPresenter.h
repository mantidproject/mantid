// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2015 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include "FQFitConstants.h"
#include "FqFitAddWorkspaceDialog.h"
#include "FqFitModel.h"
#include "IFQFitObserver.h"
#include "IndirectFitDataPresenter.h"
#include "IndirectFunctionBrowser/SingleFunctionTemplateBrowser.h"
#include "Notifier.h"

#include <QComboBox>
#include <QSpacerItem>

namespace MantidQt {
namespace CustomInterfaces {
namespace IDA {

class MANTIDQT_INDIRECT_DLL FqFitDataPresenter : public IndirectFitDataPresenter {
  Q_OBJECT
public:
  FqFitDataPresenter(FqFitModel *model, IIndirectFitDataView *view, QComboBox *cbParameterType, QComboBox *cbParameter,
                     QLabel *lbParameterType, QLabel *lbParameter, IFQFitObserver *SingleFunctionTemplateBrowser);

private slots:
  void hideParameterComboBoxes();
  void showParameterComboBoxes();
  void updateAvailableParameters();
  void updateAvailableParameterTypes();
  void updateAvailableParameters(const QString &type);
  void updateParameterSelectionEnabled();
  void setParameterLabel(const QString &parameter);
  void dialogParameterTypeUpdated(FqFitAddWorkspaceDialog *dialog, const std::string &type);
  void setDialogParameterNames(FqFitAddWorkspaceDialog *dialog, const std::string &workspace);
  void setActiveParameterType(const std::string &type);
  void updateActiveDataIndex();
  void updateActiveDataIndex(int index);
  void setSingleModelSpectrum(int index);
  void handleParameterTypeChanged(const QString &parameter);
  void handleSpectrumSelectionChanged(int parameterIndex);
  void handleMultipleInputSelected();
  void handleSingleInputSelected();

signals:
  void spectrumChanged(WorkspaceIndex);

protected slots:
  void handleSampleLoaded(const QString &) override;

private:
  void setAvailableParameters(const std::vector<std::string> &parameters);
  void addDataToModel(IAddWorkspaceDialog const *dialog) override;
  void closeDialog() override;
  std::unique_ptr<IAddWorkspaceDialog> getAddWorkspaceDialog(QWidget *parent) const override;
  void updateParameterOptions(FqFitAddWorkspaceDialog *dialog);
  void updateParameterTypes(FqFitAddWorkspaceDialog *dialog);
  std::vector<std::string> getParameterTypes(TableDatasetIndex dataIndex) const;
  void addWorkspace(IndirectFittingModel *model, const std::string &name);
  void setModelSpectrum(int index);
  void setDataIndexToCurrentWorkspace(IAddWorkspaceDialog const *dialog);

  void setMultiInputResolutionFBSuffixes(IAddWorkspaceDialog *dialog) override;
  void setMultiInputResolutionWSSuffixes(IAddWorkspaceDialog *dialog) override;

  std::string m_activeParameterType;
  TableDatasetIndex m_dataIndex;

  QComboBox *m_cbParameterType;
  QComboBox *m_cbParameter;
  QLabel *m_lbParameterType;
  QLabel *m_lbParameter;
  FqFitModel *m_fqFitModel;
  Notifier<IFQFitObserver> m_notifier;
};

} // namespace IDA
} // namespace CustomInterfaces
} // namespace MantidQt
