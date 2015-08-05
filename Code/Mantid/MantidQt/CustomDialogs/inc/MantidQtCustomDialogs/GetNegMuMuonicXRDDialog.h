#ifndef MANTIDQT_CUSTOM_DIALOGS_GETNEGMUMUONICXRD_H_
#define MANTIDQT_CUSTOM_DIALOGS_GETNEGMUMUONICXRD_H_

#include "MantidQtAPI/AlgorithmDialog.h"
namespace MantidQt {

namespace CustomDialogs {

  class GetNegMuMuonicXRDDialog : public API::AlgorithmDialog {
  Q_OBJECT

  public:
  GetNegMuMuonicXRDDialog(QWidget *parent = 0);
  ~GetNegMuMuonicXRDDialog(){};
  private:
  bool validateDialogInput(QString input);
  void enableElementsFromString(QString elementToEnable);
  void enableElementsForGetNegMuMuonicXRD();
  //void setEnabledElementsInPeriodicTable()
  private slots:
  //void SelectElementsClicked();
  void runClicked();
signals:
  void validInput();
  protected:
  void initLayout();    };
  }
}
#endif // !MANTIDQT_CUSTOM_DIALOGS_GETNEGMUMUONICXRD_H_
