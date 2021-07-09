// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include "IndirectDataAnalysisTab.h"
#include "ui_IndirectDataAnalysisIqtTab.h"

namespace MantidQt {
namespace CustomInterfaces {
namespace IDA {
class DLLExport IndirectDataAnalysisIqtTab : public IndirectDataAnalysisTab {
  Q_OBJECT

public:
  IndirectDataAnalysisIqtTab(QWidget *parent = nullptr);
  ~IndirectDataAnalysisIqtTab();

private:
  void run() override;
  void setup() override;
  bool validate() override;
  void loadTabSettings(const QSettings &settings);
  void setFileExtensionsByName(bool filter) override;

  bool isErrorsEnabled();

  void setRunEnabled(bool enabled);
  void setSaveResultEnabled(bool enabled);
  void setButtonsEnabled(bool enabled);
  void setRunIsRunning(bool running);
  void setPreviewSpectrumMaximum(int value);

  Ui::IndirectDataAnalysisIqtTab m_uiForm;
  QtTreePropertyBrowser *m_iqtTree;
  bool m_iqtResFileType;

private slots:
  void algorithmComplete(bool error);
  void plotInput();
  void plotInput(const QString &wsname);
  void rangeChanged(double min, double max);
  void updateRangeSelector(QtProperty *prop, double val);
  void updateDisplayedBinParameters();
  void runClicked();
  void saveClicked();
  void errorsClicked();
  void updateEnergyRange(int state);
};
} // namespace IDA
} // namespace CustomInterfaces
} // namespace MantidQt
