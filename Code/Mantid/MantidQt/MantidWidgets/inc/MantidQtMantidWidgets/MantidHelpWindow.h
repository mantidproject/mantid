#ifndef MANTIDHELPWINDOW_H
#define MANTIDHELPWINDOW_H

#include "MantidQtAPI/MantidHelpInterface.h"
#include "WidgetDllOption.h"
#include <QWidget>
#include <string>

// forward declaration
class QHelpEngine;
class QString;
class QWidget;
class pqHelpWindow;

namespace MantidQt
{
namespace MantidWidgets
{

class EXPORT_OPT_MANTIDQT_MANTIDWIDGETS MantidHelpWindow : public API::MantidHelpInterface
{
Q_OBJECT

public:
  MantidHelpWindow(QWidget* parent=0, Qt::WindowFlags flags=0);
  virtual ~MantidHelpWindow();

  virtual void showPage(const std::string & url=std::string());
  virtual void showPage(const QString & url);
  virtual void showPage(const QUrl & url);
  virtual void showWikiPage(const std::string &page=std::string());
  virtual void showAlgorithm(const std::string &name=std::string(), const int version=-1);
  virtual void showAlgorithm(const QString &name, const int version=-1);
  virtual void showConcept(const std::string &name);
  virtual void showConcept(const QString &name);
  virtual void showFitFunction(const std::string &name=std::string());
  virtual void showCustomInterface(const std::string &name=std::string());
  virtual void showCustomInterface(const QString &name);

private:
  void showHelp(const QString &url);
    void openWebpage(const QUrl &url);

    /// The full path of the collection file.
    std::string m_collectionFile;
    /** The full path of the cache file. If it is not
        determined this is an empty string. */
    std::string m_cacheFile;
    /// The window that renders the help information
    static pqHelpWindow *g_helpWindow;

    /// Whether this is the very first startup of the helpwindow.
    bool m_firstRun;

    void findCollectionFile(std::string & binDir);
    void determineFileLocs();

public slots:
  /// Perform any clean up on main window shutdown
  virtual void shutdown();
  void warning(QString msg);
};

} // namespace MantidWidgets
} // namespace MantidQt
#endif // MANTIDHELPWINDOW_H
