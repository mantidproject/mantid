// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2015 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include "FitDataModel.h"
#include "FitDataView.h"
#include "MantidQtWidgets/Common/IAddWorkspaceDialog.h"
#include "ParameterEstimation.h"

#include "DllConfig.h"
#include "InelasticFitPropertyBrowser.h"
#include "MantidAPI/AnalysisDataServiceObserver.h"
#include "MantidAPI/MatrixWorkspace.h"

namespace MantidQt {
namespace CustomInterfaces {
namespace Inelastic {

class IFitTab;

class MANTIDQT_INELASTIC_DLL IFitDataPresenter {
public:
  virtual std::string tabName() const = 0;

  virtual void handleAddData(MantidWidgets::IAddWorkspaceDialog const *dialog) = 0;
  virtual void handleRemoveClicked() = 0;
  virtual void handleUnifyClicked() = 0;
  virtual void handleCellChanged(int row, int column) = 0;
};

class MANTIDQT_INELASTIC_DLL FitDataPresenter : public IFitDataPresenter, public AnalysisDataServiceObserver {
public:
  FitDataPresenter(IFitTab *tab, IFitDataModel *model, IFitDataView *view);
  ~FitDataPresenter();
  virtual bool addWorkspaceFromDialog(MantidWidgets::IAddWorkspaceDialog const *dialog);
  void addWorkspace(const std::string &workspaceName, const FunctionModelSpectra &workspaceIndices);
  void setResolution(const std::string &name);
  void setStartX(double startX, WorkspaceID workspaceID);
  void setStartX(double startX, WorkspaceID workspaceID, WorkspaceIndex spectrum);
  void setEndX(double startX, WorkspaceID workspaceID);
  void setEndX(double startX, WorkspaceID workspaceID, WorkspaceIndex spectrum);

  std::vector<std::pair<std::string, size_t>> getResolutionsForFit() const;

  void updateTableFromModel();
  WorkspaceID getNumberOfWorkspaces() const;
  size_t getNumberOfDomains() const;
  QList<FunctionModelDataset> getDatasets() const;
  DataForParameterEstimationCollection getDataForParameterEstimation(const EstimationDataSelector &selector) const;
  std::vector<double> getQValuesForData() const;
  std::vector<std::string> createDisplayNames() const;
  void validate(UserInputValidator &validator);

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

  std::string tabName() const override;

  void handleAddData(MantidWidgets::IAddWorkspaceDialog const *dialog) override;
  void handleRemoveClicked() override;
  void handleUnifyClicked() override;
  void handleCellChanged(int row, int column) override;

protected:
  IFitDataView const *getView() const;
  void displayWarning(const std::string &warning);
  virtual void addTableEntry(FitDomainIndex row);

  IFitTab *m_tab;
  IFitDataModel *m_model;
  IFitDataView *m_view;

private:
  void setModelStartXAndEmit(double startX, FitDomainIndex row);
  void setModelEndXAndEmit(double endX, FitDomainIndex row);
  void setTableStartXAndEmit(double X, int row, int column);
  void setTableEndXAndEmit(double X, int row, int column);
  void setModelExcludeAndEmit(const std::string &exclude, FitDomainIndex row);
  std::map<int, QModelIndex> getUniqueIndices(const QModelIndexList &selectedIndices);
};

} // namespace Inelastic
} // namespace CustomInterfaces
} // namespace MantidQt
