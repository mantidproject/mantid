// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#include "MantidICat/CatalogListInstruments.h"
#include "MantidAPI/CatalogManager.h"
#include "MantidKernel/ArrayProperty.h"

namespace Mantid {
namespace ICat {

DECLARE_ALGORITHM(CatalogListInstruments)

/// Init method
void CatalogListInstruments::init() {
  declareProperty("Session", "",
                  "The session information of the catalog to use.");
  declareProperty(std::make_unique<Kernel::ArrayProperty<std::string>>(
                      "InstrumentList", std::vector<std::string>(),
                      boost::make_shared<Kernel::NullValidator>(),
                      Kernel::Direction::Output),
                  "A list containing instrument names.");
}

/// exec method
void CatalogListInstruments::exec() {
  std::vector<std::string> instruments;
  API::CatalogManager::Instance()
      .getCatalog(getPropertyValue("Session"))
      ->listInstruments(instruments);
  setProperty("InstrumentList", instruments);
}
} // namespace ICat
} // namespace Mantid
