#ifndef MANTIDQTCUSTOMINTERFACES_TOMOTOOLCONFIGTOMOPYDIALOG_H_
#define MANTIDQTCUSTOMINTERFACES_TOMOTOOLCONFIGTOMOPYDIALOG_H_

#include "ui_TomoToolConfigTomoPy.h"
#include "MantidQtCustomInterfaces/Tomography/TomoToolConfigDialogBase.h"

namespace MantidQt {
namespace CustomInterfaces {
class MANTIDQT_CUSTOMINTERFACES_DLL TomoToolConfigTomoPyDialog : public QDialog,
                                   public TomoToolConfigDialogBase {
  Q_OBJECT

public:
  TomoToolConfigTomoPyDialog(QWidget *parent = 0)
      : QDialog(parent),
        TomoToolConfigDialogBase(DEFAULT_TOOL_NAME, DEFAULT_TOOL_METHOD) {}

private:
  void setupToolConfig() override;
  void setupDialogUi() override;
  int executeQt() override;

  // initialised in .cpp file
  static const std::string DEFAULT_TOOL_NAME;
  static const std::string DEFAULT_TOOL_METHOD;

  Ui::TomoToolConfigTomoPy m_tomoPyUi;
};
} // CustomInterfaces
} // MantidQt
#endif // MANTIDQTCUSTOMINTERFACES_TOMOTOOLCONFIGTOMOPYDIALOG_H_
