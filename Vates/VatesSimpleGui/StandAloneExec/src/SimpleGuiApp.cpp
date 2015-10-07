#include "SimpleGuiApp.h"
#include "MantidKernel/Logger.h"

#include <pqPVApplicationCore.h>

#include <QMessageBox>
#include <QPushButton>

#include <exception>
#include <iostream>

namespace
{
  // static logger
  Mantid::Kernel::Logger g_log("VatesSimpleGui");
}

SimpleGuiApp::SimpleGuiApp(int &argc, char **argv) : QApplication(argc, argv)
{
  this->pvApp = new pqPVApplicationCore(argc, argv);
}

bool SimpleGuiApp::notify(QObject *receiver, QEvent *event)
{
  bool res = false;
  try
  {
    res = QApplication::notify(receiver, event);
  }
  catch (std::exception &e)
  {
    // Restore possible override cursor
    while(QApplication::overrideCursor())
    {
      QApplication::restoreOverrideCursor();
    }

    g_log.fatal() << "Unexpected exception: " << e.what() << "\n";
    QMessageBox ask;
    QAbstractButton *terminateButton = ask.addButton(tr("Terminate"),
                                                     QMessageBox::ActionRole);
    ask.addButton(tr("Continue"), QMessageBox::ActionRole);
    ask.setText("Sorry, VatesSimpleGui has caught an unexpected exception:\n\n"\
                +QString::fromStdString(e.what())+
                "\n\nWould you like to terminate VatesSimpleGui or try to "\
                "continue working?\n\nIf you choose to continue it is advisable "\
                "to save your data and restart the application.");
    ask.setIcon(QMessageBox::Critical);
    ask.exec();
    if (ask.clickedButton() == terminateButton)
    {
        g_log.fatal("Terminated by user.");
        quit();
    }
  }
  catch (...)
  {
    g_log.fatal() << "Unknown exception\n";
    QMessageBox ask;
    QAbstractButton *terminateButton = ask.addButton(tr("Terminate"),
                                                     QMessageBox::ActionRole);
    ask.addButton(tr("Continue"), QMessageBox::ActionRole);
    ask.setText("Sorry, VatesSimpleGui has caught an unexpected exception\n\n"\
                "Would you like to terminate VatesSimpleGui or try to continue "\
                "working?\n\nIf you choose to continue it is advisable to save "\
                "your data and restart the application.");
    ask.setIcon(QMessageBox::Critical);
    ask.exec();
    if (ask.clickedButton() == terminateButton)
    {
       g_log.fatal("Terminated by user.");
       quit();
    }
  }

  return res;
}
