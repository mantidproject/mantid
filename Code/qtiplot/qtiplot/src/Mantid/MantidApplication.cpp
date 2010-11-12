//=============================
//MantidApplciation definitions
//==============================
#include "MantidApplication.h"
#include "MantidQtAPI/MantidDialog.h"

#include <QMessageBox>
#include <QPushButton>

/// The logger
Mantid::Kernel::Logger& MantidApplication::g_log = Mantid::Kernel::Logger::get("MantidPlot");

/// Constructor
MantidApplication::MantidApplication(int &argc, char ** argv ) : QApplication(argc, argv)
{
}

bool MantidApplication::notify( QObject * receiver, QEvent * event )
{
  bool res = false;
  try
  {
    res = QApplication::notify(receiver,event);
  }
  catch(std::exception& e) 
  {

      if (MantidQt::API::MantidDialog::handle(receiver,e))
          return true; // stops event propagation

    g_log.fatal()<<"Unexpected exception: "<<e.what()<<"\n";
    QMessageBox ask;
    QAbstractButton *terminateButton = ask.addButton(tr("Terminate"), QMessageBox::ActionRole);
    ask.addButton(tr("Continue"), QMessageBox::ActionRole);
    ask.setText("Sorry, MantidPlot has caught an unexpected exception:\n\n"+QString::fromStdString(e.what())+
		"\n\nWould you like to terminate MantidPlot or try to continue working?\nIf you choose to continue it is advisable to save your data and restart the application.");
    ask.setIcon(QMessageBox::Critical);
    ask.exec();
    if (ask.clickedButton() == terminateButton)
    {
        g_log.fatal("Terminated by user.");
        quit();
    }else
        g_log.fatal("Continue working.");
  }
  catch(...) 
  {

    g_log.fatal()<<"Unknown exception\n";
    QMessageBox ask;
    QAbstractButton *terminateButton = ask.addButton(tr("Terminate"), QMessageBox::ActionRole);
    ask.addButton(tr("Continue"), QMessageBox::ActionRole);
    ask.setText("Sorry, MantidPlot has caught an unexpected exception"\
		"\n\nWould you like to terminate MantidPlot or try to continue working?\nIf you choose to continue it is advisable to save your data and restart the application.");
    ask.setIcon(QMessageBox::Critical);
    ask.exec();
    if (ask.clickedButton() == terminateButton)
    {
        g_log.fatal("Terminated by user.");
        quit();
    }else
        g_log.fatal("Continue working.");
  }
  return res;
}

//=============================================================================
