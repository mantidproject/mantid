#ifndef MANTIDQTCUSTOMINTERFACES_TOMOTOOLCONFIGTOMOPYDIALOG_H_
#define MANTIDQTCUSTOMINTERFACES_TOMOTOOLCONFIGTOMOPYDIALOG_H_

#include "ui_TomoToolConfigTomoPy.h"
#include "MantidQtCustomInterfaces/Tomography/TomoToolConfigDialogBase.h"

namespace MantidQt {
namespace CustomInterfaces {
class MANTIDQT_CUSTOMINTERFACES_DLL TomoToolConfigDialogTomoPy
    : public TomoToolConfigDialogBase {

public:
  TomoToolConfigDialogTomoPy()
      : TomoToolConfigDialogBase(DEFAULT_TOOL_NAME, DEFAULT_TOOL_METHOD) {}

  ~TomoToolConfigDialogTomoPy() override {
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
  Ui::TomoToolConfigTomoPy m_tomoPyUi;
};
} // CustomInterfaces
} // MantidQt
#endif // MANTIDQTCUSTOMINTERFACES_TOMOTOOLCONFIGTOMOPYDIALOG_H_
