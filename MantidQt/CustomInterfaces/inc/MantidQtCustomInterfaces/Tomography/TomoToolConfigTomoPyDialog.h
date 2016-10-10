#ifndef MANTIDQTCUSTOMINTERFACES_TOMOTOOLCONFIGTOMOPYDIALOG_H_
#define MANTIDQTCUSTOMINTERFACES_TOMOTOOLCONFIGTOMOPYDIALOG_H_

#include "ui_TomoToolConfigTomoPy.h"
#include "MantidQtCustomInterfaces/Tomography/TomoToolConfigDialogBase.h"

namespace MantidQt {
namespace CustomInterfaces {
class TomoToolConfigTomoPyDialog : public QDialog,
                                   public TomoToolConfigDialogBase {
  Q_OBJECT

public:
  TomoToolConfigTomoPyDialog(QWidget *parent = 0)
	  : QDialog(parent), TomoToolConfigDialogBase(DEFAULT_TOOL_METHOD) {}
  ~TomoToolConfigTomoPyDialog() override {}

private:
  void setupToolConfig() override;
  void setupDialogUi() override;
  int executeQt() override;

  const std::string DEFAULT_TOOL_NAME = "TomoPy";
  const std::string DEFAULT_TOOL_METHOD = "gridrec";

  Ui::TomoToolConfigTomoPy m_tomoPyUi;
};
} // CustomInterfaces
} // MantidQt
#endif // MANTIDQTCUSTOMINTERFACES_TOMOTOOLCONFIGTOMOPYDIALOG_H_
