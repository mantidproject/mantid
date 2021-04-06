// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2015 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include "IAddWorkspaceDialog.h"
#include "IIndirectFitDataView.h"
#include "IndirectDataTablePresenter.h"
#include "IndirectFitDataView.h"
#include "IndirectFittingModel.h"
#include "MantidQtWidgets/Common/IndexTypes.h"

#include "DllConfig.h"
#include "MantidAPI/AnalysisDataServiceObserver.h"
#include "MantidAPI/MatrixWorkspace.h"

#include <QObject>

namespace MantidQt {
namespace CustomInterfaces {
namespace IDA {
using namespace MantidWidgets;

class MANTIDQT_INDIRECT_DLL IndirectFitDataPresenter : public QObject, public AnalysisDataServiceObserver {
  Q_OBJECT
public:
  IndirectFitDataPresenter(IndirectFittingModel *model, IIndirectFitDataView *view);
  ~IndirectFitDataPresenter();

  void setSampleWSSuffices(const QStringList &suffices);
  void setSampleFBSuffices(const QStringList &suffices);
  void setResolutionWSSuffices(const QStringList &suffices);
  void setResolutionFBSuffices(const QStringList &suffices);
  void setMultiInputSampleWSSuffixes();
  void setMultiInputSampleFBSuffixes();
  void setMultiInputResolutionWSSuffixes();
  void setMultiInputResolutionFBSuffixes();

  void setStartX(double startX, TableDatasetIndex dataIndex, WorkspaceIndex spectrumIndex);
  void setStartX(double startX, TableDatasetIndex dataIndex);
  void setEndX(double endX, TableDatasetIndex dataIndex, WorkspaceIndex spectrumIndex);
  void setEndX(double endX, TableDatasetIndex dataIndex);
  void setExclude(const std::string &exclude, TableDatasetIndex dataIndex, WorkspaceIndex spectrumIndex);

  void loadSettings(const QSettings &settings);
  UserInputValidator &validate(UserInputValidator &validator);

  void replaceHandle(const std::string &workspaceName, const Workspace_sptr &workspace) override;
  DataForParameterEstimationCollection getDataForParameterEstimation(const EstimationDataSelector &selector) const;

public slots:
  void updateSpectraInTable(TableDatasetIndex dataIndex);

protected slots:
  void setModelWorkspace(const QString &name);
  void setModelFromSingleData();
  void setModelFromMultipleData();
  void showAddWorkspaceDialog();
  virtual void handleSampleLoaded(const QString &);

  virtual void closeDialog();

signals:
  void singleResolutionLoaded();
  void dataAdded();
  void dataRemoved();
  void dataChanged();
  void startXChanged(double, TableDatasetIndex, WorkspaceIndex);
  void startXChanged(double);
  void endXChanged(double, TableDatasetIndex, WorkspaceIndex);
  void endXChanged(double);
  void excludeRegionChanged(const std::string &, TableDatasetIndex, WorkspaceIndex);
  void multipleDataViewSelected();
  void singleDataViewSelected();
  void requestedAddWorkspaceDialog();
  void updateAvailableFitTypes();

protected:
  IndirectFitDataPresenter(IndirectFittingModel *model, IIndirectFitDataView *view,
                           std::unique_ptr<IndirectDataTablePresenter> tablePresenter);
  IIndirectFitDataView const *getView() const;
  void addData(IAddWorkspaceDialog const *dialog);
  virtual void addDataToModel(IAddWorkspaceDialog const *dialog);
  void setSingleModelData(const std::string &name);
  void updateRanges();
  virtual void addModelData(const std::string &name);
  void setResolutionHidden(bool hide);
  void displayWarning(const std::string &warning);

private slots:
  void addData();

private:
  virtual std::unique_ptr<IAddWorkspaceDialog> getAddWorkspaceDialog(QWidget *parent) const;
  void updateDataInTable(TableDatasetIndex dataIndex);
  void selectReplacedWorkspace(const QString &workspaceName);

  virtual void setMultiInputResolutionFBSuffixes(IAddWorkspaceDialog *dialog);
  virtual void setMultiInputResolutionWSSuffixes(IAddWorkspaceDialog *dialog);

  std::unique_ptr<IAddWorkspaceDialog> m_addWorkspaceDialog;
  IndirectFittingModel *m_model;
  IIndirectFitDataView *m_view;
  std::unique_ptr<IndirectDataTablePresenter> m_tablePresenter;
};

} // namespace IDA
} // namespace CustomInterfaces
} // namespace MantidQt
