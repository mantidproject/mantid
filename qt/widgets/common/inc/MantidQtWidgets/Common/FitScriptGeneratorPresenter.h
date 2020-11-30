// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2020 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include "DllOption.h"
#include "FitScriptGeneratorView.h"

#include "MantidAPI/MatrixWorkspace_fwd.h"

#include <string>
#include <vector>

#include <QStringList>

namespace MantidQt {
namespace MantidWidgets {

using ViewEvent = FitScriptGeneratorView::Event;

class FitScriptGeneratorModel;

class EXPORT_OPT_MANTIDQT_COMMON FitScriptGeneratorPresenter {
public:
  FitScriptGeneratorPresenter(FitScriptGeneratorView *view,
                              FitScriptGeneratorModel *model,
                              QStringList const &workspaceNames = QStringList(),
                              double startX = 0.0, double endX = 0.0);
  ~FitScriptGeneratorPresenter();

  void notifyPresenter(ViewEvent const &event);

  void openFitScriptGenerator();

private:
  void handleRemoveClicked();
  void handleAddWorkspaceClicked();

  void setWorkspaces(QStringList const &workspaceNames, double startX,
                     double endX);
  void addWorkspace(std::string const &workspaceName, double startX,
                    double endX);
  void addWorkspace(MatrixWorkspace_const_sptr const &workspace, double startX,
                    double endX);

  FitScriptGeneratorModel *m_model;
  FitScriptGeneratorView *m_view;
};

} // namespace MantidWidgets
} // namespace MantidQt
