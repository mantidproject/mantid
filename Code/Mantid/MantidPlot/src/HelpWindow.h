#ifndef HELPWINDOW_H
#define HELPWINDOW_H
#include <boost/shared_ptr.hpp>
#include <QProcess>
#include <string>
#include "MantidKernel/Logger.h"

class HelpWindow
{
public:
    HelpWindow();
    virtual ~HelpWindow();
    void showURL(const std::string & url);
    void showAlgorithm(const std::string &name, const int version=-1);
    void showFitFunction(const std::string &name);

private:
    /// Shared pointer to the process running qt assistant.
    boost::shared_ptr<QProcess> m_process;
    /// The full path of the collection file.
    std::string m_collectionFile;
    /** The full path of the cache file. If it is not
        determined this is an empty string. */
    std::string m_cacheFile;
    /// QT assistant executable.
    std::string m_assistantExe;
    /// The logger for the class.
    Mantid::Kernel::Logger& m_log;

    void start();
    bool isRunning();
    void findCollectionFile(std::string & binDir);
    void determineFileLocs();
};
#endif // HELPWINDOW_H
