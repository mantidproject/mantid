#include "MantidQtAPI/HelpWindow.h"
#include "MantidQtAPI/MantidHelpInterface.h"
#include "MantidQtAPI/InterfaceManager.h"
#include "MantidKernel/ConfigService.h"
#include "MantidKernel/Exception.h"
#include "MantidKernel/Logger.h"

#include <boost/lexical_cast.hpp>
#include <QDesktopServices>
#include <QUrl>

namespace MantidQt
{
namespace API
{
  namespace
  {
    /// static logger
    Mantid::Kernel::Logger g_log("HelpWindow");
  }

  using std::string;

  void HelpWindow::showPage(const std::string & url)
  {
    InterfaceManager interfaceManager;
    MantidHelpInterface *gui = interfaceManager.createHelpWindow();
    if (gui)
    {
      gui->showPage(url);
    }
    else
    {
      g_log.error() << "Failed to launch help for page " << url << "\n";
    }
  }

  void HelpWindow::showPage(const QString & url)
  {
    InterfaceManager interfaceManager;
    MantidHelpInterface *gui = interfaceManager.createHelpWindow();
    if (gui)
    {
      gui->showPage(url);
    }
    else
    {
      g_log.error() << "Failed to launch help for page " << url.toStdString() << "\n";
    }
  }

  void HelpWindow::showPage(const QUrl & url)
  {
    InterfaceManager interfaceManager;
    MantidHelpInterface *gui = interfaceManager.createHelpWindow();
    if (gui)
    {
      gui->showPage(url);
    }
    else
    {
      g_log.error() << "Failed to launch help for page " << url.toString().toStdString() << "\n";
    }
  }

  void HelpWindow::showWikiPage(const std::string &page)
  {
    InterfaceManager interfaceManager;
    MantidHelpInterface *gui = interfaceManager.createHelpWindow();
    if (gui)
    {
      gui->showWikiPage(page);
    }
    else
    {
      g_log.error() << "Failed to launch help for wiki page " << page << "\n";
    }
  }

  void HelpWindow::showAlgorithm(const std::string &name, const int version)
  {
    InterfaceManager interfaceManager;
    MantidHelpInterface *gui = interfaceManager.createHelpWindow();
    if (gui)
    {
      gui->showAlgorithm(name, version);
    }
    else
    {
      g_log.error() << "Failed to launch help for algorithm " << name << " v" << version << "\n";
    }
  }

  void HelpWindow::showAlgorithm(const QString &name, const int version)
  {
    InterfaceManager interfaceManager;
    MantidHelpInterface *gui = interfaceManager.createHelpWindow();
    if (gui)
    {
      gui->showAlgorithm(name, version);
    }
    else
    {
      g_log.error() << "Failed to launch help for algorithm " << name.toStdString()
                    << " v" << version << "\n";
    }
  }

  void HelpWindow::showFitFunction(const std::string &name)
  {
    InterfaceManager interfaceManager;
    MantidHelpInterface *gui = interfaceManager.createHelpWindow();
    if (gui)
    {
      gui->showFitFunction(name);
    }
    else
    {
      g_log.error() << "Failed to launch help for fit function " << name << "\n";
    }
  }

} // namespace API
} // namespace MantidQt
