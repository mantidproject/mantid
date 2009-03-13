//=============================
//MantidApplciation definitions
//==============================
#include "MantidApplication.h"

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
    g_log.error()<<"Error in an event handler: "<<e.what()<<"\n";
    QMessageBox ask;
    QAbstractButton *terminateButton = ask.addButton(tr("Terminate"), QMessageBox::ActionRole);
    ask.addButton(tr("Continue"), QMessageBox::ActionRole);
    ask.setText("An exception is caught in an event handler:\n\n"+QString::fromStdString(e.what())+
		"\n\nWould you like to terminate MantidPlot or continue working?");
    ask.setIcon(QMessageBox::Critical);
    ask.exec();
    if (ask.clickedButton() == terminateButton)
    {
      exit(-1);
    }
  }
  return res;
}

//=============================================================================
