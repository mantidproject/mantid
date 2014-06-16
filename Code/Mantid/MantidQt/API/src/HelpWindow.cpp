#include "MantidQtAPI/HelpWindow.h"
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
  {}

  void HelpWindow::showPage(const QString & url)
  {}

  void HelpWindow::showPage(const QUrl & url)
  {}

  void HelpWindow::showWikiPage(const std::string &page)
  {}

  void HelpWindow::showAlgorithm(const std::string &name, const int version)
  {}

  void HelpWindow::showAlgorithm(const QString &name, const int version)
  {}

  void HelpWindow::showFitFunction(const std::string &name)
  {}

} // namespace API
} // namespace MantidQt
