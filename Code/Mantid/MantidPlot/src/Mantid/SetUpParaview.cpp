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

/// Constructor
SetUpParaview::SetUpParaview(QWidget *parent) : QDialog(parent)
{
  m_uiForm.setupUi(this);

  QPalette plt;
  plt.setColor(QPalette::WindowText, Qt::red);
  m_uiForm.lbl_message->setPalette(plt);

  initLayout();
}

/// Destructor
SetUpParaview::~SetUpParaview()
{
}

/// Initialise the layout of the form
void SetUpParaview::initLayout()
{
  clearStatus();

  m_candidateLocation = QString(ConfigService::Instance().getString("paraview.path").c_str());

  connect(m_uiForm.btn_choose_location, SIGNAL(clicked()), this, SLOT(onChoose()));
  connect(m_uiForm.btn_set, SIGNAL(clicked()), this, SLOT(onSet()));
  connect(m_uiForm.btn_cancel, SIGNAL(clicked()), this, SLOT(reject()));
  connect(m_uiForm.btn_help, SIGNAL(clicked()), this, SLOT(onHelp()));
  
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
  
  std::cout << "getting location" << m_candidateLocation.toStdString() << std::endl;
  config.setParaviewLibraryPath(m_candidateLocation.toStdString());

  std::cout << "getting paraview path" << std::endl;
  config.setString("paraview.path", m_candidateLocation.toStdString());

  std::cout << "getting file name" << std::endl;
  std::string filename = config.getUserFilename();
  //Save the result so that on the next start up we don't have to bother the user.

  std::cout << "saving" << std::endl;
  config.saveConfig(filename);

  std::cout << "closing" << std::endl;
  this->close();
}

/// Event handler for the onChoose event.
void SetUpParaview::onChoose()
{
  using boost::regex;
  using boost::regex_search;

  clearStatus();
  QString temp = QFileDialog::getExistingDirectory(this, tr("Open Directory"), "/home");
  temp = QDir::fromNativeSeparators(temp);
  if(!temp.isEmpty())
  {
    bool found = false;

    QDirIterator it(temp, QDirIterator::NoIteratorFlags);
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
    if(!found)
    {
      writeError("Try again. Expected paraview libaries were not found in the location given.");
    }
    else
    {
      m_candidateLocation = temp;
      m_uiForm.txt_location->setText(temp);
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
