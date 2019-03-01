// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2007 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#ifndef NEXUSIOHELPER_H
#define NEXUSIOHELPER_H

#include "MantidIndexing/DllConfig.h"
#include <algorithm>
#include <boost/any.hpp>
#include <nexus/NeXusFile.hpp>
#include <numeric>
#include <string>

namespace Mantid {
namespace NeXus {
namespace NeXusIOHelper {

namespace {

/// Macro to run a function depending on the type of data in the Nexus file
#define RUN_NEXUSIOHELPER_FUNCTION(type, func_name, ...)                       \
  switch (type) {                                                              \
  case ::NeXus::FLOAT32:                                                       \
    return func_name<T, float>(__VA_ARGS__);                                   \
  case ::NeXus::FLOAT64:                                                       \
    return func_name<T, double>(__VA_ARGS__);                                  \
  case ::NeXus::INT8:                                                          \
    return func_name<T, int8_t>(__VA_ARGS__);                                  \
  case ::NeXus::UINT8:                                                         \
    return func_name<T, uint8_t>(__VA_ARGS__);                                 \
  case ::NeXus::INT16:                                                         \
    return func_name<T, int16_t>(__VA_ARGS__);                                 \
  case ::NeXus::UINT16:                                                        \
    return func_name<T, uint16_t>(__VA_ARGS__);                                \
  case ::NeXus::INT32:                                                         \
    return func_name<T, int32_t>(__VA_ARGS__);                                 \
  case ::NeXus::UINT32:                                                        \
    return func_name<T, uint32_t>(__VA_ARGS__);                                \
  case ::NeXus::INT64:                                                         \
    return func_name<T, int64_t>(__VA_ARGS__);                                 \
  case ::NeXus::UINT64:                                                        \
    return func_name<T, uint64_t>(__VA_ARGS__);                                \
  default:                                                                     \
    throw std::runtime_error("Unknown type in Nexus file");                    \
  }

std::pair<::NeXus::Info, bool> checkIfOpenAndGetInfo(::NeXus::File &file,
                                                     std::string entry) {
  std::pair<::NeXus::Info, bool> info_and_close;
  info_and_close.second = false;
  if (!file.isDataSetOpen()) {
    file.openData(entry);
    info_and_close.second = true;
  }
  info_and_close.first = file.getInfo();
  return info_and_close;
}

/// Use the getData function to read the buffer and close file if needed
template <typename T>
void callGetData(::NeXus::File &file, std::vector<T> &buf, bool close_file) {
  file.getData(buf);
  if (close_file)
    file.closeData();
}

/// Use the getSlab function to read the buffer and close file if needed
template <typename T>
void callGetSlab(::NeXus::File &file, std::vector<T> &buf,
                 const std::vector<int64_t> &start,
                 const std::vector<int64_t> &size, bool close_file) {
  file.getSlab(buf.data(), start, size);
  if (close_file)
    file.closeData();
}

/** Templated function to read any type of data and store it into another vector
 * type. If the two types are the same, the conversion is skipped.
 */
template <typename T, typename U>
std::vector<T> readNexusAnyVector(::NeXus::File &file, size_t size,
                                  bool close_file) {
  if (sizeof(T) < sizeof(U)) {
    if (close_file)
      file.closeData();
    throw std::runtime_error(
        "Downcasting is forbidden in NeXusIOHelper::readNexusAnyVector");
  } else if (std::is_same<T, U>::value) {
    std::vector<T> buf(size);
    callGetData(file, buf, close_file);
    return buf;
  } else {
    std::vector<U> buf(size);
    std::vector<T> vec(size);
    callGetData(file, buf, close_file);
    std::transform(buf.begin(), buf.end(), vec.begin(),
                   [](U a) -> T { return static_cast<T>(a); });
    return vec;
  }
}

/** Templated function to read any type of slab and store it into another vector
 * type. If the two types are the same, the conversion is skipped.
 */
template <typename T, typename U>
std::vector<T>
readNexusAnySlab(::NeXus::File &file, const std::vector<int64_t> &start,
                 const std::vector<int64_t> &size, bool close_file) {
  if (sizeof(T) < sizeof(U)) {
    if (close_file)
      file.closeData();
    throw std::runtime_error(
        "Downcasting is forbidden in NeXusIOHelper::readNexusAnySlab");
  } else if (std::is_same<T, U>::value) {
    std::vector<T> buf(size[0]);
    callGetSlab(file, buf, start, size, close_file);
    return buf;
  } else {
    std::vector<U> buf(size[0]);
    std::vector<T> vec(size[0]);
    callGetSlab(file, buf, start, size, close_file);
    std::transform(buf.begin(), buf.end(), vec.begin(),
                   [](U a) -> T { return static_cast<T>(a); });
    return vec;
  }
}

} // end of anonymous namespace

/** Opens the data group if needed, finds the data type, computes the data size,
 * and calls readNexusAnyVector via the RUN_NEXUSIOHELPER_FUNCTION macro.
 */
template <typename T>
std::vector<T> readNexusVector(::NeXus::File &file, std::string entry = "") {
  auto info_and_close = checkIfOpenAndGetInfo(file, entry);
  auto dims = (info_and_close.first).dims;
  auto total_size = std::accumulate(dims.begin(), dims.end(), int64_t{1},
                                    std::multiplies<>());
  RUN_NEXUSIOHELPER_FUNCTION((info_and_close.first).type, readNexusAnyVector,
                             file, total_size, info_and_close.second);
}

/** Opens the data group if needed, finds the data type, and calls
 * readNexusAnySlab via the RUN_NEXUSIOHELPER_FUNCTION macro.
 */
template <typename T>
std::vector<T> readNexusSlab(::NeXus::File &file, std::string entry,
                             const std::vector<int64_t> &start,
                             const std::vector<int64_t> &size) {
  auto info_and_close = checkIfOpenAndGetInfo(file, entry);
  RUN_NEXUSIOHELPER_FUNCTION((info_and_close.first).type, readNexusAnySlab,
                             file, start, size, info_and_close.second);
}

} // namespace NeXusIOHelper
} // namespace NeXus
} // namespace Mantid

#endif /* NEXUSIOHELPER_H */
