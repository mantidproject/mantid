// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2026 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include "MantidNexus/DllConfig.h"
#include "MantidNexus/NexusFile_fwd.h"

#include <hdf5.h>
#include <optional>
#include <span>
#include <string>
#include <vector>

namespace Mantid::Nexus::H5ValidatingAdapter {

/** Identifies the HDF5 operation and object involved in a failure. */
struct MANTID_NEXUS_DLL Context {
  std::string operation;
  std::string filename;
  std::string object;
};

MANTID_NEXUS_DLL void checkStatus(herr_t status, const Context &context);
MANTID_NEXUS_DLL bool checkTriState(htri_t status, const Context &context);
MANTID_NEXUS_DLL void checkId(hid_t id, const Context &context);

MANTID_NEXUS_DLL int rank(hid_t dataspace, const Context &context);
MANTID_NEXUS_DLL DimVector dimensions(hid_t dataspace, const Context &context);
MANTID_NEXUS_DLL std::size_t elementCount(std::span<const dimsize_t> dimensions, const Context &context);
MANTID_NEXUS_DLL std::size_t elementCount(hid_t dataspace, const Context &context);

MANTID_NEXUS_DLL void readDataset(hid_t dataset, hid_t memoryType, hid_t memorySpace, hid_t fileSpace, void *data,
                                  const Context &context);
MANTID_NEXUS_DLL void writeDataset(hid_t dataset, hid_t memoryType, hid_t memorySpace, hid_t fileSpace,
                                   const void *data, const Context &context);
MANTID_NEXUS_DLL std::optional<std::string> readScalarVlenDataset(hid_t dataset, const Context &context);
MANTID_NEXUS_DLL std::vector<std::optional<std::string>> readVlenDataset(hid_t dataset, const Context &context);
MANTID_NEXUS_DLL std::optional<std::string> readScalarVlenAttribute(hid_t attribute, const Context &context);

MANTID_NEXUS_DLL void selectHyperslab(hid_t dataspace, std::span<const dimsize_t> start,
                                      std::span<const dimsize_t> size, const Context &context);

} // namespace Mantid::Nexus::H5ValidatingAdapter
