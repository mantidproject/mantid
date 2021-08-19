// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2015 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include "IAddWorkspaceDialog.h"
#include "IIndirectFitDataView.h"
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
  IndirectFitDataPresenter(IIndirectFitDataModel *model, IIndirectFitDataView *view);
  ~IndirectFitDataPresenter();
  void addWorkspace(const std::string &workspaceName, const std::string &spectra);
  void setResolution(const std::string &name);
  void setSampleWSSuffices(const QStringList &suffices);
  void setSampleFBSuffices(const QStringList &suffices);
  void setResolutionWSSuffices(const QStringList &suffices);
  void setResolutionFBSuffices(const QStringList &suffices);
  void setStartX(double startX, WorkspaceID workspaceID);
  void setStartX(double startX, WorkspaceID workspaceID, WorkspaceIndex spectrum);
  void setEndX(double startX, WorkspaceID workspaceID);
  void setEndX(double startX, WorkspaceID workspaceID, WorkspaceIndex spectrum);
  std::vector<std::pair<std::string, size_t>> getResolutionsForFit() const;
  QStringList getSampleWSSuffices() const;
  QStringList getSampleFBSuffices() const;
  QStringList getResolutionWSSuffices() const;
  QStringList getResolutionFBSuffices() const;
  void updateTableFromModel();
  size_t getNumberOfDomains();
  std::vector<double> getQValuesForData() const;
  std::string createDisplayName(WorkspaceID workspaceID) const;
  std::vector<std::string> createDisplayNames() const;
  UserInputValidator &validate(UserInputValidator &validator);

  virtual void addWorkspace(const std::string &workspaceName, const std::string &paramType, const int &spectrum_index) {
    UNUSED_ARG(workspaceName);
    UNUSED_ARG(paramType);
    UNUSED_ARG(spectrum_index);
  };

  virtual void setActiveWidth(std::size_t widthIndex, WorkspaceID dataIndex, bool single = true) {
    UNUSED_ARG(widthIndex);
    UNUSED_ARG(dataIndex);
    UNUSED_ARG(single);
  };
  virtual void setActiveEISF(std::size_t eisfIndex, WorkspaceID dataIndex, bool single = true) {
    UNUSED_ARG(eisfIndex);
    UNUSED_ARG(dataIndex);
    UNUSED_ARG(single);
  };

protected slots:
  void showAddWorkspaceDialog();

  virtual void closeDialog();

signals:
  void singleResolutionLoaded();
  void dataAdded(IAddWorkspaceDialog const *);
  void dataRemoved();
  void dataChanged();
  void startXChanged(double, WorkspaceID, WorkspaceIndex);
  void startXChanged(double);
  void endXChanged(double, WorkspaceID, WorkspaceIndex);
  void endXChanged(double);
  void requestedAddWorkspaceDialog();

protected:
  IIndirectFitDataView const *getView() const;
  void addData(IAddWorkspaceDialog const *dialog);
  void displayWarning(const std::string &warning);
  virtual void addTableEntry(FitDomainIndex row);
  QStringList m_wsSampleSuffixes;
  QStringList m_fbSampleSuffixes;
  QStringList m_wsResolutionSuffixes;
  QStringList m_fbResolutionSuffixes;
  IIndirectFitDataModel *m_model;
  IIndirectFitDataView *m_view;

private slots:
  void addData();
  void handleCellChanged(int row, int column);
  void removeSelectedData();

private:
  virtual std::unique_ptr<IAddWorkspaceDialog> getAddWorkspaceDialog(QWidget *parent) const;
  void setModelStartXAndEmit(double startX, FitDomainIndex row);
  void setModelEndXAndEmit(double endX, FitDomainIndex row);
  void setModelExcludeAndEmit(const std::string &exclude, FitDomainIndex row);

  std::unique_ptr<IAddWorkspaceDialog> m_addWorkspaceDialog;
  bool m_emitCellChanged = true;
};

} // namespace IDA
} // namespace CustomInterfaces
} // namespace MantidQt
