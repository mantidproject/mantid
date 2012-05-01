#include "SetUpParaview.h"
#include "MantidKernel/ConfigService.h"
#include "MantidQtAPI/ManageUserDirectories.h"
#include "MantidKernel/ConfigService.h"
#include <boost/regex.hpp>
#include <QFileDialog>
#include <QProcess>
#include <QPalette>
#include <QDirIterator>
#include <QDesktopServices>
#include <iostream> 


using Mantid::Kernel::ConfigService;
using Mantid::Kernel::ConfigServiceImpl;

/**
Is Paraview at this location.
@return TRUE if determined to be present.
*/
bool isParaviewHere(const QString& location)
{
  using boost::regex;
  using boost::regex_search;

  bool found = false;
  if(!location.isEmpty())
  {
    QDirIterator it(location, QDirIterator::NoIteratorFlags);
    while (it.hasNext())
    {
      it.next();
      QString file =it.fileName();
      regex expression("^(pqcore)", boost::regex::icase);
      if(regex_search(file.toStdString(), expression) && it.fileInfo().isFile())
      {
        found = true;
        break;
      }
    }
  }
  return found;
}

/// Constructor
SetUpParaview::SetUpParaview(StartUpFrom from, QWidget *parent) : QDialog(parent), m_from(from)
{
  m_uiForm.setupUi(this);

  initLayout();

  //Do our best to figure out the location based on where paraview normally sits.
  if(m_candidateLocation.isEmpty())
  {
    const QString predictedLocation = "C:/Program Files (x86)/ParaView 3.10.1/bin";
    if(isParaviewHere(predictedLocation))
    {
      acceptPotentialLocation(predictedLocation);
    }
  }
}

/// Destructor
SetUpParaview::~SetUpParaview()
{
}

/// Initialise the layout of the form
void SetUpParaview::initLayout()
{
  clearStatus();

  //Until the user has provided a location, they will not be able to set the result.
  m_uiForm.btn_set->setEnabled(false);
  
  QPalette plt;
  plt.setColor(QPalette::WindowText, Qt::red);
  m_uiForm.lbl_message->setPalette(plt);

  m_candidateLocation = QString(ConfigService::Instance().getString("paraview.path").c_str());

  connect(m_uiForm.btn_choose_location, SIGNAL(clicked()), this, SLOT(onChoose()));
  connect(m_uiForm.btn_set, SIGNAL(clicked()), this, SLOT(onSet()));
  connect(m_uiForm.btn_cancel, SIGNAL(clicked()), this, SLOT(reject()));
  connect(m_uiForm.btn_help, SIGNAL(clicked()), this, SLOT(onHelp()));
  connect(m_uiForm.btn_ignore_paraview, SIGNAL(clicked()), this, SLOT(onIgnoreHenceforth()));
}

///On help requested.
void SetUpParaview::onHelp()
{
  QDesktopServices::openUrl(QUrl(QString("http://www.mantidproject.org/") +
				   "Paraview_setup"));
}

/// Set the result.
void SetUpParaview::onSet()
{
  ConfigServiceImpl& config = ConfigService::Instance();
  config.setParaviewLibraryPath(m_candidateLocation.toStdString());
  config.setString("paraview.path", m_candidateLocation.toStdString());
  std::string filename = config.getUserFilename();
  //Save the result so that on the next start up we don't have to bother the user.
  config.saveConfig(filename);
  this->close();
}

/// Event handler for the ignore paraview henceforth event.
void SetUpParaview::onIgnoreHenceforth()
{
  ConfigServiceImpl& config = ConfigService::Instance();
  config.setString("paraview.ignore", QString::number(true).toStdString());
  std::string filename = config.getUserFilename();
  //Save the result so that on the next start up we don't have to bother the user.
  config.saveConfig(filename);
  this->close();
}

/*
stash location on dialog object and display in ui text box.
@param location: location to stash before applying.
*/
void SetUpParaview::acceptPotentialLocation(const QString& location)
{
  m_candidateLocation = location;
  m_uiForm.txt_location->setText(location);
  m_uiForm.btn_set->setEnabled(true);
}

/*
Handle the rejection of a potential location
@param location: The rejected location
*/
void SetUpParaview::rejectPotentialLocation(const QString& location)
{
  m_candidateLocation = "";
  m_uiForm.txt_location->setText(location); //show the location anyway, good to help the user fix what's wrong
  m_uiForm.btn_set->setEnabled(false);
  writeError("Try again. Expected paraview libaries were not found in the location given.");
}

/// Event handler for the onChoose event.
void SetUpParaview::onChoose()
{
  using boost::regex;
  using boost::regex_search;

  clearStatus();
  QString temp = QFileDialog::getExistingDirectory(this, tr("Open Directory"), "/home");
  temp = QDir::fromNativeSeparators(temp);
  if(isParaviewHere(temp))
  {
    acceptPotentialLocation(temp);
  }
  else
  {
    //Try to predict the location, since users usually do not give the full path to the bin directory
    temp += "/bin";
    if(isParaviewHere(temp))
    {
      acceptPotentialLocation(temp);
    }
    else
    {
      rejectPotentialLocation(temp);
    }
  }
}

/// Clear any existing status messages.
void SetUpParaview::clearStatus()
{
  m_uiForm.lbl_message->clear();
}

/**
Passes error information up to UI.
@param error : error string.
*/
void SetUpParaview::writeError(const QString& error)
{
  m_uiForm.lbl_message->setText(error);
}
