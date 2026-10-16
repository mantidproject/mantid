// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2026 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#include "MantidNexus/H5ErrorHandling.h"

#include "MantidNexus/NexusException.h"
#include "MantidNexus/UniqueID.h"

#include <limits>
#include <sstream>

namespace Mantid::Nexus::H5ErrorHandling {
namespace {

[[noreturn]] void throwError(const Context &context, const std::string &reason) {
  std::ostringstream message;
  message << reason;
  if (!context.object.empty())
    message << " for " << context.object;
  throw Exception(message.str(), context.operation, context.filename);
}

void requireVariableString(hid_t datatype, const Context &context) {
  checkId(datatype, context);
  if (!checkTriState(H5Tis_variable_str(datatype), context))
    throwError(context, "HDF5 datatype is not a variable-length string");
}

void reclaimVlen(hid_t datatype, hid_t dataspace, void *data, const Context &context) {
  checkStatus(H5Dvlen_reclaim(datatype, dataspace, H5P_DEFAULT, data), context);
}

} // namespace

void checkStatus(const herr_t status, const Context &context) {
  if (status < 0)
    throwError(context, "HDF5 operation failed");
}

bool checkTriState(const htri_t status, const Context &context) {
  if (status < 0)
    throwError(context, "HDF5 query failed");
  return status > 0;
}

void checkId(const hid_t id, const Context &context) {
  if (id <= 0 || H5Iis_valid(id) <= 0)
    throwError(context, "HDF5 returned an invalid handle");
}

int rank(const hid_t dataspace, const Context &context) {
  checkId(dataspace, context);
  const int value = H5Sget_simple_extent_ndims(dataspace);
  if (value < 0)
    throwError(context, "Failed to determine HDF5 dataspace rank");
  return value;
}

DimVector dimensions(const hid_t dataspace, const Context &context) {
  const int dataRank = rank(dataspace, context);
  DimVector result(static_cast<std::size_t>(dataRank));
  if (dataRank > 0 && H5Sget_simple_extent_dims(dataspace, result.data(), nullptr) < 0)
    throwError(context, "Failed to determine HDF5 dataspace dimensions");
  return result;
}

std::size_t elementCount(const std::span<const dimsize_t> dimensions, const Context &context) {
  std::size_t count = 1;
  for (const auto dimension : dimensions) {
    if (count == 0)
      return 0;
    if (dimension > std::numeric_limits<std::size_t>::max() / count)
      throwError(context, "HDF5 dataspace element count exceeds the supported range");
    count *= static_cast<std::size_t>(dimension);
  }
  return count;
}

std::size_t elementCount(const hid_t dataspace, const Context &context) {
  const hssize_t count = H5Sget_simple_extent_npoints(dataspace);
  if (count < 0)
    throwError(context, "Failed to determine HDF5 dataspace element count");
  return static_cast<std::size_t>(count);
}

void readDataset(const hid_t dataset, const hid_t memoryType, const hid_t memorySpace, const hid_t fileSpace,
                 void *data, const Context &context) {
  checkId(dataset, context);
  checkId(memoryType, context);
  if (data == nullptr)
    throwError(context, "Cannot read an HDF5 dataset into a null buffer");
  checkStatus(H5Dread(dataset, memoryType, memorySpace, fileSpace, H5P_DEFAULT, data), context);
}

std::vector<std::optional<std::string>> readVlenDataset(const hid_t dataset, const hid_t datatype,
                                                        const hid_t dataspace, const Context &context) {
  checkId(dataset, context);
  checkId(dataspace, context);
  requireVariableString(datatype, context);
  const auto count = elementCount(dataspace, context);
  std::vector<char *> raw(count, nullptr);
  if (count > 0)
    readDataset(dataset, datatype, H5S_ALL, H5S_ALL, raw.data(), context);

  std::vector<std::optional<std::string>> result;
  result.reserve(count);
  try {
    for (const auto value : raw) {
      if (value == nullptr)
        result.emplace_back(std::nullopt);
      else
        result.emplace_back(std::string(value));
    }
  } catch (...) {
    if (count > 0)
      H5Dvlen_reclaim(datatype, dataspace, H5P_DEFAULT, raw.data());
    throw;
  }
  if (count > 0)
    reclaimVlen(datatype, dataspace, raw.data(), context);
  return result;
}

std::optional<std::string> readScalarVlenDataset(const hid_t dataset, const hid_t datatype, const hid_t dataspace,
                                                 const Context &context) {
  if (elementCount(dataspace, context) != 1)
    throwError(context, "Expected a scalar or single-element HDF5 string dataset");
  return readVlenDataset(dataset, datatype, dataspace, context).front();
}

std::optional<std::string> readScalarVlenAttribute(const hid_t attribute, const hid_t datatype,
                                                   const Context &context) {
  checkId(attribute, context);
  requireVariableString(datatype, context);
  UniqueID<&H5Sclose> dataspace{H5Aget_space(attribute)};
  checkId(dataspace, context);
  if (elementCount(dataspace, context) != 1)
    throwError(context, "Expected a scalar or single-element HDF5 string attribute");
  char *raw = nullptr;
  checkStatus(H5Aread(attribute, datatype, &raw), context);
  std::optional<std::string> result;
  if (raw != nullptr) {
    try {
      result = std::string(raw);
    } catch (...) {
      H5free_memory(raw);
      throw;
    }
    checkStatus(H5free_memory(raw), context);
  }
  return result;
}

void selectHyperslab(const hid_t dataspace, const std::span<const dimsize_t> start,
                     const std::span<const dimsize_t> size, const Context &context) {
  const auto extent = dimensions(dataspace, context);
  if (start.size() != extent.size() || size.size() != extent.size())
    throwError(context, "Hyperslab rank does not match the HDF5 dataspace rank");
  for (std::size_t i = 0; i < extent.size(); ++i) {
    if (start[i] > extent[i] || size[i] > extent[i] - start[i])
      throwError(context, "Hyperslab is outside the HDF5 dataspace extent");
  }
  checkStatus(H5Sselect_hyperslab(dataspace, H5S_SELECT_SET, start.data(), nullptr, size.data(), nullptr), context);
}

} // namespace Mantid::Nexus::H5ErrorHandling
