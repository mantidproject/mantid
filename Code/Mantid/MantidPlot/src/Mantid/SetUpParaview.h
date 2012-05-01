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
  enum StartUpFrom {FirstLaunch, MantidMenu};
  SetUpParaview(StartUpFrom from, QWidget *parent=0);
  ~SetUpParaview();
private:
  void initLayout();
  void clearStatus();
  void writeError(const QString& error);
  void acceptPotentialLocation(const QString& location);
  void rejectPotentialLocation(const QString& location);
private slots:
  void onChoose();
  void onSet();
  void onHelp();
  void onIgnoreHenceforth();
private:
  Ui::SetUpParaview m_uiForm;
  QString m_candidateLocation;
  StartUpFrom m_from;

};

#endif