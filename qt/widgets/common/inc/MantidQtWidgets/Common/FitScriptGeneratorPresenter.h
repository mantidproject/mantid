// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2020 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include "DllOption.h"
#include "MantidAPI/MatrixWorkspace_fwd.h"
#include "MantidQtWidgets/Common/IFitScriptGeneratorPresenter.h"
#include "MantidQtWidgets/Common/IndexTypes.h"

#include <string>
#include <vector>

#include <QStringList>

namespace MantidQt {
namespace MantidWidgets {

class IFitScriptGeneratorModel;
class IFitScriptGeneratorView;

class EXPORT_OPT_MANTIDQT_COMMON FitScriptGeneratorPresenter
    : public IFitScriptGeneratorPresenter {
public:
  FitScriptGeneratorPresenter(IFitScriptGeneratorView *view,
                              IFitScriptGeneratorModel *model,
                              QStringList const &workspaceNames = QStringList(),
                              double startX = 0.0, double endX = 0.0);
  ~FitScriptGeneratorPresenter() override;

  void notifyPresenter(ViewEvent const &event,
                       std::string const &arg = "") override;

  void openFitScriptGenerator() override;

private:
  void handleRemoveClicked();
  void handleAddWorkspaceClicked();
  void handleStartXChanged();
  void handleEndXChanged();
  void handleSelectionChanged();
  void handleFunctionAdded(std::string const &function);
  void handleFunctionRemoved(std::string const &function);
  void handleParameterChanged(std::string const &parameter);

  void setWorkspaces(QStringList const &workspaceNames, double startX,
                     double endX);
  void addWorkspaces(
      std::vector<Mantid::API::MatrixWorkspace_const_sptr> const &workspaces,
      std::vector<WorkspaceIndex> const &workspaceIndices);
  void addWorkspace(std::string const &workspaceName, double startX,
                    double endX);
  void addWorkspace(Mantid::API::MatrixWorkspace_const_sptr const &workspace,
                    double startX, double endX);
  void addWorkspace(Mantid::API::MatrixWorkspace_const_sptr const &workspace,
                    WorkspaceIndex workspaceIndex, double startX, double endX);
  void addWorkspace(std::string const &workspaceName,
                    WorkspaceIndex workspaceIndex, double startX, double endX);

  void updateStartX(std::string const &workspaceName,
                    WorkspaceIndex workspaceIndex, double startX);
  void updateEndX(std::string const &workspaceName,
                  WorkspaceIndex workspaceIndex, double endX);

  void checkForWarningMessages();

  std::vector<std::string> m_warnings;

  IFitScriptGeneratorView *m_view;
  IFitScriptGeneratorModel *m_model;
};

} // namespace MantidWidgets
} // namespace MantidQt
