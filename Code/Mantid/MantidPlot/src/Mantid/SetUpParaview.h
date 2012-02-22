#ifndef MANTIDPLOT_SETUP_PARAVIEW_WINDOW
#define MANTIDPLOT_SETUP_PARAVIEW_WINDOW

#include <QDialog>
#include "ui_SetUpParaview.h"

/**
* SetUpParaview dialog for MantidPlot.
*
*/

class SetUpParaview : public QDialog
{
  Q_OBJECT

public:
  SetUpParaview(QWidget *parent=0);
  ~SetUpParaview();
private:
  void initLayout();
  void clearStatus();
  void writeError(const QString& error);
private slots:
  void onChoose();
  void onSet();
  void onHelp();
private:
  Ui::SetUpParaview m_uiForm;
  QString m_candidateLocation;

};

#endif