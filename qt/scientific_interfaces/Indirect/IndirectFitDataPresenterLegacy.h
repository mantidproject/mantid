// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2015 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#ifndef MANTIDQTCUSTOMINTERFACESIDA_INDIRECTFITDATAPRESENTERLEGACY_H_
#define MANTIDQTCUSTOMINTERFACESIDA_INDIRECTFITDATAPRESENTERLEGACY_H_

#include "IAddWorkspaceDialog.h"
#include "IIndirectFitDataViewLegacy.h"
#include "IndirectDataTablePresenterLegacy.h"
#include "IndirectFittingModelLegacy.h"

#include "DllConfig.h"
#include "MantidAPI/AnalysisDataServiceObserver.h"
#include "MantidAPI/MatrixWorkspace.h"

#include <QObject>

namespace MantidQt {
namespace CustomInterfaces {
namespace IDA {

class MANTIDQT_INDIRECT_DLL IndirectFitDataPresenterLegacy
    : public QObject,
      public AnalysisDataServiceObserver {
  Q_OBJECT
public:
  IndirectFitDataPresenterLegacy(IndirectFittingModelLegacy *model,
                                 IIndirectFitDataViewLegacy *view);
  IndirectFitDataPresenterLegacy(
      IndirectFittingModelLegacy *model, IIndirectFitDataViewLegacy *view,
      std::unique_ptr<IndirectDataTablePresenterLegacy> tablePresenter);
  ~IndirectFitDataPresenterLegacy();

  void setSampleWSSuffices(const QStringList &suffices);
  void setSampleFBSuffices(const QStringList &suffices);
  void setResolutionWSSuffices(const QStringList &suffices);
  void setResolutionFBSuffices(const QStringList &suffices);
  void setMultiInputSampleWSSuffixes();
  void setMultiInputSampleFBSuffixes();
  void setMultiInputResolutionWSSuffixes();
  void setMultiInputResolutionFBSuffixes();

  void setStartX(double startX, std::size_t dataIndex, int spectrumIndex);
  void setEndX(double endX, std::size_t dataIndex, int spectrumIndex);
  void setExclude(const std::string &exclude, std::size_t dataIndex,
                  int spectrumIndex);

  void loadSettings(const QSettings &settings);
  UserInputValidator &validate(UserInputValidator &validator);

  void replaceHandle(const std::string &workspaceName,
                     const Workspace_sptr &workspace) override;

public slots:
  void updateSpectraInTable(std::size_t dataIndex);

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
  void startXChanged(double /*_t1*/, std::size_t /*_t2*/, std::size_t /*_t3*/);
  void endXChanged(double /*_t1*/, std::size_t /*_t2*/, std::size_t /*_t3*/);
  void excludeRegionChanged(const std::string & /*_t1*/, std::size_t /*_t2*/,
                            std::size_t /*_t3*/);
  void multipleDataViewSelected();
  void singleDataViewSelected();
  void requestedAddWorkspaceDialog();
  void updateAvailableFitTypes();

protected:
  IIndirectFitDataViewLegacy const *getView() const;
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
  void updateDataInTable(std::size_t dataIndex);

  void selectReplacedWorkspace(const QString &workspaceName);

  virtual void setMultiInputResolutionFBSuffixes(IAddWorkspaceDialog *dialog);
  virtual void setMultiInputResolutionWSSuffixes(IAddWorkspaceDialog *dialog);

  std::unique_ptr<IAddWorkspaceDialog> m_addWorkspaceDialog;
  IndirectFittingModelLegacy *m_model;
  PrivateFittingDataLegacy m_singleData;
  PrivateFittingDataLegacy m_multipleData;
  IIndirectFitDataViewLegacy *m_view;
  std::unique_ptr<IndirectDataTablePresenterLegacy> m_tablePresenter;
};

} // namespace IDA
} // namespace CustomInterfaces
} // namespace MantidQt

#endif /* MANTIDQTCUSTOMINTERFACESIDA_INDIRECTFITDATAPRESENTERLEGACY_H_ */
