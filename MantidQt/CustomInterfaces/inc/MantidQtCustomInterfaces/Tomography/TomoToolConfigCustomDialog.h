#ifndef MANTIDQTCUSTOMINTERFACES_TOMOTOOLCONFIGCUSTOMDIALOG_H_
#define MANTIDQTCUSTOMINTERFACES_TOMOTOOLCONFIGCUSTOMDIALOG_H_

#include "ui_TomoToolConfigCustom.h"
#include "MantidQtCustomInterfaces/Tomography/TomoToolConfigDialogBase.h"

namespace MantidQt {
namespace CustomInterfaces {
class TomoToolConfigCustomDialog : public QDialog,
                                   public TomoToolConfigDialogBase {
  Q_OBJECT
public:
  TomoToolConfigCustomDialog(QWidget *parent = 0);
  ~TomoToolConfigCustomDialog() override;

private:
  void setupToolConfig() override;
  void setupDialogUi() override;
  int executeQt() override;

  Ui::TomoToolConfigCustom m_customUi;
};
} // CustomInterfaces
} // MantidQt
#endif // MANTIDQTCUSTOMINTERFACES_TOMOTOOLCONFIGCUSTOMDIALOG_H_
