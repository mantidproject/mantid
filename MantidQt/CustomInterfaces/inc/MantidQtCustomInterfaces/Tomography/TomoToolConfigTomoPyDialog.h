#ifndef MANTIDQTCUSTOMINTERFACES_TOMOTOOLCONFIGTOMOPYDIALOG_H_
#define MANTIDQTCUSTOMINTERFACES_TOMOTOOLCONFIGTOMOPYDIALOG_H_

#include "ui_TomoToolConfigTomoPy.h"
#include "MantidQtCustomInterfaces/Tomography/TomoToolConfigDialogBase.h"

namespace MantidQt {
namespace CustomInterfaces {
class TomoToolConfigTomoPyDialog : public TomoToolConfigDialogBase {
  Q_OBJECT

public:
  TomoToolConfigTomoPyDialog(QWidget *parent = 0);
  void setUpDialog() override;
  int execute() override;

private:
  Ui::TomoToolConfigTomoPy m_tomoPyUi;
};
} // CustomInterfaces
} // MantidQt
#endif // MANTIDQTCUSTOMINTERFACES_TOMOTOOLCONFIGTOMOPYDIALOG_H_
