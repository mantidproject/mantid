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

/**
 * All paths used here are unix-style paths as they will be viewed
 * with a browser
 */

/// Relative path to html docs directory from application directory
const string DEFAULT_PACKAGE_HTML_RELPATH( "../share/doc/html");
/// relative path from html root to algorithms directory
const string ALGORITHMS_DIR("algorithms");
/// relative path from html root to functions directory
const string FUNCTIONS_DIR("functions");

/// Base url for all of the wiki links
const string WIKI_BASE_URL("http://mantidproject.org/");
/// Url to display if nothing else is suggested.
const string WIKI_DEFAULT_URL(WIKI_BASE_URL + "MantidPlot");

/**
 * Show a specific url. If the url doesn't exist
 * this just pops up the default view for the help.
 *
 * @param url The url to open.
 * If it is empty show the default page.
 */
void HelpWindow::showURL(const string &url)
{
  if (url.empty())
    openWebpage(WIKI_DEFAULT_URL);
  else
    openWebpage(url);
}

void HelpWindow::showWikiPage(const string &page)
{
  if (page.empty())
    openWebpage(WIKI_DEFAULT_URL);
  else
    openWebpage(WIKI_BASE_URL + page);
}

/**
 * Show the help page for a particular algorithm. The page is picked
 * using matching naming conventions. If version > 0, then the filename
 * is formed as name-vX.html, otherwise it is formed as name.html.
 *
 * @param name The name of the algorithm to show. If this is empty show
 * the algorithm index.
 * @param version The version of the algorithm to jump do. The default
 * value (-1) will show the top of the page.
 */
void HelpWindow::showAlgorithm(const string &name, const int version)
{
  if (name.empty())
    openWebpage(htmlRoot() + "/categories/algorithms/Algorithms.html");
  else
  {
    string url =  htmlRoot() + "/" + ALGORITHMS_DIR + "/" + name;
    if (version > 0) url += "-v" + boost::lexical_cast<std::string>(version);
    openWebpage(url + ".html");
  }
}

/**
 * Convenience method for HelpWindow::showAlgorithm(const string &, const int).
 *
 * @param name The name of the algorithm to show. If this is empty show
 * the algorithm index.
 * @param version The version of the algorithm to jump do. The default
 * value (-1) will show the top of the page.
 */
void HelpWindow::showAlgorithm(const QString &name, const int version)
{
  showAlgorithm(name.toStdString(), version);
}

/**
 * Show the help page for a particular fit function. The page is
 * picked using matching naming conventions.
 *
 * @param name The name of the fit function to show. If it is empty show
 * the fit function index.
 */
void HelpWindow::showFitFunction(const std::string &name)
{
  if (name.empty())
    openWebpage(htmlRoot() + "/categories/functions/Functions.html");
  else
  {
    std::string url = htmlRoot() + "/" + FUNCTIONS_DIR + "/" + name;
    openWebpage(url + ".html");
  }
}

//------------------------------------------------------------------------------
// Private methods
//------------------------------------------------------------------------------

/**
 * Opens a webpage using the QDesktopServices
 * @param url A string containing a url to view
 */
void HelpWindow::openWebpage(const string &url)
{
  g_log.debug() << "Opening url: \"" << url << "\"\n";
  QDesktopServices::openUrl(QUrl(QLatin1String(url.c_str())));
}

/**
 * Return the root of the html documentation path
 */
std::string HelpWindow::htmlRoot()
{
  using namespace Mantid::Kernel;

  string root;
  try
  {
    root = ConfigService::Instance().getString("docs.html.root");
  }
  catch(Exception::NotFoundError &)
  {
    // Try default package root
    root = DEFAULT_PACKAGE_HTML_RELPATH;
  }
  return root;
}

} // namespace API
} // namespace MantidQt
