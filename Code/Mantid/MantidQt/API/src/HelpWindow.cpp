#include "MantidQtAPI/HelpWindow.h"
#include "MantidKernel/ConfigService.h"
#include <boost/make_shared.hpp>
#include <iostream>
#include <Poco/File.h>
#include <Poco/Path.h>
#include <Poco/Thread.h>
#include <QByteArray>
#include <QDesktopServices>
#include <QUrl>
#include <stdexcept>

namespace MantidQt
{
namespace API
{
using std::string;

/// Base url for all of the files in the project.
const string BASE_URL("qthelp://org.mantidproject/doc/");
/// Url to display if nothing else is suggested.
const string DEFAULT_URL(BASE_URL + "html/index.html");

/// Base url for all of the wiki links
const string WIKI_BASE_URL("http://mantidproject.org/");
/// Url to display if nothing else is suggested.
const string WIKI_DEFAULT_URL(WIKI_BASE_URL + "MantidPlot");

/**
 * Default constructor shows the \link MantidQt::API::DEFAULT_URL.
 */
HelpWindowImpl::HelpWindowImpl() :
    m_collectionFile(""),
    m_cacheFile(""),
    m_assistantExe(""),
    m_firstRun(true),
    m_log(Mantid::Kernel::Logger::get("HelpWindow"))
{
    this->determineFileLocs();
}

/// Destructor does nothing.
HelpWindowImpl::~HelpWindowImpl()
{
    // do nothing
}

namespace { // ANONYMOUS NAMESPACE
const string stateToStr(const int code)
{
    if (code == 0)
        return "NotRunning";
    else if (code == 1)
        return "Starting";
    else if (code == 2)
        return "Running";
    else
        return "Unknown state";
}
} // ANONYMOUS NAMESPACE

void HelpWindowImpl::openWebpage(const string &url)
{
    m_log.debug() << "open url \"" << url << "\"\n";
    QDesktopServices::openUrl(QUrl(QLatin1String(url.c_str())));
}

/**
 * Have the help window show a specific url. If the url doesn't exist
 * this just pops up the default view for the help.
 *
 * \param url The url to open. This should start with \link MantidQt::API::BASE_URL.
 * If it is empty show the default page.
 */
void HelpWindowImpl::showURL(const string &url)
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

        // make sure the process is going
        this->start(url);
        m_log.debug() << m_assistantExe << " "
                      << " (state = \"" << stateToStr(m_process->state()) << "\")\n";

        m_log.debug() << "open help url \"" << urlToShow << "\"\n";
        string temp("setSource " + urlToShow + "\n");
        QByteArray ba;
        ba.append(QLatin1String(temp.c_str()));
        m_process->write(ba);
    }
}

void HelpWindowImpl::showWikiPage(const string &page)
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
void HelpWindowImpl::showAlgorithm(const string &name, const int version)
{
    // TODO jump to the version within the page
    (void)version;

    if (m_collectionFile.empty()) // qt-assistant disabled
    {
        if (name.empty())
            this->showWikiPage("Category:Algorithms");
        else
            this->showWikiPage(name);
    }
    else
    {
        string url(BASE_URL + "html/Algo_" + name + ".html");
        if (name.empty())
            url = BASE_URL + "html/algorithms_index.html";
        this->showURL(url);
    }
}

/**
 * Convenience method for \link HelpWindowImpl::showAlgorithm(string, int).
 *
 * @param name The name of the algorithm to show. If this is empty show
 * the algorithm index.
 * @param version The version of the algorithm to jump do. The default
 * value (-1) will show the top of the page.
 */
void HelpWindowImpl::showAlgorithm(const QString &name, const int version)
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
void HelpWindowImpl::showFitFunction(const std::string &name)
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
        string url(BASE_URL + "html/FitFunc_" + name + ".html");
        if (name.empty())
        {
            url = BASE_URL + "html/fitfunctions_index.html";
        }
        this->showURL(url);
    }
}

/**
 * Can be called by the host process to indicate that it will
 * close soon. This closes the help window & releases the QProcess
 */
void HelpWindowImpl::hostShuttingDown()
{
  if(m_process)
  {
    if(isRunning())
    {
      m_process->close();
      m_process->waitForFinished(100); // 100ms
    }
    // Delete
    m_process.reset();
  }
}


/**
 * Start up the help browser in a separate process.
 *
 * This will only do something if the browser is not already
 * running. Due to a bug in qt 4.8.1 this will delete the
 * cache file every time the browser is started.
 *
 * @param url The url to show at startup. This is ignored if it is
 * already started.
 */
void HelpWindowImpl::start(const std::string &url)
{
    // check if it is already started
    if (this->isRunning())
    {
        m_log.debug() << "helpwindow process already running\n";
        return;
    }

    // see if chache file exists and remove it - shouldn't be necessary, but it is
    if ((!m_cacheFile.empty()) && (Poco::File(m_cacheFile).exists()))
    {
        m_log.debug() << "Removing help cache file \"" << m_cacheFile << "\"\n";
        Poco::File(m_cacheFile).remove();
    }

    // don't start the process if qt-assistant disabled
    if (m_collectionFile.empty())
    {
        return;
    }

    // determine the argument list
    QStringList args;
    args << QLatin1String("-collectionFile")
         << QLatin1String(m_collectionFile.c_str())
         << QLatin1String("-enableRemoteControl");
    if (!url.empty())
        args << QLatin1String("-showUrl")
             << QLatin1String(url.c_str());


    // start the process
    m_process = boost::make_shared<QProcess>();
    m_log.debug() << m_assistantExe
                  << " " << args.join(QString(" ")).toStdString()
                  << " (state = \"" << stateToStr(m_process->state()) << "\")\n";
    m_process->start(QLatin1String(m_assistantExe.c_str()), args);

    // wait for it to start before returning
    if (!m_process->waitForStarted())
    {
        m_log.warning() << "Failed to start qt assistant process\n";
        return;
    }
    if (m_firstRun)
    {
        m_firstRun = false;
        m_log.debug() << "Very first run of qt assistant comes up slowly (2.5 sec pause)\n";
        Poco::Thread::sleep(2500); // 2.5 seconds
    }
}

/**
 * @return True if the browser is running.
 */
bool HelpWindowImpl::isRunning()
{
    // NULL pointer definitely isn't running
    if (!m_process)
        return false;

    // ask the process for its state
    if (m_process->state() == QProcess::NotRunning)
        return false;
    else
        return true;
}

/**
 * Determine the location of the collection file, "mantid.qhc". This
 * checks in multiple locations and can throw an exception.
 *
 * @param binDir The location of the mantid executable.
 */
void HelpWindowImpl::findCollectionFile(std::string &binDir)
{
    // this being empty notes the feature being disabled
    m_collectionFile = "";

    // name of the collection file itself
    const std::string COLLECTION("mantid.qhc");

    // try next to the executable
    Poco::Path path(binDir, COLLECTION);
    m_log.debug() << "Trying \"" << path.absolute().toString() << "\"\n";
    if (Poco::File(path).exists())
    {
        m_collectionFile =path.absolute().toString();
        return;
    }

    // try where the builds will put it
    path = Poco::Path(binDir, "qtassistant/"+COLLECTION);
    m_log.debug() << "Trying \"" << path.absolute().toString() << "\"\n";
    if (Poco::File(path).exists())
    {
        m_collectionFile = path.absolute().toString();
        return;
    }

    // try in a good linux install location
    path = Poco::Path(binDir, "../share/doc/" + COLLECTION);
    m_log.debug() << "Trying \"" << path.absolute().toString() << "\"\n";
    if (Poco::File(path).exists())
    {
        m_collectionFile = path.absolute().toString();
        return;
    }

    // try a special place for mac/osx
    path = Poco::Path(binDir, "../../share/doc/" + COLLECTION);
    if (Poco::File(path).exists())
    {
        m_collectionFile = path.absolute().toString();
        return;
    }

    // all tries have failed
    m_log.information("Failed to find help system collection file \"" + COLLECTION + "\"");
}

/**
 * Determine the location of qt-assistant executable. This
 * checks in multiple locations.
 *
 * @param binDir The location of the mantid executable.
 */
void HelpWindowImpl::findQtAssistantExe(std::string &binDir)
{
#ifdef __linux__
    // not needed in linux since qt-assistant is
    // assumed system-level installed
    UNUSED_ARG(binDir);

    // check system locations
    m_assistantExe = "/usr/local/bin/assistant-qt4";
    if (Poco::File(m_assistantExe).exists())
        return;
    m_log.debug() << "File \"" << m_assistantExe << "\" does not exist\n";

    m_assistantExe = "/usr/bin/assistant-qt4";
    if (Poco::File(m_assistantExe).exists())
        return;
    m_log.debug() << "File \"" << m_assistantExe << "\" does not exist\n";

    m_assistantExe = "/usr/local/bin/assistant";
    if (Poco::File(m_assistantExe).exists())
        return;
    m_log.debug() << "File \"" << m_assistantExe << "\" does not exist\n";

    m_assistantExe = "/usr/bin/assistant";
    if (Poco::File(m_assistantExe).exists())
        return;
    m_log.debug() << "File \"" << m_assistantExe << "\" does not exist\n";

    // give up and hope it is in the path
    m_log.debug() << "Assuming qt-assistant is elsewhere in the path.\n";
    m_assistantExe = "assistant";
#else
    // windows it is next to MantidPlot
    m_assistantExe = Poco::Path(binDir, "assistant").absolute().toString();
    if (Poco::File(m_assistantExe).exists())
        return;
    m_log.debug() << "File \"" << m_assistantExe << "\" does not exist\n";

    m_assistantExe = Poco::Path(binDir, "assistant.exe").absolute().toString();
    if (Poco::File(m_assistantExe).exists())
        return;
    m_log.debug() << "File \"" << m_assistantExe << "\" does not exist\n";

    // give up and hope it is in the path
    m_log.debug() << "Assuming qt-assistant is elsewhere in the path.\n";
    m_assistantExe = "assistant.exe";
#endif
}

/**
 * Determine the location of the collection and cache files.
 */
void HelpWindowImpl::determineFileLocs()
{
    // determine collection file location
    string binDir = Mantid::Kernel::ConfigService::Instance().getDirectoryOfExecutable();
    this->findCollectionFile(binDir);
    if (m_collectionFile.empty())
    {
        // clear out the other filenames
        m_assistantExe = "";
        m_cacheFile = "";
        return;
    }
    m_log.debug() << "Using collection file \"" << m_collectionFile << "\"\n";

    // location for qtassistant
    this->findQtAssistantExe(binDir);
    if (m_assistantExe.empty())
    {
        // clear out the other filenames
        m_collectionFile = "";
        m_cacheFile = "";
        return;
    }
    m_log.debug() << "Using \"" << m_assistantExe << "\" for viewing help\n";

    // determine cache file location
    m_cacheFile = "mantid.qhc";
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
        m_log.debug() << "Failed to determine help cache file location\n"; // REMOVE
        m_cacheFile = "";
    }
}

} // namespace API
} // namespace MantidQt
