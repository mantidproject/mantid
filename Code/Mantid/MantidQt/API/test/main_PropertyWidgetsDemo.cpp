
#include "MantidKernel/Logger.h"
#include "MantidKernel/MantidVersion.h"
#include "MantidKernel/PropertyWithValue.h"
#include "qmainwindow.h"
#include <iostream>
#include <QApplication>
#include <QDir>
#include <QFrame>
#include <qlayout.h>
#include <QMessageBox>
#include <QSplashScreen>
#include <QThread>
#include "MantidQtAPI/BoolPropertyWidget.h"
#include "MantidQtAPI/TextPropertyWidget.h"
#include "MantidQtAPI/PropertyWidgetFactory.h"
#include "MantidKernel/ListValidator.h"
#include "MantidAPI/FileProperty.h"
#include "MantidAPI/MultipleFileProperty.h"

using namespace Mantid::Kernel;
using namespace MantidQt::API;
using Mantid::API::FileProperty;
using Mantid::API::MultipleFileProperty;

/** Main application
 *
 * @param argc :: ignored
 * @param argv :: ignored
 * @return return code
 */
int main( int argc, char ** argv )
{
  QApplication app(argc, argv);
  app.setApplicationName("PropertyWidgets demo");
  QMainWindow * mainWin = new QMainWindow();

  QFrame * frame = new QFrame(mainWin);
  mainWin->setCentralWidget(frame);

  QHBoxLayout * layout = new QHBoxLayout(frame);
  frame->setLayout(layout);

  QFrame * frame1 = new QFrame(mainWin);
  frame1->setFrameStyle(QFrame::Box);
  QFrame * frame2 = new QFrame(mainWin);
  frame2->setFrameStyle(QFrame::Box);

  layout->addWidget(frame1);
  layout->addWidget(frame2);

  QVBoxLayout * layout1 = new QVBoxLayout(frame1);
  QGridLayout * grid = new QGridLayout(frame2);

  std::vector<Property*> props;
  props.push_back(new PropertyWithValue<bool>("BooleanProp", true));
  props.push_back(new PropertyWithValue<std::string>("StringProperty", "default value"));

  std::vector<std::string> exts;
  exts.push_back(".txt");
  exts.push_back(".nxs");
  props.push_back(new FileProperty("SaveFileProperty", "default.file.txt", FileProperty::Save, exts));
  props.push_back(new FileProperty("LoadFileProperty", "default.file.txt", FileProperty::Load, exts));
  props.push_back(new FileProperty("DirectoryFileProperty", "default.file.txt", FileProperty::Directory, exts));
  props.push_back(new MultipleFileProperty("MultipleFileProperty", exts));

  std::vector<std::string> propOptions;
  propOptions.push_back("OptionA");
  propOptions.push_back("OptionTwo");
  propOptions.push_back("Yet Another Option");
  props.push_back(new PropertyWithValue<std::string>("OptionsProperty", "OptionTwo", new ListValidator(propOptions)));

  for (size_t i=0; i<props.size(); i++)
  {
    PropertyWidget * widget1 = PropertyWidgetFactory::createWidget(props[i], frame1, NULL);
    layout1->addWidget(widget1);
    PropertyWidget * widget2 = PropertyWidgetFactory::createWidget(props[i], frame2, grid, int(i));
    UNUSED_ARG(widget2);
  }

  mainWin->move(100, 100);
  mainWin->resize(700, 700);
  mainWin->show();

  app.exec();

  mainWin->close();
  delete mainWin;
  return 0;
}
