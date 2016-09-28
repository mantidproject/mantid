#ifndef MANTIDQTCUSTOMINTERFACES_TOMOTOOLCONFIGCUSTOMDIALOG_H_
#define MANTIDQTCUSTOMINTERFACES_TOMOTOOLCONFIGCUSTOMDIALOG_H_

#include "ui_TomoToolConfigTomoPy.h"
#include "MantidQtCustomInterfaces/Tomography/TomoToolConfigDialogBase.h"

namespace MantidQt {
namespace CustomInterfaces {
class TomoToolConfigCustomDialog : public TomoToolConfigDialogBase {
  Q_OBJECT
public:
  TomoToolConfigCustomDialog(QWidget *parent = 0);
  virtual void setUpDialog() override {
	  std::cout << "TODO" << std::endl;
	  //TODO
  }


private:
  void initLayout();
};
} // CustomInterfaces
} // MantidQt
#endif // MANTIDQTCUSTOMINTERFACES_TOMOTOOLCONFIGCUSTOMDIALOG_H_
