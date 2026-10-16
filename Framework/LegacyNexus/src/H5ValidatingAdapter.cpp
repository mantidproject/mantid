// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2026 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#include "H5ValidatingAdapter.h"

#include "MantidLegacyNexus/napi.h"

#include <limits>
#include <sstream>

namespace Mantid::LegacyNexus::H5ValidatingAdapter {
namespace {

void report(const Context &context, const std::string &reason) {
  std::ostringstream message;
  message << "ERROR: " << reason;
  if (!context.operation.empty())
    message << " while " << context.operation;
  if (!context.object.empty())
    message << " (" << context.object << ')';
  NXReportError(message.str().c_str());
}

bool requireVariableString(const hid_t datatype, const Context &context) {
  if (!checkId(datatype, context))
    return false;
  bool isVariable = false;
  if (!checkTriState(H5Tis_variable_str(datatype), isVariable, context))
    return false;
  if (!isVariable)
    report(context, "HDF5 datatype is not a variable-length string");
  return isVariable;
}

} // namespace

bool checkStatus(const herr_t status, const Context &context) {
  if (status < 0) {
    report(context, "HDF5 operation failed");
    return false;
  }
  return true;
}

bool checkTriState(const htri_t status, bool &result, const Context &context) {
  if (status < 0) {
    report(context, "HDF5 query failed");
    return false;
  }
  result = status > 0;
  return true;
}

bool checkId(const hid_t id, const Context &context) {
  if (id <= 0 || H5Iis_valid(id) <= 0) {
    report(context, "HDF5 returned an invalid handle");
    return false;
  }
  return true;
}

bool rank(const hid_t dataspace, int &result, const Context &context) {
  if (!checkId(dataspace, context))
    return false;
  result = H5Sget_simple_extent_ndims(dataspace);
  if (result < 0) {
    report(context, "failed to determine HDF5 dataspace rank");
    return false;
  }
  return true;
}

bool dimensions(const hid_t dataspace, std::vector<hsize_t> &result, const Context &context) {
  int dataRank = 0;
  if (!rank(dataspace, dataRank, context))
    return false;
  result.assign(static_cast<std::size_t>(dataRank), 0);
  return dataRank == 0 || checkStatus(H5Sget_simple_extent_dims(dataspace, result.data(), nullptr), context);
}

bool elementCount(const std::span<const hsize_t> dimensions, std::size_t &result, const Context &context) {
  result = 1;
  for (const auto dimension : dimensions) {
    if (result == 0)
      return true;
    if (dimension > std::numeric_limits<std::size_t>::max() / result) {
      report(context, "HDF5 dataspace element count exceeds the supported range");
      return false;
    }
    result *= static_cast<std::size_t>(dimension);
  }
  return true;
}

bool elementCount(const hid_t dataspace, hssize_t &result, const Context &context) {
  if (!checkId(dataspace, context))
    return false;
  result = H5Sget_simple_extent_npoints(dataspace);
  if (result < 0) {
    report(context, "failed to determine HDF5 dataspace element count");
    return false;
  }
  return true;
}

bool readDataset(const hid_t dataset, const hid_t memoryType, const hid_t memorySpace, const hid_t fileSpace,
                 void *data, const Context &context) {
  if (!checkId(dataset, context) || !checkId(memoryType, context))
    return false;
  if (data == nullptr) {
    report(context, "cannot read an HDF5 dataset into a null buffer");
    return false;
  }
  return checkStatus(H5Dread(dataset, memoryType, memorySpace, fileSpace, H5P_DEFAULT, data), context);
}

bool readAttribute(const hid_t attribute, const hid_t memoryType, void *data, const Context &context) {
  if (!checkId(attribute, context) || !checkId(memoryType, context))
    return false;
  if (data == nullptr) {
    report(context, "cannot read an HDF5 attribute into a null buffer");
    return false;
  }
  return checkStatus(H5Aread(attribute, memoryType, data), context);
}

bool readScalarVlenDataset(const hid_t dataset, const hid_t datatype, const hid_t dataspace, std::string &result,
                           const Context &context) {
  hssize_t count = 0;
  if (!checkId(dataset, context) || !requireVariableString(datatype, context) ||
      !elementCount(dataspace, count, context))
    return false;
  if (count != 1) {
    report(context, "expected a scalar or single-element HDF5 string dataset");
    return false;
  }

  char *raw = nullptr;
  if (!readDataset(dataset, datatype, H5S_ALL, H5S_ALL, &raw, context))
    return false;
  try {
    result = raw == nullptr ? std::string{} : std::string(raw);
  } catch (...) {
    if (raw != nullptr)
      H5free_memory(raw);
    report(context, "failed to copy variable-length string dataset");
    return false;
  }
  if (raw != nullptr && !checkStatus(H5free_memory(raw), context))
    return false;
  return true;
}

bool readScalarVlenAttribute(const hid_t attribute, const hid_t datatype, std::string &result, const Context &context) {
  if (!checkId(attribute, context) || !requireVariableString(datatype, context))
    return false;
  const hid_t dataspace = H5Aget_space(attribute);
  if (!checkId(dataspace, context))
    return false;
  hssize_t count = 0;
  const bool countValid = elementCount(dataspace, count, context);
  const bool closeSucceeded = checkStatus(H5Sclose(dataspace), context);
  if (!countValid || !closeSucceeded)
    return false;
  if (count != 1) {
    report(context, "expected a scalar or single-element HDF5 string attribute");
    return false;
  }

  char *raw = nullptr;
  if (!readAttribute(attribute, datatype, &raw, context))
    return false;
  try {
    result = raw == nullptr ? std::string{} : std::string(raw);
  } catch (...) {
    if (raw != nullptr)
      H5free_memory(raw);
    report(context, "failed to copy variable-length string attribute");
    return false;
  }
  if (raw != nullptr && !checkStatus(H5free_memory(raw), context))
    return false;
  return true;
}

bool readVlenAttribute(const hid_t attribute, const hid_t datatype, const hid_t dataspace,
                       std::vector<std::string> &result, const Context &context) {
  hssize_t signedCount = 0;
  if (!checkId(attribute, context) || !checkId(dataspace, context) || !requireVariableString(datatype, context) ||
      !elementCount(dataspace, signedCount, context))
    return false;
  const auto count = static_cast<std::size_t>(signedCount);
  std::vector<char *> raw(count, nullptr);
  if (count > 0 && !readAttribute(attribute, datatype, raw.data(), context))
    return false;
  try {
    result.clear();
    result.reserve(count);
    for (const auto value : raw)
      result.emplace_back(value == nullptr ? std::string{} : std::string(value));
  } catch (...) {
    if (count > 0)
      H5Dvlen_reclaim(datatype, dataspace, H5P_DEFAULT, raw.data());
    report(context, "failed to copy variable-length string attribute");
    return false;
  }
  return count == 0 || checkStatus(H5Dvlen_reclaim(datatype, dataspace, H5P_DEFAULT, raw.data()), context);
}

bool selectHyperslab(const hid_t dataspace, const std::span<const hsize_t> start, const std::span<const hsize_t> size,
                     const Context &context) {
  std::vector<hsize_t> extent;
  if (!dimensions(dataspace, extent, context))
    return false;
  if (start.size() != extent.size() || size.size() != extent.size()) {
    report(context, "hyperslab rank does not match the HDF5 dataspace rank");
    return false;
  }
  for (std::size_t i = 0; i < extent.size(); ++i) {
    if (start[i] > extent[i] || size[i] > extent[i] - start[i]) {
      report(context, "hyperslab is outside the HDF5 dataspace extent");
      return false;
    }
  }
  return checkStatus(H5Sselect_hyperslab(dataspace, H5S_SELECT_SET, start.data(), nullptr, size.data(), nullptr),
                     context);
}

} // namespace Mantid::LegacyNexus::H5ValidatingAdapter
