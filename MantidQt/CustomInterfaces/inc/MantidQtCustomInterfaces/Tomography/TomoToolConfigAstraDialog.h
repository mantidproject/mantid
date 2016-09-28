#ifndef MANTIDQTCUSTOMINTERFACES_TOMOTOOLCONFIGASTRADIALOG_H_
#define MANTIDQTCUSTOMINTERFACES_TOMOTOOLCONFIGASTRADIALOG_H_

#include "ui_TomoToolConfigTomoPy.h"
#include "MantidQtCustomInterfaces/Tomography/TomoToolConfigDialogBase.h"

namespace MantidQt {
namespace CustomInterfaces {
class TomoToolConfigAstraDialog : public TomoToolConfigDialogBase {
  Q_OBJECT
public:
  TomoToolConfigAstraDialog(QWidget *parent = 0);
  virtual void setUpDialog() override {
	  std::cout << "TODO" << std::endl;
	  // TODO
  }
private:
  void initLayout();
};

} // CustomInterfaces
} // MantidQt
#endif // MANTIDQTCUSTOMINTERFACES_TOMOTOOLCONFIGASTRADIALOG_H_
