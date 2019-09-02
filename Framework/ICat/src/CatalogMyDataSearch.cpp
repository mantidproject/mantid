// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#include "MantidICat/CatalogMyDataSearch.h"
#include "MantidAPI/CatalogManager.h"
#include "MantidAPI/ITableWorkspace.h"
#include "MantidAPI/WorkspaceFactory.h"

namespace Mantid {
namespace ICat {
DECLARE_ALGORITHM(CatalogMyDataSearch)

/// Initialisation method.
void CatalogMyDataSearch::init() {
  declareProperty("Session", "",
                  "The session information of the catalog to use.");
  declareProperty(
      std::make_unique<API::WorkspaceProperty<API::ITableWorkspace>>(
          "OutputWorkspace", "", Kernel::Direction::Output),
      "The name of the workspace to store the search results.");
}

/// Execution method.
void CatalogMyDataSearch::exec() {
  auto outputws =
      API::WorkspaceFactory::Instance().createTable("TableWorkspace");
  API::CatalogManager::Instance()
      .getCatalog(getPropertyValue("Session"))
      ->myData(outputws);
  setProperty("OutputWorkspace", outputws);
}
} // namespace ICat
} // namespace Mantid
