#ifndef HELPWINDOW_H
#define HELPWINDOW_H
#include <boost/shared_ptr.hpp>
#include <QProcess>
#include <QString>
#include <string>
#include "MantidKernel/SingletonHolder.h"

namespace MantidQt
{
namespace API
{

class DLLExport HelpWindowImpl
{
public:
    void showURL(const std::string & url=std::string());
    void showWikiPage(const std::string &page=std::string());
    void showAlgorithm(const std::string &name=std::string(), const int version=-1);
    void showAlgorithm(const QString &name, const int version=-1);
    void showFitFunction(const std::string &name=std::string());
    void hostShuttingDown();

private:
    friend struct Mantid::Kernel::CreateUsingNew<HelpWindowImpl>;

    /// Default constructor
    HelpWindowImpl();
    /// Destructor
    virtual ~HelpWindowImpl();

    void openWebpage(const std::string &url);

    /// Shared pointer to the process running qt assistant.
    boost::shared_ptr<QProcess> m_process;
    /// The full path of the collection file.
    std::string m_collectionFile;
    /** The full path of the cache file. If it is not
        determined this is an empty string. */
    std::string m_cacheFile;
    /// QT assistant executable.
    std::string m_assistantExe;
    /// Whether this is the very first startup of the helpwindow.
    bool m_firstRun;

    void start(const std::string &url);
    bool isRunning();
    void findCollectionFile(std::string & binDir);
    void findQtAssistantExe(std::string & binDir);
    void determineFileLocs();
};

/** Forward declaration of a specialisation of SingletonHolder for HelpWindowImpl
    (needed for dllexport/dllimport) and a typedef for it. */
#ifdef _WIN32
// this breaks new namespace declaraion rules; need to find a better fix
template class DLLExport Mantid::Kernel::SingletonHolder<HelpWindowImpl>; // MANTID_API_DLL was after class
#endif /* _WIN32 */
typedef DLLExport Mantid::Kernel::SingletonHolder<HelpWindowImpl> HelpWindow; // MANTID_API_DLL was after template

} // namespace API
} // namespace MantidQt
#endif // HELPWINDOW_H
