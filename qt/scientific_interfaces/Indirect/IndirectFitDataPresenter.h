// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2015 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include "IAddWorkspaceDialog.h"
#include "IIndirectFitDataView.h"
#include "IndirectFitDataTablePresenter.h"
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
  IndirectFitDataPresenter(IIndirectFittingModel *model, IIndirectFitDataView *view);
  ~IndirectFitDataPresenter();

  void setSampleWSSuffices(const QStringList &suffices);
  void setSampleFBSuffices(const QStringList &suffices);
  void setResolutionWSSuffices(const QStringList &suffices);
  void setResolutionFBSuffices(const QStringList &suffices);
  QStringList getSampleWSSuffices() const;
  QStringList getSampleFBSuffices() const;
  QStringList getResolutionWSSuffices() const;
  QStringList getResolutionFBSuffices() const;

  void updateDataInTable();

  UserInputValidator &validate(UserInputValidator &validator);

  DataForParameterEstimationCollection getDataForParameterEstimation(const EstimationDataSelector &selector) const;

protected slots:
  void showAddWorkspaceDialog();

  virtual void closeDialog();

signals:
  void singleResolutionLoaded();
  void dataAdded();
  void dataRemoved();
  void dataChanged();
  void startXChanged(double, WorkspaceID, WorkspaceIndex);
  void startXChanged(double);
  void endXChanged(double, WorkspaceID, WorkspaceIndex);
  void endXChanged(double);
  void requestedAddWorkspaceDialog();

protected:
  IndirectFitDataPresenter(IIndirectFittingModel *model, IIndirectFitDataView *view,
                           std::unique_ptr<IndirectFitDataTablePresenter> tablePresenter);
  IIndirectFitDataView const *getView() const;
  void addData(IAddWorkspaceDialog const *dialog);
  virtual void addDataToModel(IAddWorkspaceDialog const *dialog);
  void displayWarning(const std::string &warning);
  QStringList m_wsSampleSuffixes;
  QStringList m_fbSampleSuffixes;
  QStringList m_wsResolutionSuffixes;
  QStringList m_fbResolutionSuffixes;

private slots:
  void addData();

private:
  virtual std::unique_ptr<IAddWorkspaceDialog> getAddWorkspaceDialog(QWidget *parent) const;

  std::unique_ptr<IAddWorkspaceDialog> m_addWorkspaceDialog;
  IIndirectFittingModel *m_model;
  IIndirectFitDataView *m_view;
  std::unique_ptr<IndirectFitDataTablePresenter> m_tablePresenter;
};

} // namespace IDA
} // namespace CustomInterfaces
} // namespace MantidQt
