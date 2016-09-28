#ifndef MANTIDQTCUSTOMINTERFACES_TOMOTOOLCONFIGSAVUDIALOG_H_
#define MANTIDQTCUSTOMINTERFACES_TOMOTOOLCONFIGSAVUDIALOG_H_

#include "ui_TomoToolConfigTomoPy.h"
#include "MantidQtCustomInterfaces/Tomography/TomoToolConfigDialogBase.h"

namespace MantidQt {
namespace CustomInterfaces {
class TomoToolConfigSavuDialog : public QMainWindow {
  Q_OBJECT
public:
  TomoToolConfigSavuDialog(QWidget *parent = 0);

private:
  void initLayout();
};

} // CustomInterfaces
} // MantidQt
#endif // MANTIDQTCUSTOMINTERFACES_TOMOTOOLCONFIGSAVUDIALOG_H_
