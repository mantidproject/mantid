#include "MantidQtAPI/HelpWindow.h"
#include "MantidKernel/ConfigService.h"
#include <boost/make_shared.hpp>
#include <iostream>
#include <Poco/File.h>
#include <Poco/Path.h>
#include <Poco/Thread.h>
#include <QByteArray>
#include <QDesktopServices>
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

/**
 * Have the help window show a specific url. If the url doesn't exist
 * this just pops up the default view for the help.
 *
 * \param url The url to open. This should start with \link MantidQt::API::BASE_URL.
 * If it is empty show the default page.
 */
void HelpWindowImpl::showURL(const string &url)
{
    std::string urlToShow(url);
    if (urlToShow.empty())
        urlToShow = DEFAULT_URL;

    // make sure the process is going
    this->start(url);
    m_process->waitForStarted(); // REMOVE

    m_log.debug() << "open help url \"" << urlToShow << "\"\n";
    string temp("setSource " + urlToShow + "\n");
    QByteArray ba;
    ba.append(QLatin1String(temp.c_str()));
    m_process->write(ba);
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

    string url(BASE_URL + "html/Algo_" + name + ".html");
    if (name.empty())
        url = BASE_URL + "html/algorithms_index.html";
    this->showURL(url);
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
    string url(BASE_URL + "html/FitFunc_" + name + ".html");
    if (name.empty())
        url = BASE_URL + "html/fitfunctions_index.html";
    this->showURL(url);
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

    // see if chache file exists and remove it
    if ((!m_cacheFile.empty()) && (Poco::File(m_cacheFile).exists()))
    {
        m_log.debug() << "Removing help cache file \"" << m_cacheFile << "\"\n";
        Poco::File(m_cacheFile).remove();
    }

    // start the process
    m_process = boost::make_shared<QProcess>();
    QStringList args;
    args << QLatin1String("-collectionFile")
         << QLatin1String(m_collectionFile.c_str())
         << QLatin1String("-enableRemoteControl");
    if (!url.empty())
        args << QLatin1String("-showUrl")
             << QLatin1String(url.c_str());
    m_log.debug() << m_assistantExe
                  << " " << args.join(QString(" ")).toStdString()
                  << " (state = " << m_process->state() << ")\n";
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
        m_log.debug() << "Very first run of qt assistant comes up slowly (2 sec pause)\n";
        Poco::Thread::sleep(2000); // 2 seconds
    }

    m_log.debug() << m_assistantExe
                  << " " << args.join(QString(" ")).toStdString()
                  << " (state = " << m_process->state() << ")\n";
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

    // all tries have failed
    throw std::runtime_error("Failed to find help system collection file \"" + COLLECTION + "\"");
}

/**
 * Determine the location of the collection and cache files.
 */
void HelpWindowImpl::determineFileLocs()
{
    // determine collection file location
    string binDir = Mantid::Kernel::ConfigService::Instance().getDirectoryOfExecutable();
    this->findCollectionFile(binDir);
    m_log.debug() << "using collection file \"" << m_collectionFile << "\"\n";

    // location for qtassistant
#ifdef __linux__
    // linux it is in system locations
    m_assistantExe = "/usr/bin/assistant";
    if (!Poco::File(m_assistantExe).exists())
    {
        m_log.debug() << "File \"" << m_assistantExe << "\" does not exist\n";
        m_assistantExe = "/usr/local/bin/assistant";
        if (!Poco::File(m_assistantExe).exists())
        {
            m_log.debug() << "File \"" << m_assistantExe << "\" does not exist\n";
            m_assistantExe = "/usr/bin/assistant-qt4";
            if (!Poco::File(m_assistantExe).exists())
            {
                m_log.debug() << "File \"" << m_assistantExe
                              << "\" does not exist. Assuming it is elsewhere in the path.\n";
                m_assistantExe = "assistant";
            }
        }
    }
#else
    // windows it is next to MantidPlot
    m_assistantExe = Poco::Path(binDir, "assistant").absolute().toString();
    if (!Poco::File(m_assistantExe).exists())
    {
        m_log.debug() << "File \"" << m_assistantExe << "\" does not exist\n";
        m_assistantExe = Poco::Path(binDir, "assistant.exe").absolute().toString();
        if (!Poco::File(m_assistantExe).exists())
        {
            m_log.debug() << "File \"" << m_assistantExe << "\" does not exist\n";
            m_assistantExe = "assistant.exe";
        }
    }
#endif
    m_log.debug() << "Using \"" << m_assistantExe << "\" for viewing help\n";

    // determine cache file location
    m_cacheFile = "mantid.qhc";
    QString dataLoc = QDesktopServices::storageLocation(QDesktopServices::DataLocation);
    if (dataLoc.endsWith("mantidproject"))
    {
        Poco::Path path (dataLoc.toStdString(), m_cacheFile);
        m_cacheFile = path.absolute().toString();
    }
    else if (dataLoc.endsWith("MantidPlot")) // understood to end in "ISIS/MantidPlot"
    {
        Poco::Path path(dataLoc.toStdString());
        path = path.parent(); // drop off "MantidPlot"
        path = path.parent(); // drop off "ISIS"
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
