#ifndef MANTIDQT_CUSTOM_DIALOGS_GETNEGMUMUONICXRD_H_
#define MANTIDQT_CUSTOM_DIALOGS_GETNEGMUMUONICXRD_H_

#include "MantidQtAPI/AlgorithmDialog.h"
#include "MantidQtMantidWidgets/PeriodicTableWidget.h"

namespace MantidQt {

namespace CustomDialogs {

  class GetNegMuMuonicXRDDialog : public API::AlgorithmDialog {
  Q_OBJECT

  public:
  GetNegMuMuonicXRDDialog(QWidget *parent = 0);
  ~GetNegMuMuonicXRDDialog(){};
  private:
  PeriodicTableWidget *periodicTable;
  QLineEdit *yPosition;
  bool validateDialogInput(QString input);
  void enableElementsFromString(QString elementToEnable);
  void enableElementsForGetNegMuMuonicXRD();
  private slots:
  void runClicked();
signals:
  void validInput();
  protected:
  void initLayout();    
  };
  }
}
#endif // !MANTIDQT_CUSTOM_DIALOGS_GETNEGMUMUONICXRD_H_
