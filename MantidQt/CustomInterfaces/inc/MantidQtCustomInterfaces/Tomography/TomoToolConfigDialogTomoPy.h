#ifndef MANTIDQTCUSTOMINTERFACES_TOMOTOOLCONFIGDIALOGTOMOPY_H_
#define MANTIDQTCUSTOMINTERFACES_TOMOTOOLCONFIGDIALOGTOMOPY_H_

#include "ui_TomoToolConfigTomoPy.h"
#include "MantidQtCustomInterfaces/Tomography/TomoToolConfigDialogBase.h"

namespace MantidQt {
namespace CustomInterfaces {
class TomoToolConfigDialogTomoPy : public TomoToolConfigDialogBase {

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
  std::vector<std::pair<std::string, std::string>> getToolMethods() override;
  // initialised in .cpp file
  static const std::string DEFAULT_TOOL_NAME;
  static const std::string DEFAULT_TOOL_METHOD;

  QDialog *m_dialog = nullptr;
  Ui::TomoToolConfigTomoPy m_tomoPyUi;
};
} // CustomInterfaces
} // MantidQt
#endif // MANTIDQTCUSTOMINTERFACES_TOMOTOOLCONFIGDIALOGTOMOPY_H_
