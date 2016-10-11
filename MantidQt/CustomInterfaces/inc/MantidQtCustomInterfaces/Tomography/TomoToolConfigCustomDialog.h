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
  TomoToolConfigCustomDialog(QWidget *parent = 0)
      : QDialog(parent), TomoToolConfigDialogBase() {}

private:
  void setupToolConfig() override;
  void setupDialogUi() override;
  int executeQt() override;

  // initialised in .cpp file
  static const std::string DEFAULT_TOOL_NAME;
  static const std::string DEFAULT_TOOL_METHOD;

  Ui::TomoToolConfigCustom m_customUi;
};
} // CustomInterfaces
} // MantidQt
#endif // MANTIDQTCUSTOMINTERFACES_TOMOTOOLCONFIGCUSTOMDIALOG_H_
