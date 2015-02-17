#include "MantidQtMantidWidgets/MantidHelpWindow.h"
#include "MantidQtMantidWidgets/pqHelpWindow.h"
#include "MantidQtAPI/InterfaceManager.h"
#include "MantidKernel/ConfigService.h"
#include "MantidKernel/Logger.h"
#include "MantidKernel/RegistrationHelper.h"
#include <boost/make_shared.hpp>
#include <boost/lexical_cast.hpp>
#include <iostream>
#include <Poco/File.h>
#include <Poco/Path.h>
#include <QDesktopServices>
#include <QDir>
#include <QFileInfo>
#include <QHelpEngine>
#include <QLatin1Char>
#include <QLatin1String>
#include <QResource>
#include <QString>
#include <QTemporaryFile>
#include <QTextStream>
#include <QUrl>
#include <QWidget>
#include <stdexcept>

namespace MantidQt
{
namespace MantidWidgets
{

using std::string;
using namespace MantidQt::API;

REGISTER_HELPWINDOW(MantidHelpWindow)

namespace
{
  /// static logger
  Mantid::Kernel::Logger g_log("MantidHelpWindow");
}

// initialise the help window
pqHelpWindow *MantidHelpWindow::g_helpWindow = NULL;

/// Base url for all of the files in the project.
const QString BASE_URL("qthelp://org.mantidproject/doc/");
/// Url to display if nothing else is suggested.
const QString DEFAULT_URL(BASE_URL + "index.html");

/// Base url for all of the wiki links
const QString WIKI_BASE_URL("http://mantidproject.org/");
/// Url to display if nothing else is suggested.
const QString WIKI_DEFAULT_URL(WIKI_BASE_URL + "MantidPlot");

/// name of the collection file itself
const QString COLLECTION_FILE("MantidProject.qhc");

/**
 * Default constructor shows the @link DEFAULT_URL @endlink.
 */
MantidHelpWindow::MantidHelpWindow(QWidget* parent, Qt::WindowFlags flags) :
  MantidHelpInterface(),
    m_collectionFile(""),
    m_cacheFile(""),
    m_firstRun(true)
{
  // find the collection and delete the cache file if this is the first run
  if (!bool(g_helpWindow))
  {
    this->determineFileLocs();

    // see if chache file exists and remove it - shouldn't be necessary, but it is
    if (!m_cacheFile.empty())
    {
      if (Poco::File(m_cacheFile).exists())
      {
        g_log.debug() << "Removing help cache file \"" << m_cacheFile << "\"\n";
        Poco::File(m_cacheFile).remove();
      }
      else
      {
        Poco::Path direcPath = Poco::Path(m_cacheFile).parent(); // drop off the filename
        Poco::File direcFile(direcPath.absolute().toString());
        if (!direcFile.exists())
        {
          direcFile.createDirectories();
        }
      }
    }

    // create the help engine with the found location
    g_log.debug() << "Loading " << m_collectionFile << "\n";
    auto helpEngine = new QHelpEngine(QString(m_collectionFile.c_str()), parent);
    QObject::connect(helpEngine, SIGNAL(warning(QString)), this, SLOT(warning(QString)));
    g_log.debug() << "Making local cache copy for saving information at "
                  << m_cacheFile << "\n";
    if (helpEngine->copyCollectionFile(QString(m_cacheFile.c_str())))
    {
      helpEngine->setCollectionFile(QString(m_cacheFile.c_str()));
    }
    else
    {
      g_log.warning("Failed to copy collection file");
    }
    g_log.debug() << "helpengine.setupData() returned " << helpEngine->setupData() << "\n";

    // create a new help window
    g_helpWindow = new pqHelpWindow(helpEngine, parent, flags);
    g_helpWindow->setWindowTitle(QString("MantidPlot - help"));

    // show the home page on startup
    auto registeredDocs = helpEngine->registeredDocumentations();
    if (registeredDocs.size() > 0)
    {
      g_helpWindow->showHomePage(registeredDocs[0]);
    }
    g_helpWindow->show();
    g_helpWindow->raise();
  }
}

/// Destructor does nothing.
MantidHelpWindow::~MantidHelpWindow()
{
  this->shutdown();
}

void MantidHelpWindow::showHelp(const QString &url)
{
  g_log.debug() << "open help window for \"" << url.toStdString() << "\"\n";
  // bring up the help window if it is showing
  g_helpWindow->show();
  g_helpWindow->raise();
  if (!url.isEmpty())
  {
    g_helpWindow->showPage(url);
  }
}


void MantidHelpWindow::openWebpage(const QUrl &url)
{
    g_log.debug() << "open url \"" << url.toString().toStdString() << "\"\n";
    QDesktopServices::openUrl(url);
}

void MantidHelpWindow::showPage(const QString &url)
{
  this->showPage(QUrl(url));
}

void MantidHelpWindow::showPage(const QUrl &url)
{
  if (bool(g_helpWindow))
  {
    if (url.isEmpty())
      this->showHelp(DEFAULT_URL);
    else
      this->showHelp(url);
  }
  else // qt-assistant disabled
  {
      if (url.isEmpty())
          this->openWebpage(WIKI_DEFAULT_URL);
      else
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
void MantidHelpWindow::showPage(const string &url)
{
  this->showPage(QUrl(QString(url.c_str())));
}

void MantidHelpWindow::showWikiPage(const string &page)
{
    if (page.empty())
        this->openWebpage(WIKI_DEFAULT_URL);
    else
        this->openWebpage(WIKI_BASE_URL + page.c_str());
}

/**
 * Show the help page for a particular algorithm. The page is picked
 * using matching naming conventions.
 *
 * @param name The name of the algorithm to show. If this is empty show
 * the algorithm index.
 * @param version The version of the algorithm to jump do. The default
 * value (-1) will show the top of the page.
 */
void MantidHelpWindow::showAlgorithm(const string &name, const int version)
{
    auto versionStr("-v" + boost::lexical_cast<string>(version));
    if (version <= 0)
      versionStr = ""; // let the redirect do its thing

    if (bool(g_helpWindow))
    {
        QString url(BASE_URL);
        url += "algorithms/";
        if (name.empty())
            url += "index.html";
        else
          url += QString(name.c_str()) + QString(versionStr.c_str()) + ".html";
        this->showHelp(url);
    }
    else // qt-assistant disabled
    {
        if (name.empty())
            this->showWikiPage("Category:Algorithms");
        else
            this->showWikiPage(name);
    }
}

/**
 * Convenience method for HelpWindowImpl::showAlgorithm(const string &, const int).
 *
 * @param name The name of the algorithm to show. If this is empty show
 * the algorithm index.
 * @param version The version of the algorithm to jump do. The default
 * value (-1) will show the top of the page.
 */
void MantidHelpWindow::showAlgorithm(const QString &name, const int version)
{
    this->showAlgorithm(name.toStdString(), version);
}


/**
 * Show the help page for a particular concept. 
 *
 * @param name The name of the concept to show. If this is empty show
 * the concept index.
 */
void MantidHelpWindow::showConcept(const string &name)
{
    if (bool(g_helpWindow))
    {
        QString url(BASE_URL);
        url += "concepts/";
        if (name.empty())
            url += "index.html";
        else
          url += QString(name.c_str()) + ".html";
        this->showHelp(url);
    }
    else // qt-assistant disabled
    {
        if (name.empty())
            this->showWikiPage("Category:Concepts");
        else
            this->showWikiPage(name);
    }
}


/**
 * Show the help page for a particular concept. 
 *
 * @param name The name of the concept to show. If this is empty show
 * the concept index.
 */
void MantidHelpWindow::showConcept(const QString &name)
{
    this->showConcept(name.toStdString());
}

/**
 * Show the help page for a particular fit function. The page is
 * picked using matching naming conventions.
 *
 * @param name The name of the fit function to show. If it is empty show
 * the fit function index.
 */
void MantidHelpWindow::showFitFunction(const std::string &name)
{
    if (bool(g_helpWindow))
    {
        QString url(BASE_URL);
        url += "functions/";
        if (name.empty())
            url += "index.html";
        else
        url += QString(name.c_str()) + ".html";

        this->showHelp(url);
    }
    else // qt-assistant disabled
    {
        if (name.empty())
            this->showWikiPage("Category:Fit_functions");
        else
            this->showWikiPage(name);
    }
}

/**
 * Show the help page for a given custom interface.
 *
 * @param name The name of the interface to show
 */
void MantidHelpWindow::showCustomInterface(const QString &name)
{
    this->showCustomInterface(name.toStdString());
}

/**
 * Show the help page for a given custom interface.
 *
 * @param name The name of the interface to show
 */
void MantidHelpWindow::showCustomInterface(const std::string &name)
{
    if (bool(g_helpWindow))
    {
        QString url(BASE_URL);
        url += "interfaces/";
        if (name.empty())
            url += "index.html";
        else
          url += QString(name.c_str()) + ".html";
        this->showHelp(url);
    }
}

/**
 * Can be called by the host process to indicate that it will
 * close soon. This closes the help window & releases the QProcess
 */
void MantidHelpWindow::shutdown()
{
  // close the window
  g_helpWindow->close();
}

/**
 * Determine the location of the collection file, "mantid.qhc". This
 * checks in multiple locations and can throw an exception. For more
 * information see http://doc.qt.digia.com/qq/qq28-qthelp.html#htmlfilesandhelpprojects
 *
 * @param binDir The location of the mantid executable.
 */
void MantidHelpWindow::findCollectionFile(std::string &binDir)
{
    // this being empty notes the feature being disabled
    m_collectionFile = "";

    QDir searchDir(QString::fromStdString(binDir));
        
    // try next to the executable
    QString path = searchDir.absoluteFilePath(COLLECTION_FILE);
    g_log.debug() << "Trying \"" << path.toStdString() << "\"\n";
    if ( searchDir.exists(COLLECTION_FILE) )
    {
      m_collectionFile = path.toStdString();
      return;
    }

    // try where the builds will put it for a single configuration build
    searchDir.cdUp();
    if(searchDir.cd("docs"))
    {
      searchDir.cd("qthelp");
      path = searchDir.absoluteFilePath(COLLECTION_FILE);
      g_log.debug() << "Trying \"" << path.toStdString() << "\"\n";
      if ( searchDir.exists(COLLECTION_FILE) )
      {
        m_collectionFile = path.toStdString();
        return;
      }
    }
    // try where the builds will put it for a multi-configuration build
    searchDir.cdUp();
    if(searchDir.cd("docs"))
    {
      searchDir.cd("qthelp");
      path = searchDir.absoluteFilePath(COLLECTION_FILE);
      g_log.debug() << "Trying \"" << path.toStdString() << "\"\n";
      if ( searchDir.exists(COLLECTION_FILE) )
      {
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
    if (searchDir.exists(COLLECTION_FILE))
    {
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
    if (searchDir.exists(COLLECTION_FILE))
    {
      m_collectionFile = path.toStdString();
      return;
    }

    // all tries have failed
    g_log.information("Failed to find help system collection file \"" + COLLECTION_FILE.toStdString() + "\"");
}

/**
 * Determine the location of the collection and cache files.
 */
void MantidHelpWindow::determineFileLocs()
{
    // determine collection file location
    string binDir = Mantid::Kernel::ConfigService::Instance().getDirectoryOfExecutable();
    this->findCollectionFile(binDir);
    if (m_collectionFile.empty())
    {
        // clear out the other filenames
        m_cacheFile = "";
        return;
    }
    g_log.debug() << "Using collection file \"" << m_collectionFile << "\"\n";

    // determine cache file location
    m_cacheFile = COLLECTION_FILE.toStdString();
    QString dataLoc = QDesktopServices::storageLocation(QDesktopServices::DataLocation);
    if (dataLoc.endsWith("mantidproject"))
    {
        Poco::Path path (dataLoc.toStdString(), m_cacheFile);
        m_cacheFile = path.absolute().toString();
    }
    else if (dataLoc.endsWith("MantidPlot")) // understood to end in "Mantid/MantidPlot"
    {
        Poco::Path path(dataLoc.toStdString());
        path = path.parent(); // drop off "MantidPlot"
        path = path.parent(); // drop off "Mantid"
        path = Poco::Path(path, "mantidproject");
        path = Poco::Path(path, m_cacheFile);
        m_cacheFile = path.absolute().toString();
    }
    else
    {
        g_log.debug() << "Failed to determine help cache file location\n"; // REMOVE
        Poco::Path path(dataLoc.toStdString(), "mantidproject");
        path = Poco::Path(path, COLLECTION_FILE.toStdString());
        m_cacheFile = path.absolute().toString();
    }
}

void MantidHelpWindow::warning(QString msg)
{
  g_log.warning(msg.toStdString());
}

} // namespace MantidWidgets
} // namespace MantidQt
