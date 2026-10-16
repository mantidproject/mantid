// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2026 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include "MantidLegacyNexus/DllConfig.h"

#include <hdf5.h>

#include <cstddef>
#include <span>
#include <string>
#include <vector>

namespace Mantid::LegacyNexus::H5ErrorHandling {

struct Context {
  std::string operation;
  std::string object;
};

MANTID_LEGACYNEXUS_DLL bool checkStatus(herr_t status, const Context &context);
MANTID_LEGACYNEXUS_DLL bool checkTriState(htri_t status, bool &result, const Context &context);
MANTID_LEGACYNEXUS_DLL bool checkId(hid_t id, const Context &context);

MANTID_LEGACYNEXUS_DLL bool rank(hid_t dataspace, int &result, const Context &context);
MANTID_LEGACYNEXUS_DLL bool dimensions(hid_t dataspace, std::vector<hsize_t> &result, const Context &context);
MANTID_LEGACYNEXUS_DLL bool elementCount(std::span<const hsize_t> dimensions, std::size_t &result,
                                         const Context &context);
MANTID_LEGACYNEXUS_DLL bool elementCount(hid_t dataspace, hssize_t &result, const Context &context);

MANTID_LEGACYNEXUS_DLL bool readDataset(hid_t dataset, hid_t memoryType, hid_t memorySpace, hid_t fileSpace, void *data,
                                        const Context &context);
MANTID_LEGACYNEXUS_DLL bool readAttribute(hid_t attribute, hid_t memoryType, void *data, const Context &context);
MANTID_LEGACYNEXUS_DLL bool readScalarVlenDataset(hid_t dataset, hid_t datatype, hid_t dataspace, std::string &result,
                                                  const Context &context);
MANTID_LEGACYNEXUS_DLL bool readScalarVlenAttribute(hid_t attribute, hid_t datatype, std::string &result,
                                                    const Context &context);
MANTID_LEGACYNEXUS_DLL bool readVlenAttribute(hid_t attribute, hid_t datatype, hid_t dataspace,
                                              std::vector<std::string> &result, const Context &context);

MANTID_LEGACYNEXUS_DLL bool selectHyperslab(hid_t dataspace, std::span<const hsize_t> start,
                                            std::span<const hsize_t> size, const Context &context);

} // namespace Mantid::LegacyNexus::H5ErrorHandling
