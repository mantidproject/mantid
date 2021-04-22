// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include "IndirectDataAnalysisTab.h"
#include "MantidAPI/MatrixWorkspace_fwd.h"
#include "ui_IndirectDataAnalysisElwinTab.h"

namespace MantidQt {
namespace CustomInterfaces {
namespace IDA {
class DLLExport IndirectDataAnalysisElwinTab : public IndirectDataAnalysisTab {
  Q_OBJECT

public:
  IndirectDataAnalysisElwinTab(QWidget *parent = nullptr);
  ~IndirectDataAnalysisElwinTab();

private:
  void run() override;
  void setup() override;
  bool validate() override;
  void loadSettings(const QSettings &settings) override;
  void setFileExtensionsByName(bool filter) override;
  void setBrowserWorkspace() override{};
  void setDefaultResolution(const Mantid::API::MatrixWorkspace_const_sptr &ws, const QPair<double, double> &range);
  void setDefaultSampleLog(const Mantid::API::MatrixWorkspace_const_sptr &ws);

  void checkForELTWorkspace();

  std::vector<std::string> getOutputWorkspaceNames();
  QString getOutputBasename();

  void setRunIsRunning(const bool &running);
  void setButtonsEnabled(const bool &enabled);
  void setRunEnabled(const bool &enabled);
  void setSaveResultEnabled(const bool &enabled);

  Ui::IndirectDataAnalysisElwinTab m_uiForm;
  QtTreePropertyBrowser *m_elwTree;

private slots:
  void newInputFiles();
  void newPreviewFileSelected(int index);
  void plotInput();
  void handlePreviewSpectrumChanged();
  void twoRanges(QtProperty *prop, bool enabled);
  void minChanged(double val);
  void maxChanged(double val);
  void updateRS(QtProperty *prop, double val);
  void unGroupInput(bool error);
  void runClicked();
  void saveClicked();
  void updateIntegrationRange();
};
} // namespace IDA
} // namespace CustomInterfaces
} // namespace MantidQt
