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

/// name of the collection file itself
const QString COLLECTION_FILE("MantidProject.qhc");
/// QtHelp scheme
const QString QTHELP_SCHEME("qthelp");
/// Base url for all of the files in the QtHelp project.
const QString QTHELP_HOST("org.mantidproject");
/// Base path for all files in collection
const QString QTHELP_BASE_PATH("/doc/");
/// HTML scheme
const QString HTML_SCHEME("https");
/// Base url for all of the files in the online html
const QString HTML_HOST("docs.mantidproject.org");
/// Base path for all files in collection
const QString HTML_BASE_PATH("/");
/// Page to display if nothing provided
const QString DEFAULT_PAGENAME("index");

/**
 * Default constructor shows the base index page.
 */
MantidHelpWindow::MantidHelpWindow(const Qt::WindowFlags &flags)
    : MantidHelpInterface(), m_collectionFile(""), m_cacheFile(""), m_firstRun(true) {
  // find the collection and delete the cache file if this is the first run
  if (!helpWindowExists()) {
    this->determineFileLocs();

    // see if chache file exists and remove it - shouldn't be necessary, but it
    // is
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
  // Compute Url from input.
  // An absolute Url is used as is. For relative urls
  // it is assumed the path of the URL is relative to
  // the base directory of that scheme where the data
  // is hosted.

  QUrl targetUrl;
  if (url.isRelative()) {
    const QString pagePath = url.isEmpty() ? DEFAULT_PAGENAME + ".html" : url.path();
    if (helpWindowExists()) {
      targetUrl.setScheme(QTHELP_SCHEME);
      targetUrl.setHost(QTHELP_HOST);
      targetUrl.setPath(QTHELP_BASE_PATH + pagePath);
    } else {
      targetUrl.setScheme(HTML_SCHEME);
      targetUrl.setHost(HTML_HOST);
      targetUrl.setPath(HTML_BASE_PATH + pagePath);
    }
  } else {
    targetUrl = url;
  }

  if (helpWindowExists()) {
    this->showHelp(targetUrl.toString());
  } else // qt-assistant disabled
  {
    this->openWebpage(targetUrl.toString());
  }
}

/**
 * Have the help window show a specific url. If the url doesn't exist
 * this just pops up the default view for the help.
 *
 * @param url The url to open. A relative path is assumed to be relative
 *            to the base url. An absolute path is used as given.
 *            If it is empty show the default page.
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
  this->showAlgorithm(QString::fromStdString(name), version);
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
  auto versionStr("-v" + QString::number(version));
  if (version <= 0) {
    versionStr = ""; // let the redirect do its thing
  }

  // do we have an override of the URL?
  QString helpUrl;
  if (!name.isEmpty()) {
    const auto alg = Mantid::API::AlgorithmManager::Instance().createUnmanaged(name.toStdString());
    helpUrl = QString::fromStdString(alg->helpURL());
  }
  if (helpUrl.isEmpty()) {
    const QString pageName = name.isEmpty() ? DEFAULT_PAGENAME : name;
    const QString pagePath = QString("algorithms/%1%2.html").arg(pageName, versionStr);
    this->showPage(pagePath);
  } else {
    this->showPage(helpUrl);
  }
}

/**
 * Show the help page for a particular concept.
 *
 * @param name The name of the concept to show. If this is empty show
 * the concept index.
 */
void MantidHelpWindow::showConcept(const string &name) { this->showConcept(QString::fromStdString(name)); }

/**
 * Show the help page for a particular concept.
 *
 * @param name The name of the concept to show. If this is empty show
 * the concept index.
 */
void MantidHelpWindow::showConcept(const QString &name) {
  const QString pageName = name.isEmpty() ? DEFAULT_PAGENAME : name;
  const QString pagePath = QString("concepts/%1.html").arg(pageName);
  this->showPage(pagePath);
}

/**
 * Show the help page for a particular fit function. The page is
 * picked using matching naming conventions.
 *
 * @param name The name of the fit function to show. If it is empty show
 * the fit function index.
 */
void MantidHelpWindow::showFitFunction(const std::string &name) { this->showFitFunction(QString::fromStdString(name)); }

/**
 * Show the help page for a particular fit function. The page is
 * picked using matching naming conventions.
 *
 * @param name The name of the fit function to show. If it is empty show
 * the fit function index.
 */
void MantidHelpWindow::showFitFunction(const QString &name) {
  const QString pageName = name.isEmpty() ? DEFAULT_PAGENAME : name;
  const QString pagePath = QString("fitting/fitfunctions/%1.html").arg(pageName);
  this->showPage(pagePath);
}

/**
 * Show the help page for a given custom interface.
 *
 * @param name The name of the interface to show
 * @param area :: the folder in the custom interface documentation directory
 * @param section :: the section of the interface to show
 */
void MantidHelpWindow::showCustomInterface(const QString &name, const QString &area, const QString &section) {
  const QString areaPath = area.isEmpty() ? "" : QString("%1/").arg(area);
  const QString pagePath = QString("%1.html").arg(name.isEmpty() ? DEFAULT_PAGENAME : name);
  const QString sectionAnchor = section.isEmpty() ? "" : QString("#%1").arg(section);
  this->showPage("interfaces/" + areaPath + pagePath + sectionAnchor);
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
  this->showCustomInterface(QString::fromStdString(name), QString::fromStdString(area),
                            QString::fromStdString(section));
}

/**
 * Can be called by the host process to indicate that it will
 * close soon. This closes the help window & releases the QProcess
 */
void MantidHelpWindow::shutdown() {
  // close the window and delete the object
  // Deleting the object ensures the help engine's destructor is called and
  // avoids a segfault when workbench is closed
  g_helpWindow->setAttribute(Qt::WA_DeleteOnClose);
  g_helpWindow->close();
}

/**
 * Determine the location of the collection file, "mantid.qhc". This
 * checks in multiple locations and can throw an exception. For more
 * information see
 *http://doc.qt.digia.com/qq/qq28-qthelp.html#htmlfilesandhelpprojects
 *
 * @param binDir The location of the mantid executable.
 */
void MantidHelpWindow::findCollectionFile(std::string &binDir) {
  // this being empty notes the feature being disabled
  m_collectionFile = "";

  QDir searchDir(QString::fromStdString(binDir));

  // try next to the executable
  QString path = searchDir.absoluteFilePath(COLLECTION_FILE);
  g_log.debug() << "Trying \"" << path.toStdString() << "\"\n";
  if (searchDir.exists(COLLECTION_FILE)) {
    m_collectionFile = path.toStdString();
    return;
  } else {
    g_log.debug() << "QHelp Collection file " << path.toStdString() << " not found\n";
  }

  // try where the builds will put it for a single configuration build
  searchDir.cdUp();
  if (searchDir.cd("docs")) {
    searchDir.cd("qthelp");
    path = searchDir.absoluteFilePath(COLLECTION_FILE);
    g_log.debug() << "Trying \"" << path.toStdString() << "\"\n";
    if (searchDir.exists(COLLECTION_FILE)) {
      m_collectionFile = path.toStdString();
      return;
    }
  }
  // try where the builds will put it for a multi-configuration build
  searchDir.cdUp();
  if (searchDir.cd("docs")) {
    searchDir.cd("qthelp");
    path = searchDir.absoluteFilePath(COLLECTION_FILE);
    g_log.debug() << "Trying \"" << path.toStdString() << "\"\n";
    if (searchDir.exists(COLLECTION_FILE)) {
      m_collectionFile = path.toStdString();
      return;
    }
  }

  // try in windows/linux install location
  searchDir = QDir(QString::fromStdString(binDir));
  searchDir.cdUp();
  searchDir.cd("share");
  searchDir.cd("doc");
  path = searchDir.absoluteFilePath(COLLECTION_FILE);
  g_log.debug() << "Trying \"" << path.toStdString() << "\"\n";
  if (searchDir.exists(COLLECTION_FILE)) {
    m_collectionFile = path.toStdString();
    return;
  }

  // try a special place for mac/osx
  searchDir = QDir(QString::fromStdString(binDir));
  searchDir.cdUp();
  searchDir.cdUp();
  searchDir.cd("share");
  searchDir.cd("doc");
  path = searchDir.absoluteFilePath(COLLECTION_FILE);
  g_log.debug() << "Trying \"" << path.toStdString() << "\"\n";
  if (searchDir.exists(COLLECTION_FILE)) {
    m_collectionFile = path.toStdString();
    return;
  }

  // all tries have failed
  g_log.information("Failed to find help system collection file \"" + COLLECTION_FILE.toStdString() + "\"");
}

/**
 * Determine the location of the collection and cache files.
 */
void MantidHelpWindow::determineFileLocs() {
  // determine collection file location
  string binDir = Mantid::Kernel::ConfigService::Instance().getPropertiesDir();

  this->findCollectionFile(binDir);
  if (m_collectionFile.empty()) {
    // clear out the other filenames
    m_cacheFile = "";
    return;
  }
  g_log.debug() << "Using collection file \"" << m_collectionFile << "\"\n";

  // determine cache file location
  m_cacheFile = COLLECTION_FILE.toStdString();

  QString dataLoc = QStandardPaths::writableLocation(QStandardPaths::GenericDataLocation) + "/data";

  if (dataLoc.endsWith("mantidproject")) {
    Poco::Path path(dataLoc.toStdString(), m_cacheFile);
    m_cacheFile = path.absolute().toString();
  } else if (dataLoc.endsWith("MantidPlot")) // understood to end in "Mantid/MantidPlot"
  {
    Poco::Path path(dataLoc.toStdString());
    path = path.parent(); // drop off "MantidPlot"
    path = path.parent(); // drop off "Mantid"
    path = Poco::Path(path, "mantidproject");
    path = Poco::Path(path, m_cacheFile);
    m_cacheFile = path.absolute().toString();
  } else {
    g_log.debug() << "Failed to determine help cache file location\n"; // REMOVE
    Poco::Path path(dataLoc.toStdString(), "mantidproject");
    path = Poco::Path(path, COLLECTION_FILE.toStdString());
    m_cacheFile = path.absolute().toString();
  }
}

void MantidHelpWindow::warning(const QString &msg) { g_log.warning(msg.toStdString()); }

} // namespace MantidQt::MantidWidgets
