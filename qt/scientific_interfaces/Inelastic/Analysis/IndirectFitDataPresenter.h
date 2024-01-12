// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2015 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include "Common/IAddWorkspaceDialog.h"
#include "IndirectFitDataModel.h"
#include "IndirectFitDataView.h"
#include "ParameterEstimation.h"

#include "DllConfig.h"
#include "IndirectFitPropertyBrowser.h"
#include "MantidAPI/AnalysisDataServiceObserver.h"
#include "MantidAPI/MatrixWorkspace.h"

namespace MantidQt {
namespace CustomInterfaces {
namespace IDA {

using namespace MantidWidgets;

class IIndirectDataAnalysisTab;

class MANTIDQT_INELASTIC_DLL IIndirectFitDataPresenter {
public:
  virtual void handleAddData(IAddWorkspaceDialog const *dialog) = 0;
  virtual void handleRemoveClicked() = 0;
  virtual void handleUnifyClicked() = 0;
  virtual void handleCellChanged(int row, int column) = 0;
};

class MANTIDQT_INELASTIC_DLL IndirectFitDataPresenter : public IIndirectFitDataPresenter,
                                                        public AnalysisDataServiceObserver {
public:
  IndirectFitDataPresenter(IIndirectDataAnalysisTab *tab, IIndirectFitDataModel *model, IIndirectFitDataView *view);
  ~IndirectFitDataPresenter();
  std::vector<IndirectFitData> *getFittingData();
  virtual bool addWorkspaceFromDialog(IAddWorkspaceDialog const *dialog);
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

  void updateTableFromModel();
  WorkspaceID getNumberOfWorkspaces() const;
  size_t getNumberOfDomains() const;
  FunctionModelSpectra getSpectra(WorkspaceID workspaceID) const;
  DataForParameterEstimationCollection getDataForParameterEstimation(const EstimationDataSelector &selector) const;
  std::vector<double> getQValuesForData() const;
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

  virtual void subscribeFitPropertyBrowser(IIndirectFitPropertyBrowser *browser) { UNUSED_ARG(browser); };

  void handleAddData(IAddWorkspaceDialog const *dialog) override;
  void handleRemoveClicked() override;
  void handleUnifyClicked() override;
  void handleCellChanged(int row, int column) override;


protected:
  IIndirectFitDataView const *getView() const;
  void displayWarning(const std::string &warning);
  virtual void addTableEntry(FitDomainIndex row);

  IIndirectDataAnalysisTab *m_tab;
  IIndirectFitDataModel *m_model;
  IIndirectFitDataView *m_view;

private:
  void setModelStartXAndEmit(double startX, FitDomainIndex row);
  void setModelEndXAndEmit(double endX, FitDomainIndex row);
  void setTableStartXAndEmit(double X, int row, int column);
  void setTableEndXAndEmit(double X, int row, int column);
  void setModelExcludeAndEmit(const std::string &exclude, FitDomainIndex row);
  std::map<int, QModelIndex> getUniqueIndices(const QModelIndexList &selectedIndices);
};

} // namespace IDA
} // namespace CustomInterfaces
} // namespace MantidQt
