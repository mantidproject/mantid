// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#include "CatalogSearcher.h"
#include "GUI/MainWindow/IMainWindowView.h"

#include "MantidAPI/AlgorithmManager.h"
#include "MantidAPI/CatalogManager.h"
#include "MantidAPI/ITableWorkspace.h"

using namespace Mantid::API;

namespace MantidQt {
namespace CustomInterfaces {

CatalogSearcher::CatalogSearcher(IMainWindowView *view) : m_view(view) {}

ITableWorkspace_sptr CatalogSearcher::search(const std::string &text) {
  logInToCatalog();
  auto sessionId = activeSessionId();

  auto algSearch = AlgorithmManager::Instance().create("CatalogGetDataFiles");
  algSearch->initialize();
  algSearch->setChild(true);
  algSearch->setLogging(false);
  algSearch->setProperty("Session", sessionId);
  algSearch->setProperty("InvestigationId", text);
  algSearch->setProperty("OutputWorkspace", "_ReflSearchResults");
  algSearch->execute();
  ITableWorkspace_sptr results = algSearch->getProperty("OutputWorkspace");

  // Now, tidy up the data
  std::set<size_t> toRemove;
  for (size_t i = 0; i < results->rowCount(); ++i) {
    std::string &run = results->String(i, 0);

    // Too short to be more than ".raw or .nxs"
    if (run.size() < 5) {
      toRemove.insert(i);
    }
  }

  // Sets are sorted so if we go from back to front we won't trip over ourselves
  for (auto row = toRemove.rbegin(); row != toRemove.rend(); ++row)
    results->removeRow(*row);

  return results;
}

bool CatalogSearcher::hasActiveSession() const {
  auto sessions = CatalogManager::Instance().getActiveSessions();
  return !sessions.empty();
}

/** Log in to the catalog
 * @throws : if login failed
 */
void CatalogSearcher::logInToCatalog() {
  if (hasActiveSession())
    return;

  try {
    std::stringstream pythonSrc;
    pythonSrc << "try:\n";
    pythonSrc << "  algm = CatalogLoginDialog()\n";
    pythonSrc << "except:\n";
    pythonSrc << "  pass\n";
    m_view->runPythonAlgorithm(pythonSrc.str());
  } catch (std::runtime_error &e) {
    throw std::runtime_error(std::string("Catalog login failed: ") + e.what());
  }

  // Check we logged in ok
  if (!hasActiveSession())
    throw std::runtime_error("Catalog login failed");
}

std::string CatalogSearcher::activeSessionId() const {
  auto sessions = CatalogManager::Instance().getActiveSessions();
  if (sessions.empty())
    throw std::runtime_error("You are not logged into any catalogs.");

  return sessions.front()->getSessionId();
}
} // namespace CustomInterfaces
} // namespace MantidQt
