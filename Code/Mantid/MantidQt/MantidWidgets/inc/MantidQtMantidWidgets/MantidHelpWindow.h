#ifndef MANTIDHELPWINDOW_H
#define MANTIDHELPWINDOW_H

#include "MantidQtAPI/MantidHelpInterface.h"
#include "WidgetDllOption.h"
#include <boost/shared_ptr.hpp>
#include <QWidget>
#include <string>

// forward declaration
class QHelpEngine;
class QString;
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
  void showHelp(const QString &url);
    void openWebpage(const std::string &url);

    /// The full path of the collection file.
    std::string m_collectionFile;
    /** The full path of the cache file. If it is not
        determined this is an empty string. */
    std::string m_cacheFile;
    /// The actual help engine
    boost::shared_ptr<QHelpEngine> m_helpEngine;
    /// Whether this is the very first startup of the helpwindow.
    bool m_firstRun;

    void findCollectionFile(std::string & binDir);
    void determineFileLocs();
};

} // namespace MantidWidgets
} // namespace MantidQt
#endif // MANTIDHELPWINDOW_H
