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
#include <QHelpEngine>
#include <QString>
#include <QTemporaryFile>
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

// initialise help engine
boost::shared_ptr<QHelpEngine> MantidHelpWindow::g_helpEngine = NULL;

/// Base url for all of the files in the project.
const string BASE_URL("qthelp://org.mantidproject/doc/");
/// Url to display if nothing else is suggested.
const string DEFAULT_URL(BASE_URL + "html/index.html");

/// Base url for all of the wiki links
const string WIKI_BASE_URL("http://mantidproject.org/");
/// Url to display if nothing else is suggested.
const string WIKI_DEFAULT_URL(WIKI_BASE_URL + "MantidPlot");

/// name of the collection file itself
const std::string COLLECTION_FILE("MantidProject.qhc");

/**
 * Default constructor shows the @link MantidQt::API::DEFAULT_URL @endlink.
 */
MantidHelpWindow::MantidHelpWindow(QWidget* parent, Qt::WindowFlags flags) :
  MantidHelpInterface(),
    m_collectionFile(""),
    m_cacheFile(""),
    m_firstRun(true)
{
  // find the collection and delete the cache file if this is the first run
  if (!bool(g_helpEngine))
  {
    this->determineFileLocs();

    // see if chache file exists and remove it - shouldn't be necessary, but it is
    if ((!m_cacheFile.empty()) && (Poco::File(m_cacheFile).exists()))
    {
      g_log.debug() << "Removing help cache file \"" << m_cacheFile << "\"\n";
      Poco::File(m_cacheFile).remove();
    }

    g_helpEngine = boost::make_shared<QHelpEngine>(QString(m_collectionFile.c_str()), parent);
  }
}

/// Destructor does nothing.
MantidHelpWindow::~MantidHelpWindow()
{
  this->shutdown();
}

void MantidHelpWindow::showHelp(const QString &url)
{
  // help window is a static variable
  static boost::shared_ptr<pqHelpWindow> helpWindow;

  // bring up the help window if it is showing
  if (bool(helpWindow))
  {
    helpWindow->show();
    helpWindow->raise();
    if (!url.isEmpty())
    {
      helpWindow->showPage(url);
    }
    return;
  }

  // create a new help window
  // TODO set the parent widget
  helpWindow = boost::make_shared<pqHelpWindow>(g_helpEngine.get());
  // TODO set window title

  // show the home page on startup
  auto registeredDocs = g_helpEngine->registeredDocumentations();
  if (registeredDocs.size() > 0)
  {
    helpWindow->showHomePage(registeredDocs[0]);
  }
  helpWindow->show();
  helpWindow->raise();
  if (!url.isEmpty())
  {
    helpWindow->showPage(url);
  }
}


void MantidHelpWindow::openWebpage(const string &url)
{
    g_log.debug() << "open url \"" << url << "\"\n";
    QDesktopServices::openUrl(QUrl(QLatin1String(url.c_str())));
}

void MantidHelpWindow::showPage(const QString &url)
{

}

void MantidHelpWindow::showPage(const QUrl &url)
{

}

/**
 * Have the help window show a specific url. If the url doesn't exist
 * this just pops up the default view for the help.
 *
 * @param url The url to open. This should start with @link MantidQt::API::BASE_URL @endlink.
 * If it is empty show the default page.
 */
void MantidHelpWindow::showPage(const string &url)
{
    if (m_collectionFile.empty()) // qt-assistant disabled
    {
        if (url.empty())
            this->openWebpage(WIKI_DEFAULT_URL);
        else
            this->openWebpage(url);
    }
    else
    {
        std::string urlToShow(url);
        if (urlToShow.empty())
            urlToShow = DEFAULT_URL;

        this->showHelp(QString(urlToShow.c_str()));
    }
}

void MantidHelpWindow::showWikiPage(const string &page)
{
    if (page.empty())
        this->openWebpage(WIKI_DEFAULT_URL);
    else
        this->openWebpage(WIKI_BASE_URL + page);
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

    if (m_collectionFile.empty()) // qt-assistant disabled
    {
        if (name.empty())
            this->showWikiPage("Category:Algorithms");
        else
            this->showWikiPage(name);
    }
    else
    {
        string url(BASE_URL + "algorithms/" + name + versionStr + ".html");
        if (name.empty())
            url = BASE_URL + "algorithms/index.html";
        this->showHelp(QString(url.c_str()));
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
 * Show the help page for a particular fit function. The page is
 * picked using matching naming conventions.
 *
 * @param name The name of the fit function to show. If it is empty show
 * the fit function index.
 */
void MantidHelpWindow::showFitFunction(const std::string &name)
{
    if (m_collectionFile.empty()) // qt-assistant disabled
    {
        if (name.empty())
            this->showWikiPage("Category:Fit_functions");
        else
            this->showWikiPage(name);
    }
    else
    {
        string url(BASE_URL + "functions/" + name + ".html");
        if (name.empty())
        {
            url = BASE_URL + "functions/index.html";
        }
        this->showHelp(QString(url.c_str()));
    }
}

/**
 * Can be called by the host process to indicate that it will
 * close soon. This closes the help window & releases the QProcess
 */
void MantidHelpWindow::shutdown()
{
  // close the window
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

    // try next to the executable
    Poco::Path path(binDir, COLLECTION_FILE);
    g_log.debug() << "Trying \"" << path.absolute().toString() << "\"\n";
    if (Poco::File(path).exists())
    {
        m_collectionFile = path.absolute().toString();
        return;
    }

    // try where the builds will put it
    path = Poco::Path(binDir, "qthelp/"+COLLECTION_FILE);
    g_log.debug() << "Trying \"" << path.absolute().toString() << "\"\n";
    if (Poco::File(path).exists())
    {
        m_collectionFile = path.absolute().toString();
        return;
    }
    path = Poco::Path(binDir, "../docs/qthelp/"+COLLECTION_FILE);
    g_log.debug() << "Trying \"" << path.absolute().toString() << "\"\n";
    if (Poco::File(path).exists())
    {
        m_collectionFile = path.absolute().toString();
        return;
    }

    // try in a good linux install location
    path = Poco::Path(binDir, "../share/doc/" + COLLECTION_FILE);
    g_log.debug() << "Trying \"" << path.absolute().toString() << "\"\n";
    if (Poco::File(path).exists())
    {
        m_collectionFile = path.absolute().toString();
        return;
    }

    // try a special place for mac/osx
    path = Poco::Path(binDir, "../../share/doc/" + COLLECTION_FILE);
    if (Poco::File(path).exists())
    {
        m_collectionFile = path.absolute().toString();
        return;
    }

    // all tries have failed
    g_log.information("Failed to find help system collection file \"" + COLLECTION_FILE + "\"");
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
    m_cacheFile = COLLECTION_FILE;
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
        m_cacheFile = "";
    }
}
//const std::string COLLECTION("MantidProject.qhc");

} // namespace MantidWidgets
} // namespace MantidQt
