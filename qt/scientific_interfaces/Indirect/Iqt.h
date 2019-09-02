// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#ifndef MANTIDQTCUSTOMINTERFACESIDA_IQT_H_
#define MANTIDQTCUSTOMINTERFACESIDA_IQT_H_

#include "IndirectDataAnalysisTab.h"
#include "ui_Iqt.h"

namespace MantidQt {
namespace CustomInterfaces {
namespace IDA {
class DLLExport Iqt : public IndirectDataAnalysisTab {
  Q_OBJECT

public:
  Iqt(QWidget *parent = nullptr);

private:
  void run() override;
  void setup() override;
  bool validate() override;
  void loadSettings(const QSettings &settings) override;
  void setFileExtensionsByName(bool filter) override;
  void setBrowserWorkspace() override{};

  bool isErrorsEnabled();

  void setRunEnabled(bool enabled);
  void setSaveResultEnabled(bool enabled);
  void setButtonsEnabled(bool enabled);
  void setRunIsRunning(bool running);

private slots:
  void algorithmComplete(bool error);
  void plotInput();
  void plotInput(const QString &wsname);
  void rsRangeChangedLazy(double min, double max);
  void updateRS(QtProperty *prop, double val);
  void updatePropertyValues(QtProperty *prop, double val);
  void updateDisplayedBinParameters();
  void runClicked();
  void saveClicked();
  void errorsClicked();
  void updateEnergyRange(int state);

private:
  void setPreviewSpectrumMaximum(int value);

  Ui::Iqt m_uiForm;
  QtTreePropertyBrowser *m_iqtTree;
  bool m_iqtResFileType;
};
} // namespace IDA
} // namespace CustomInterfaces
} // namespace MantidQt

#endif /* MANTIDQTCUSTOMINTERFACESIDA_IQT_H_ */
