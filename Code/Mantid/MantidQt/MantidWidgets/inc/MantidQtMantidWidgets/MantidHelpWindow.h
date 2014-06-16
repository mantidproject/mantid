#ifndef MANTIDHELPWINDOW_H
#define MANTIDHELPWINDOW_H

#include "MantidQtAPI/MantidHelpInterface.h"
#include "WidgetDllOption.h"
#include <boost/shared_ptr.hpp>
#include <QProcess>
#include <string>

// forward declaration
class QWidget;

namespace MantidQt
{
namespace MantidWidgets
{

class EXPORT_OPT_MANTIDQT_MANTIDWIDGETS MantidHelpWindow : public API::MantidHelpInterface
{

public:
  MantidHelpWindow(QWidget* parent=0, Qt::WindowFlags flags=0);
  virtual ~MantidHelpWindow();

  virtual void showPage(const std::string & url=std::string());
  virtual void showPage(const QString & url);
  virtual void showPage(const QUrl & url);
  virtual void showWikiPage(const std::string &page=std::string());
  virtual void showAlgorithm(const std::string &name=std::string(), const int version=-1);
  virtual void showAlgorithm(const QString &name, const int version=-1);
  virtual void showFitFunction(const std::string &name=std::string());
  /// Perform any clean up on main window shutdown
  virtual void shutdown();

private:
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

} // namespace MantidWidgets
} // namespace MantidQt
#endif // MANTIDHELPWINDOW_H
