#ifndef MANTIDQTCUSTOMINTERFACES_TOMOTOOLCONFIGASTRADIALOG_H_
#define MANTIDQTCUSTOMINTERFACES_TOMOTOOLCONFIGASTRADIALOG_H_

#include "ui_TomoToolConfigAstra.h"
#include "MantidQtCustomInterfaces/Tomography/TomoToolConfigDialogBase.h"

namespace MantidQt {
namespace CustomInterfaces {
class MANTIDQT_CUSTOMINTERFACES_DLL TomoToolConfigAstraDialog
    : public TomoToolConfigDialogBase {
public:
  TomoToolConfigAstraDialog()
      : TomoToolConfigDialogBase(DEFAULT_TOOL_NAME, DEFAULT_TOOL_METHOD) {}

  ~TomoToolConfigAstraDialog() override {
    if (m_dialog) {
      delete m_dialog;
    }
  }

private:
  void initialiseDialog() override;
  void setupMethodSelected() override;
  void setupToolSettingsFromPaths() override;  
  void setupDialogUi() override;
  int executeQt() override;

  // initialised in .cpp file
  static const std::string DEFAULT_TOOL_NAME;
  static const std::string DEFAULT_TOOL_METHOD;

  QDialog *m_dialog = nullptr;
  Ui::TomoToolConfigAstra m_astraUi;
};

} // CustomInterfaces
} // MantidQt
#endif // MANTIDQTCUSTOMINTERFACES_TOMOTOOLCONFIGASTRADIALOG_H_
