#ifndef MANTIDQTCUSTOMINTERFACES_TOMOTOOLCONFIGASTRADIALOG_H_
#define MANTIDQTCUSTOMINTERFACES_TOMOTOOLCONFIGASTRADIALOG_H_

#include "ui_TomoToolConfigAstra.h"
#include "MantidQtCustomInterfaces/Tomography/TomoToolConfigDialogBase.h"

namespace MantidQt {
namespace CustomInterfaces {
class TomoToolConfigDialogAstra
    : public TomoToolConfigDialogBase {
public:
  TomoToolConfigDialogAstra()
      : TomoToolConfigDialogBase(DEFAULT_TOOL_NAME, DEFAULT_TOOL_METHOD) {}

  ~TomoToolConfigDialogAstra() override {
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
