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
  bool validateDialogInput(QString input);
  void enableElementsFromString(QString elementToEnable);
  void enableElementsForGetNegMuMuonicXRD(PeriodicTableWidget *periodicTable);
  private slots:
  void runClicked(PeriodicTableWidget *periodicTable, QLineEdit *yPosition);
signals:
  void validInput();
  protected:
  void initLayout();    
  };
  }
}
#endif // !MANTIDQT_CUSTOM_DIALOGS_GETNEGMUMUONICXRD_H_
