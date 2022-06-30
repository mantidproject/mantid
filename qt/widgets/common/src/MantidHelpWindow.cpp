// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#include "MantidQtWidgets/Common/MantidHelpWindow.h"
#include "MantidAPI/AlgorithmManager.h"
#include "MantidKernel/ConfigService.h"
#include "MantidKernel/Logger.h"
#include "MantidKernel/RegistrationHelper.h"
#include "MantidQtWidgets/Common/InterfaceManager.h"
#include "MantidQtWidgets/Common/MantidDesktopServices.h"
#include "MantidQtWidgets/Common/pqHelpWindow.h"

#include <Poco/File.h>
#include <Poco/Path.h>

#include <QApplication>
#include <QDir>
#include <QFileInfo>
#include <QHelpEngine>
#include <QLatin1Char>
#include <QLatin1String>
#include <QResource>
#include <QStandardPaths>
#include <QString>
#include <QTemporaryFile>
#include <QTextStream>
#include <QUrl>
#include <QWidget>
#include <boost/lexical_cast.hpp>
#include <memory>
#include <stdexcept>

namespace MantidQt::MantidWidgets {

using std::string;
using namespace MantidQt::API;

REGISTER_HELPWINDOW(MantidHelpWindow)

namespace {
/// static logger
Mantid::Kernel::Logger g_log("MantidHelpWindow");
} // namespace

// initialise the help window
QPointer<pqHelpWindow> MantidHelpWindow::g_helpWindow;

/// Base url for all of the files in the project.
const QString BASE_URL("qthelp://org.mantidproject/doc/");
/// Url to display if nothing else is suggested.
const QString DEFAULT_URL(BASE_URL + "index.html");

/// name of the collection file itself
const QString COLLECTION_FILE("MantidProject.qhc");

/**
 * Default constructor shows the @link DEFAULT_URL @endlink.
 */
MantidHelpWindow::MantidHelpWindow(QWidget *parent, const Qt::WindowFlags &flags)
    : MantidHelpInterface(parent), m_collectionFile(""), m_cacheFile("") {
  // find the collection files
  this->determineFileLocs();

  if (!helpWindowExists()) {
    // see if cache file exists and remove it - shouldn't be necessary, but it is
    if (!m_cacheFile.empty()) {
      if (Poco::File(m_cacheFile).exists()) {
        g_log.debug() << "Removing help cache file \"" << m_cacheFile << "\"\n";
        Poco::File(m_cacheFile).remove();
      } else {
        Poco::Path direcPath = Poco::Path(m_cacheFile).parent(); // drop off the filename
        Poco::File direcFile(direcPath.absolute().toString());
        if (!direcFile.exists()) {
          direcFile.createDirectories();
        }
      }
    }

    // create the help engine with the found location
    g_log.debug() << "Loading " << m_collectionFile << "\n";
    auto helpEngine = new QHelpEngine(QString(m_collectionFile.c_str()));
    QObject::connect(helpEngine, SIGNAL(warning(QString)), this, SLOT(warning(QString)));
    g_log.debug() << "Making local cache copy for saving information at " << m_cacheFile << "\n";

    if (helpEngine->copyCollectionFile(QString(m_cacheFile.c_str()))) {
      helpEngine->setCollectionFile(QString(m_cacheFile.c_str()));
    } else {
      g_log.warning("Failed to copy collection file");
      g_log.debug(helpEngine->error().toStdString());
    }
    g_log.debug() << "helpengine.setupData() returned " << helpEngine->setupData() << "\n";

    // create a new help window
    g_helpWindow = new pqHelpWindow(helpEngine, this, flags);
    g_helpWindow->setAttribute(Qt::WA_DeleteOnClose);
    g_helpWindow->setWindowTitle(QString("Mantid - Help"));
    g_helpWindow->setWindowIcon(QIcon(":/images/MantidIcon.ico"));

    // show the home page on startup
    auto registeredDocs = helpEngine->registeredDocumentations();
    if (registeredDocs.size() > 0) {
      g_helpWindow->showHomePage(registeredDocs[0]);
    }
    g_helpWindow->show();
    g_helpWindow->raise();
  }
}

void MantidHelpWindow::showHelp(const QString &url) {
  g_log.debug() << "open help window for \"" << url.toStdString() << "\"\n";
  // bring up the help window if it is showing
  g_helpWindow->show();
  g_helpWindow->raise();
  if (!url.isEmpty()) {
    g_helpWindow->showPage(url);
  }
}

void MantidHelpWindow::openWebpage(const QUrl &url) {
  g_log.debug() << "open url \"" << url.toString().toStdString() << "\"\n";
  MantidDesktopServices::openUrl(url);
}

void MantidHelpWindow::showPage(const QString &url) { this->showPage(QUrl(url)); }

void MantidHelpWindow::showPage(const QUrl &url) {
  if (helpWindowExists()) {
    this->showHelp(!url.isEmpty() ? url.toString() : DEFAULT_URL);
  } else if (!url.isEmpty()) { // qt-assistant disabled
    this->openWebpage(url);
  }
}

/**
 * Have the help window show a specific url. If the url doesn't exist
 * this just pops up the default view for the help.
 *
 * @param url The url to open. This should start with @link BASE_URL @endlink.
 * If it is empty show the default page.
 */
void MantidHelpWindow::showPage(const string &url) { this->showPage(QUrl(QString(url.c_str()))); }

/**
 * Show the help page for a particular algorithm. The page is picked
 * using matching naming conventions.
 *
 * @param name The name of the algorithm to show. If this is empty show
 * the algorithm index.
 * @param version The version of the algorithm to jump do. The default
 * value (-1) will show the top of the page.
 */
void MantidHelpWindow::showAlgorithm(const string &name, const int version) {
  auto versionStr("-v" + boost::lexical_cast<string>(version));
  if (version <= 0) {
    versionStr = ""; // let the redirect do its thing
  }

  QString help_url("");
  if (!name.empty()) {
    auto alg = Mantid::API::AlgorithmManager::Instance().createUnmanaged(name);
    help_url = QString::fromStdString(alg->helpURL());
  }
  if (helpWindowExists()) {
    if (help_url.isEmpty()) {
      QString url(BASE_URL);
      url += "algorithms/";
      if (name.empty()) {
        url += "index.html";
      } else {
        url += QString(name.c_str()) + QString(versionStr.c_str()) + ".html";
      }
      this->showHelp(url);
    } else {
      this->showHelp(help_url);
    }
  } else if (!help_url.isEmpty()) {
    this->openWebpage(help_url);
  }
}

/**
 * Convenience method for HelpWindowImpl::showAlgorithm(const string &, const
 *int).
 *
 * @param name The name of the algorithm to show. If this is empty show
 * the algorithm index.
 * @param version The version of the algorithm to jump do. The default
 * value (-1) will show the top of the page.
 */
void MantidHelpWindow::showAlgorithm(const QString &name, const int version) {
  this->showAlgorithm(name.toStdString(), version);
}

/**
 * Show the help page for a particular concept.
 *
 * @param name The name of the concept to show. If this is empty show
 * the concept index.
 */
void MantidHelpWindow::showConcept(const string &name) {
  if (helpWindowExists()) {
    QString url(BASE_URL);
    url += "concepts/";
    if (name.empty())
      url += "index.html";
    else
      url += QString(name.c_str()) + ".html";
    this->showHelp(url);
  }
}

/**
 * Show the help page for a particular concept.
 *
 * @param name The name of the concept to show. If this is empty show
 * the concept index.
 */
void MantidHelpWindow::showConcept(const QString &name) { this->showConcept(name.toStdString()); }

/**
 * Show the help page for a particular fit function. The page is
 * picked using matching naming conventions.
 *
 * @param name The name of the fit function to show. If it is empty show
 * the fit function index.
 */
void MantidHelpWindow::showFitFunction(const std::string &name) {
  if (helpWindowExists()) {
    QString url(BASE_URL);
    url += "fitting/fitfunctions/";
    auto functionUrl = url + QString(name.c_str()) + ".html";
    if (name.empty() || !g_helpWindow->isExistingPage(functionUrl))
      url += "index.html";
    else
      url = functionUrl;

    this->showHelp(url);
  }
}

/**
 * Show the help page for a particular fit function. The page is
 * picked using matching naming conventions.
 *
 * @param name The name of the fit function to show. If it is empty show
 * the fit function index.
 */
void MantidHelpWindow::showFitFunction(const QString &name) { this->showFitFunction(name.toStdString()); }

/**
 * Show the help page for a given custom interface.
 *
 * @param name The name of the interface to show
 * @param area :: the folder in the custom interface documentation directory
 * @param section :: the section of the interface to show
 */
void MantidHelpWindow::showCustomInterface(const QString &name, const QString &area, const QString &section) {
  this->showCustomInterface(name.toStdString(), area.toStdString(), section.toStdString());
}

/**
 * Show the help page for a given custom interface.
 *
 * @param name The name of the interface to show
 * @param area :: the folder in the custom interface documentation directory
 * @param section :: the section of the interface to show
 */
void MantidHelpWindow::showCustomInterface(const std::string &name, const std::string &area,
                                           const std::string &section) {
  if (helpWindowExists()) {
    QString url(BASE_URL);
    url += "interfaces/";
    if (!area.empty()) {
      url += QString::fromStdString(area) + "/";
    }
    if (name.empty()) {
      url += "index.html";
    } else {
      url += QString::fromStdString(name) + ".html";
      if (!section.empty()) {
        url += "#" + QString::fromStdString(section);
      }
    }
    this->showHelp(url);
  }
}

/**
 * Check to see if the help page url can be found or is missing, without opening a GUI.
 */
bool MantidHelpWindow::doesHelpPageExist(const QUrl &url) {
  const auto binDirectory = QString::fromStdString(Mantid::Kernel::ConfigService::Instance().getPropertiesDir());
  const auto collectionFile = QString::fromStdString(findCollectionFile(binDirectory));

  auto helpEngine = new QHelpEngine(collectionFile);
  return (helpEngine->findFile(url).isValid() && (helpEngine->fileData(url).size() > 0));
}

/**
 * Can be called by the host process to indicate that it will
 * close soon. This closes the help window & releases the QProcess
 */
void MantidHelpWindow::shutdown() {
  // close the window and delete the object
  // Deleting the object ensures the help engine's destructor is called and
  // avoids a segfault when workbench is closed
  if (helpWindowExists()) {
    g_helpWindow->close();
  }
}

/**
 * Determine the location of the collection file, "mantid.qhc". This
 * checks in multiple locations and can throw an exception. For more
 * information see
 *http://doc.qt.digia.com/qq/qq28-qthelp.html#htmlfilesandhelpprojects
 *
 * @param binDirectory The location of the mantid executable.
 */
std::string MantidHelpWindow::findCollectionFile(const QString &binDirectory) {

  std::vector<QDir> searchDirectories;
  // try next to the executable
  searchDirectories.emplace_back(QDir(binDirectory));
  // try where the builds will put it for a single configuration build
  QDir searchDirectory(binDirectory);
  searchDirectory.cdUp();
  if (searchDirectory.cd("docs")) {
    searchDirectory.cd("qthelp");
    searchDirectories.emplace_back(searchDirectory);
  }
  // try where the builds will put it for a multi-configuration build
  searchDirectory.cdUp();
  if (searchDirectory.cd("docs")) {
    searchDirectory.cd("qthelp");
    searchDirectories.emplace_back(searchDirectory);
  }
  // try in windows/linux install location
  searchDirectory = QDir(binDirectory);
  searchDirectory.cdUp();
  searchDirectory.cd("share");
  searchDirectory.cd("doc");
  searchDirectories.emplace_back(searchDirectory);
  // try a special place for mac/osx
  searchDirectory = QDir(binDirectory);
  searchDirectory.cdUp();
  searchDirectory.cdUp();
  searchDirectory.cd("share");
  searchDirectory.cd("doc");
  searchDirectories.emplace_back(searchDirectory);

  for (const auto &directory : searchDirectories) {
    const auto path = directory.absoluteFilePath(COLLECTION_FILE);
    g_log.debug() << "Trying \"" << path.toStdString() << "\"\n";
    if (directory.exists(COLLECTION_FILE)) {
      return path.toStdString();
    }
  }
  return "";
}

/**
 * Determine the location of the cache file.
 * @param collectionFile The location of the collection file.
 */
std::string MantidHelpWindow::findCacheFile(const std::string &collectionFile) {
  std::string cacheFile("");
  if (collectionFile.empty()) {
    // clear out the other filenames
    return cacheFile;
  }
  g_log.debug() << "Using collection file \"" << collectionFile << "\"\n";

  // determine cache file location
  cacheFile = COLLECTION_FILE.toStdString();

  QString dataLoc = QStandardPaths::writableLocation(QStandardPaths::GenericDataLocation) + "/data";

  if (dataLoc.endsWith("mantidproject")) {
    Poco::Path path(dataLoc.toStdString(), cacheFile);
    cacheFile = path.absolute().toString();
  } else {
    g_log.debug() << "Failed to determine help cache file location\n"; // REMOVE
    Poco::Path path(dataLoc.toStdString(), "mantidproject");
    path = Poco::Path(path, COLLECTION_FILE.toStdString());
    cacheFile = path.absolute().toString();
  }
  return cacheFile;
}

/**
 * Determine the location of the collection and cache files.
 */
void MantidHelpWindow::determineFileLocs() {
  // determine collection file location
  string binDir = Mantid::Kernel::ConfigService::Instance().getPropertiesDir();

  m_collectionFile = findCollectionFile(QString::fromStdString(binDir));
  m_cacheFile = findCacheFile(m_collectionFile);
}

void MantidHelpWindow::warning(const QString &msg) { g_log.warning(msg.toStdString()); }

} // namespace MantidQt::MantidWidgets
