// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#ifndef MANTIDQTCUSTOMINTERFACESIDA_ELWIN_H_
#define MANTIDQTCUSTOMINTERFACESIDA_ELWIN_H_

#include "IndirectDataAnalysisTab.h"
#include "MantidAPI/MatrixWorkspace_fwd.h"
#include "ui_Elwin.h"

namespace MantidQt {
namespace CustomInterfaces {
namespace IDA {
class DLLExport Elwin : public IndirectDataAnalysisTab {
  Q_OBJECT

public:
  Elwin(QWidget *parent = nullptr);

private slots:
  void newInputFiles();
  void newPreviewFileSelected(int index);
  void plotInput();
  void twoRanges(QtProperty *prop, bool /*val*/);
  void minChanged(double val);
  void maxChanged(double val);
  void updateRS(QtProperty *prop, double val);
  void unGroupInput(bool error);
  void runClicked();
  void saveClicked();
  void updateIntegrationRange();

private:
  void run() override;
  void setup() override;
  bool validate() override;
  void loadSettings(const QSettings &settings) override;
  void setFileExtensionsByName(bool filter) override;
  void setBrowserWorkspace() override{};
  void setDefaultResolution(Mantid::API::MatrixWorkspace_const_sptr ws,
                            const QPair<double, double> &range);
  void setDefaultSampleLog(Mantid::API::MatrixWorkspace_const_sptr ws);

  void checkForELTWorkspace();

  std::vector<std::string> getOutputWorkspaceNames();
  QString getOutputBasename();

  void setRunIsRunning(const bool &running);
  void setButtonsEnabled(const bool &enabled);
  void setRunEnabled(const bool &enabled);
  void setSaveResultEnabled(const bool &enabled);

  Ui::Elwin m_uiForm;
  QtTreePropertyBrowser *m_elwTree;
};
} // namespace IDA
} // namespace CustomInterfaces
} // namespace MantidQt

#endif /* MANTIDQTCUSTOMINTERFACESIDA_ELWIN_H_ */
