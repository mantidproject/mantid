#ifndef MANTIDQTCUSTOMINTERFACES_TOMOTOOLCONFIGCUSTOMDIALOG_H_
#define MANTIDQTCUSTOMINTERFACES_TOMOTOOLCONFIGCUSTOMDIALOG_H_

#include "ui_TomoToolConfigCustom.h"
#include "MantidQtCustomInterfaces/Tomography/TomoToolConfigDialogBase.h"

namespace MantidQt {
namespace CustomInterfaces {
class MANTIDQT_CUSTOMINTERFACES_DLL TomoToolConfigCustomDialog
    : public TomoToolConfigDialogBase {
public:
  TomoToolConfigCustomDialog()
      : TomoToolConfigDialogBase(DEFAULT_TOOL_NAME, DEFAULT_TOOL_METHOD) {}
  ~TomoToolConfigCustomDialog() override {
    if (m_dialog) {
      delete m_dialog;
    }
  }

private:
  void initialiseDialog() override;
  void setupMethodSelected() override;
  void setupToolSettingsFromPaths() override;  
  void setupDialogUi() override;
  void handleDialogResult(int result) override;
  int executeQt() override;


  // initialised in .cpp file
  static const std::string DEFAULT_TOOL_NAME;
  static const std::string DEFAULT_TOOL_METHOD;
  static std::string m_backupCommandLine;

  QDialog *m_dialog = nullptr;
  Ui::TomoToolConfigCustom m_customUi;
};
} // CustomInterfaces
} // MantidQt
#endif // MANTIDQTCUSTOMINTERFACES_TOMOTOOLCONFIGCUSTOMDIALOG_H_
