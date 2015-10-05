/*********************************************************************************
 *  PLEASE READ THIS!!!!!!!
 *
 *  This file MAY NOT be modified to use anything from a package other than
 *Kernel.
 *********************************************************************************/
#ifndef TESTHELPERS_FACILITYHELPER_H_
#define TESTHELPERS_FACILITYHELPER_H_

#include "MantidKernel/ConfigService.h"
#include "MantidKernel/FacilityInfo.h"
#include <Poco/Path.h>

namespace FacilityHelper {

/**
 * Simple RAII struct to switch the facilities files to the when constructed
 * and switch back to the previous settings on destruction
 */
struct ScopedFacilities {
  /**
   * Switch facilities file & default facility
   * @param filename It is assumed it is in the instrument directory
   * @param defFacility
   */
  ScopedFacilities(const std::string &filename,
                   const std::string &defFacility) {
    auto &config = Mantid::Kernel::ConfigService::Instance();
    defFacilityOnStart = config.getFacility().name();
    Poco::Path testFile =
        Poco::Path(config.getInstrumentDirectory()).resolve(filename);
    // Load the test facilities file
    config.updateFacilities(testFile.toString());
    config.setFacility(defFacility);
  }

  ~ScopedFacilities() {
    auto &config = Mantid::Kernel::ConfigService::Instance();
    // Restore the main facilities file
    config.updateFacilities(); // no file loads the default
    config.setFacility(defFacilityOnStart);
  }

private:
  std::string defFacilityOnStart;
};
}

#endif /* TESTHELPERS_FACILITYHELPER_H_ */
