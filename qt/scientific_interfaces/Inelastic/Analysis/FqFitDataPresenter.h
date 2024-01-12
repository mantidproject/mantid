// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2015 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include "FqFitAddWorkspaceDialog.h"
#include "FqFitDataView.h"
#include "FunctionBrowser/SingleFunctionTemplateBrowser.h"
#include "IndirectFitDataPresenter.h"

namespace {
struct FqFitParameters {
  std::vector<std::string> widths;
  std::vector<std::size_t> widthSpectra;
  std::vector<std::string> eisf;
  std::vector<std::size_t> eisfSpectra;
};
} // namespace

namespace MantidQt {
namespace CustomInterfaces {
namespace IDA {

class MANTIDQT_INELASTIC_DLL IFqFitDataPresenter {
public:
  virtual void handleAddClicked() = 0;
  virtual void handleWorkspaceChanged(FqFitAddWorkspaceDialog *dialog, const std::string &workspace) = 0;
  virtual void handleParameterTypeChanged(FqFitAddWorkspaceDialog *dialog, const std::string &type) = 0;
};

class MANTIDQT_INELASTIC_DLL FqFitDataPresenter : public IndirectFitDataPresenter, public IFqFitDataPresenter {

public:
  FqFitDataPresenter(IIndirectDataAnalysisTab *tab, IIndirectFitDataModel *model, IIndirectFitDataView *view);
  bool addWorkspaceFromDialog(IAddWorkspaceDialog const *dialog) override;
  void addWorkspace(const std::string &workspaceName, const std::string &paramType, const int &spectrum_index) override;
  void setActiveWidth(std::size_t widthIndex, WorkspaceID dataIndex, bool single = true) override;
  void setActiveEISF(std::size_t eisfIndex, WorkspaceID dataIndex, bool single = true) override;

  void handleAddClicked() override;
  void handleWorkspaceChanged(FqFitAddWorkspaceDialog *dialog, const std::string &workspace) override;
  void handleParameterTypeChanged(FqFitAddWorkspaceDialog *dialog, const std::string &type) override;

protected:
  void addTableEntry(FitDomainIndex row) override;

private:
  void setActiveParameterType(const std::string &type);
  void updateActiveWorkspaceID(WorkspaceID index);
  void updateParameterOptions(FqFitAddWorkspaceDialog *dialog, const FqFitParameters &parameters);
  void updateParameterTypes(FqFitAddWorkspaceDialog *dialog, FqFitParameters &parameters);
  std::vector<std::string> getParameterTypes(FqFitParameters &parameters) const;
  void setActiveWorkspaceIDToCurrentWorkspace(IAddWorkspaceDialog const *dialog);

  std::string m_activeParameterType;
  WorkspaceID m_activeWorkspaceID;

  Mantid::API::AnalysisDataServiceImpl &m_adsInstance;
};

} // namespace IDA
} // namespace CustomInterfaces
} // namespace MantidQt
