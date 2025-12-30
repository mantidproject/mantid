// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
/*********************************************************************************
 *  PLEASE READ THIS!!!!!!!
 *
 *  This file MAY NOT be modified to use anything from a package other than
 *Kernel.
 *********************************************************************************/
#pragma once

#include "MantidKernel/ConfigService.h"
#include "MantidKernel/FacilityInfo.h"
#include <filesystem>

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
  ScopedFacilities(const std::string &filename, const std::string &defFacility) {
    auto &config = Mantid::Kernel::ConfigService::Instance();
    defFacilityOnStart = config.getFacility().name();
    std::filesystem::path testFile = std::filesystem::path(config.getInstrumentDirectory()) / filename;
    // Load the test facilities file
    config.updateFacilities(testFile.string());
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
} // namespace FacilityHelper
