#include "MantidAPI/Algorithm.h"
#include "MantidAPI/AlgorithmFactory.h"
#include "MantidAPI/AlgorithmManager.h"
#include "MantidAPI/FileProperty.h"
#include "MantidAPI/FrameworkManager.h"
#include "MantidAPI/MultipleFileProperty.h"
#include "MantidAPI/WorkspaceProperty.h"
#include "MantidKernel/ListValidator.h"
#include "MantidKernel/Logger.h"
#include "MantidKernel/MantidVersion.h"
#include "MantidKernel/PropertyWithValue.h"
#include "MantidQtAPI/BoolPropertyWidget.h"
#include "MantidQtAPI/GenericDialog.h"
#include "MantidQtAPI/PropertyWidgetFactory.h"
#include "MantidQtAPI/TextPropertyWidget.h"
#include "qmainwindow.h"
#include <iostream>
#include <QApplication>
#include <QDir>
#include <QFrame>
#include <qlayout.h>
#include <QMessageBox>
#include <QSplashScreen>
#include <QtCore/qstring.h>
#include <QThread>
#include <QtCore/qstringlist.h>

using namespace Mantid::Kernel;
using namespace Mantid::API;
using namespace MantidQt::API;

/** This application will be used for debugging and testing the
 * GenericDialog and the AlgorithmPropertiesWidget.
 */
int main( int argc, char ** argv )
{
  QApplication app(argc, argv);
  app.setApplicationName("PropertyWidgets demo");

  GenericDialog * dlg = new GenericDialog(NULL);


  FrameworkManager::Instance();

  // Create the algorithm using the argument, with a default
  std::string algo = "LoadEventNexus";
  if (argc > 1)
    algo = std::string(argv[1]);
  IAlgorithm_sptr alg = AlgorithmManager::Instance().create(algo);

  // Set the content
  dlg->setAlgorithm(alg.get());
  //dlg->setPresetValues(preset_values);
  //dlg->isForScript(forScript);
  dlg->setOptionalMessage(QString::fromStdString(alg->getOptionalMessage()));

  QStringList enabled;
  if (argc > 2) enabled = QStringList::split(",", argv[2], false);
  QStringList disabled;
  if (argc > 3) disabled = QStringList::split(",", argv[3], false);
  dlg->addEnabledAndDisableLists(enabled, disabled);

  dlg->showHiddenWorkspaces(false);

  // Setup the layout
  dlg->initializeLayout();

  // Show dialog
  dlg->show();

  app.exec();

  dlg->close();
  delete dlg;
  return 0;
}
