// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#include "MantidICat/CatalogLogin.h"
#include "MantidAPI/AlgorithmManager.h"
#include "MantidAPI/AlgorithmProperty.h"
#include "MantidAPI/CatalogManager.h"
#include "MantidKernel/ConfigService.h"
#include "MantidKernel/FacilityInfo.h"
#include "MantidKernel/ListValidator.h"
#include "MantidKernel/MandatoryValidator.h"
#include "MantidKernel/MaskedProperty.h"

#include <Poco/ActiveResult.h>

namespace Mantid {
namespace ICat {
DECLARE_ALGORITHM(CatalogLogin)

namespace {
std::vector<std::string> namesOfFacilitiesWithICAT() {
  const auto &config = Kernel::ConfigService::Instance();

  const auto facilityDoesNotHaveICAT = [&](std::string name) {
    return config.getFacility(name).catalogInfo().soapEndPoint().empty();
  };

  auto result = Kernel::ConfigService::Instance().getFacilityNames();
  result.erase(
      std::remove_if(result.begin(), result.end(), facilityDoesNotHaveICAT),
      result.end());

  return result;
}

std::string defaultFacility() {
  // If the currently-set facility has ICAT, then we should use it, else
  // we'll just pick the first facility that supports ICAT in the list,
  // else if our Facilities.xml has no ICATs registered, then return an
  // empty string.
  const auto facility = Kernel::ConfigService::Instance().getFacility().name();
  const auto facilitiesWithICAT = namesOfFacilitiesWithICAT();
  if (std::find(facilitiesWithICAT.begin(), facilitiesWithICAT.end(),
                facility) != facilitiesWithICAT.end()) {
    return facility;
  }

  if (facilitiesWithICAT.size() > 0) {
    return facilitiesWithICAT[0];
  }

  return "";
}
} // namespace

/// Init method to declare algorithm properties
void CatalogLogin::init() {
  auto requireValue =
      boost::make_shared<Kernel::MandatoryValidator<std::string>>();
  declareProperty("Username", "", requireValue,
                  "The username to log into the catalog.");
  declareProperty(Kernel::make_unique<Kernel::MaskedProperty<std::string>>(
                      "Password", "", requireValue),
                  "The password of the related username to use.");
  declareProperty("FacilityName", defaultFacility(),
                  boost::make_shared<Kernel::StringListValidator>(
                      namesOfFacilitiesWithICAT()),
                  "Select a facility to log in to.");
  declareProperty(
      "KeepSessionAlive", true,
      "Keeps the session of the catalog alive if login was successful.");
  declareProperty(Kernel::make_unique<API::AlgorithmProperty>(
                      "KeepAlive", boost::make_shared<Kernel::NullValidator>(),
                      Kernel::Direction::Output),
                  "A handle to the KeepAlive algorithm instance that continues "
                  "to keep the catalog alive after this algorithm completes.");
}

/// execute the algorithm
void CatalogLogin::exec() {
  auto catalogInfo = Kernel::ConfigService::Instance()
                         .getFacility(getProperty("FacilityName"))
                         .catalogInfo();
  if (catalogInfo.soapEndPoint().empty())
    throw std::runtime_error(
        "There is no soap end-point for the facility you have selected.");

  g_log.notice() << "Attempting to verify user credentials against "
                 << catalogInfo.catalogName() << '\n';
  progress(0.5, "Verifying user credentials...");

  // Creates a new catalog and related session if the authentication is a
  // success.
  // This allows us to easily manage sessions alongside catalogs in the
  // catalogmanager.
  auto session = API::CatalogManager::Instance().login(
      getProperty("Username"), getProperty("Password"),
      catalogInfo.soapEndPoint(), getProperty("FacilityName"));

  progress(0, "Keeping current sessions alive.");

  if (getProperty("KeepSessionAlive") && session) {
    auto keepAliveAlgorithm =
        API::AlgorithmManager::Instance().create("CatalogKeepAlive");
    keepAliveAlgorithm->initialize();
    keepAliveAlgorithm->setPropertyValue("Session", session->getSessionId());
    keepAliveAlgorithm->executeAsync();

    // Set the output property to the keep alive algorithm
    setProperty("KeepAlive", keepAliveAlgorithm);
  }
}
} // namespace ICat
} // namespace Mantid
