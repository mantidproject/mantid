// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2015 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include "FitDataView.h"
#include "MantidQtWidgets/Common/IAddWorkspaceDialog.h"
#include "MantidQtWidgets/Spectroscopy/DataModel.h"
#include "ParameterEstimation.h"

#include "DllConfig.h"
#include "InelasticFitPropertyBrowser.h"
#include "MantidAPI/MatrixWorkspace.h"
#include "MantidQtWidgets/Common/WorkspaceUtils.h"

namespace MantidQt {
namespace CustomInterfaces {
namespace Inelastic {

class IFitTab;

class MANTIDQT_INELASTIC_DLL IFitDataPresenter {
public:
  virtual ~IFitDataPresenter() = default;
  virtual std::string tabName() const = 0;

  virtual void handleAddData(MantidWidgets::IAddWorkspaceDialog const *dialog) = 0;
  virtual void handleRemoveClicked() = 0;
  virtual void handleUnifyClicked() = 0;
  virtual void handleCellChanged(int row, int column) = 0;
  virtual void handleADSDelete(const std::string &wsName) = 0;
  virtual void handleADSRename(const std::string &oldName, const std::string &newName) = 0;
  virtual void handleADSClear() = 0;
};

class MANTIDQT_INELASTIC_DLL FitDataPresenter : public IFitDataPresenter {
public:
  FitDataPresenter(IFitTab *tab, IDataModel *model, IFitDataView *view);
  ~FitDataPresenter() override = default;
  virtual bool addWorkspaceFromDialog(MantidWidgets::IAddWorkspaceDialog const *dialog);
  void addWorkspace(const std::string &workspaceName, const FunctionModelSpectra &workspaceIndices);
  void setStartX(double startX, WorkspaceID workspaceID);
  void setStartX(double startX, WorkspaceID workspaceID, WorkspaceIndex spectrum);
  void setEndX(double startX, WorkspaceID workspaceID);
  void setEndX(double startX, WorkspaceID workspaceID, WorkspaceIndex spectrum);

  std::vector<std::pair<std::string, size_t>> getResolutionsForFit() const;

  void updateTableFromModel();
  void updateTab();
  WorkspaceID getNumberOfWorkspaces() const;
  size_t getNumberOfDomains() const;
  QList<FunctionModelDataset> getDatasets() const;
  DataForParameterEstimationCollection getDataForParameterEstimation(const EstimationDataSelector &selector) const;
  std::vector<double> getQValuesForData() const;
  std::vector<std::string> createDisplayNames() const;
  void validate(IUserInputValidator *validator);

  virtual void updateFitFunctionList() const {};

  virtual void addWorkspace(const std::string &workspaceName, const std::string &paramType, const int &spectrum_index) {
    UNUSED_ARG(workspaceName);
    UNUSED_ARG(paramType);
    UNUSED_ARG(spectrum_index);
  }

  virtual void setResolution(const std::string &resName, const std::string &wsName,
                             const FunctionModelSpectra &spectra) {
    UNUSED_ARG(resName);
    UNUSED_ARG(wsName);
    UNUSED_ARG(spectra);
  }

  virtual void setActiveSpectra(std::vector<std::size_t> const &activeParameterSpectra, std::size_t parameterIndex,
                                WorkspaceID dataIndex, bool single = true) {
    UNUSED_ARG(activeParameterSpectra);
    UNUSED_ARG(parameterIndex);
    UNUSED_ARG(dataIndex);
    UNUSED_ARG(single);
  };

  std::string tabName() const override;
  std::vector<std::string> getWorkspaceNames() const;

  void handleAddData(MantidWidgets::IAddWorkspaceDialog const *dialog) override;
  void handleRemoveClicked() override;
  void handleUnifyClicked() override;
  void handleCellChanged(int row, int column) override;
  void handleADSDelete(const std::string &wsName) override;
  void handleADSRename(const std::string &oldName, const std::string &newName) override;
  void handleADSClear() override;

protected:
  IFitDataView const *getView() const;
  void displayWarning(const std::string &warning);
  virtual void addTableEntry(FitDomainIndex row);

  IFitTab *m_tab;
  IDataModel *m_model;
  IFitDataView *m_view;

private:
  void setModelStartXAndEmit(double startX, FitDomainIndex row);
  void setModelEndXAndEmit(double endX, FitDomainIndex row);
  void setTableStartXAndEmit(double X, int row, int column);
  void setTableEndXAndEmit(double X, int row, int column);
  void setModelExcludeAndEmit(const std::string &exclude, FitDomainIndex row);
  std::map<int, QModelIndex> getUniqueIndices(const QModelIndexList &selectedIndices);
  bool removeADSWorkspace(const std::string &wsName) const;
};

} // namespace Inelastic
} // namespace CustomInterfaces
} // namespace MantidQt
