// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2015 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#ifndef MANTIDQTCUSTOMINTERFACESIDA_INDIRECTFITDATAPRESENTER_H_
#define MANTIDQTCUSTOMINTERFACESIDA_INDIRECTFITDATAPRESENTER_H_

#include "IAddWorkspaceDialog.h"
#include "IIndirectFitDataView.h"
#include "IndexTypes.h"
#include "IndirectDataTablePresenter.h"
#include "IndirectFittingModel.h"

#include "DllConfig.h"
#include "MantidAPI/AnalysisDataServiceObserver.h"
#include "MantidAPI/MatrixWorkspace.h"

#include <QObject>

namespace MantidQt {
namespace CustomInterfaces {
namespace IDA {

class MANTIDQT_INDIRECT_DLL IndirectFitDataPresenter
    : public QObject,
      public AnalysisDataServiceObserver {
  Q_OBJECT
public:
  IndirectFitDataPresenter(IndirectFittingModel *model,
                           IIndirectFitDataView *view);
  IndirectFitDataPresenter(
      IndirectFittingModel *model, IIndirectFitDataView *view,
      std::unique_ptr<IndirectDataTablePresenter> tablePresenter);
  ~IndirectFitDataPresenter();

  void setSampleWSSuffices(const QStringList &suffices);
  void setSampleFBSuffices(const QStringList &suffices);
  void setResolutionWSSuffices(const QStringList &suffices);
  void setResolutionFBSuffices(const QStringList &suffices);
  void setMultiInputSampleWSSuffixes();
  void setMultiInputSampleFBSuffixes();
  void setMultiInputResolutionWSSuffixes();
  void setMultiInputResolutionFBSuffixes();

  void setStartX(double startX, DatasetIndex dataIndex,
                 WorkspaceIndex spectrumIndex);
  void setStartX(double startX, DatasetIndex dataIndex);
  void setEndX(double endX, DatasetIndex dataIndex,
               WorkspaceIndex spectrumIndex);
  void setEndX(double endX, DatasetIndex dataIndex);
  void setExclude(const std::string &exclude, DatasetIndex dataIndex,
                  WorkspaceIndex spectrumIndex);

  void loadSettings(const QSettings &settings);
  UserInputValidator &validate(UserInputValidator &validator);

  void replaceHandle(const std::string &workspaceName,
                     const Workspace_sptr &workspace) override;
  DataForParameterEstimationCollection
  getDataForParameterEstimation(EstimationDataSelector selector) const;

public slots:
  void updateSpectraInTable(DatasetIndex dataIndex);

protected slots:
  void setModelWorkspace(const QString &name);
  void setModelFromSingleData();
  void setModelFromMultipleData();
  void showAddWorkspaceDialog();

  virtual void closeDialog();

signals:
  void singleResolutionLoaded();
  void dataAdded();
  void dataRemoved();
  void dataChanged();
  void startXChanged(double, DatasetIndex, WorkspaceIndex);
  void startXChanged(double);
  void endXChanged(double, DatasetIndex, WorkspaceIndex);
  void endXChanged(double);
  void excludeRegionChanged(const std::string &, DatasetIndex, WorkspaceIndex);
  void multipleDataViewSelected();
  void singleDataViewSelected();
  void requestedAddWorkspaceDialog();
  void updateAvailableFitTypes();

protected:
  IIndirectFitDataView const *getView() const;
  void addData(IAddWorkspaceDialog const *dialog);
  virtual void addDataToModel(IAddWorkspaceDialog const *dialog);
  void setSingleModelData(const std::string &name);
  virtual void addModelData(const std::string &name);
  void setResolutionHidden(bool hide);
  void displayWarning(const std::string &warning);

private slots:
  void addData();

private:
  virtual std::unique_ptr<IAddWorkspaceDialog>
  getAddWorkspaceDialog(QWidget *parent) const;
  void updateDataInTable(DatasetIndex dataIndex);
  void selectReplacedWorkspace(const QString &workspaceName);

  virtual void setMultiInputResolutionFBSuffixes(IAddWorkspaceDialog *dialog);
  virtual void setMultiInputResolutionWSSuffixes(IAddWorkspaceDialog *dialog);

  std::unique_ptr<IAddWorkspaceDialog> m_addWorkspaceDialog;
  IndirectFittingModel *m_model;
  PrivateFittingData m_singleData;
  PrivateFittingData m_multipleData;
  IIndirectFitDataView *m_view;
  std::unique_ptr<IndirectDataTablePresenter> m_tablePresenter;
};

} // namespace IDA
} // namespace CustomInterfaces
} // namespace MantidQt

#endif /* MANTIDQTCUSTOMINTERFACESIDA_INDIRECTFITDATAPRESENTER_H_ */
