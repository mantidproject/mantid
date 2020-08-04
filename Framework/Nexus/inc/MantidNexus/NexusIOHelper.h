// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2007 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include "MantidIndexing/DllConfig.h"
#include <algorithm>
#include <boost/any.hpp>
#include <nexus/NeXusFile.hpp>
#include <numeric>
#include <string>
#include <utility>

#include <utility>

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
    throw std::runtime_error("NeXusIOHelper: Unknown type in Nexus file");     \
  }

std::pair<::NeXus::Info, bool>
checkIfOpenAndGetInfo(::NeXus::File &file, const std::string &&entry) {
  std::pair<::NeXus::Info, bool> info_and_close;
  info_and_close.second = false;
  if (!file.isDataSetOpen()) {
    file.openData(entry);
    info_and_close.second = true;
  }
  info_and_close.first = file.getInfo();
  return info_and_close;
}

/// Use the getData function to read the buffer into vector and close file if
/// needed
template <typename T>
void callGetData(::NeXus::File &file, std::vector<T> &buf,
                 const bool close_file) {
  file.getData(buf);
  if (close_file)
    file.closeData();
}

/// Use the getData function to read the buffer and close file if needed
template <typename T>
void callGetData(::NeXus::File &file, T &buf, const bool close_file) {
  file.getData(&buf);
  if (close_file)
    file.closeData();
}

/// Use the getSlab function to read the buffer and close file if needed
template <typename T>
void callGetSlab(::NeXus::File &file, std::vector<T> &buf,
                 const std::vector<int64_t> &start,
                 const std::vector<int64_t> &size, const bool close_file) {
  file.getSlab(buf.data(), start, size);
  if (close_file)
    file.closeData();
}

/** Templated function to read any type of vector and (potentially) convert it
 * to another type. If the two types are the same, the conversion is skipped.
 */
template <typename T, typename U>
void doReadNexusAnyVector(std::vector<T> &out, ::NeXus::File &file,
                          const size_t size, const bool close_file,
                          const bool allow_downcasting) {
  if (sizeof(T) < sizeof(U) && !allow_downcasting) {
    if (close_file)
      file.closeData();
    throw std::runtime_error(
        "Downcasting is forbidden in NeXusIOHelper::readNexusAnyVector");
  } else if constexpr (std::is_same<T, U>::value) {
    if (size > 0)
      callGetData(file, out, close_file);
  } else {
    if (size > 0) {
      std::vector<U> buf(size);
      callGetData(file, buf, close_file);
      std::transform(buf.begin(), buf.end(), out.begin(),
                     [](U a) -> T { return static_cast<T>(a); });
    }
  }
}

/// Read any type of vector and return it as a new vector.
template <typename T, typename U>
std::vector<T> readNexusAnyVector(::NeXus::File &file, const size_t size,
                                  const bool close_file,
                                  const bool allow_downcasting) {
  std::vector<T> vec(size);
  doReadNexusAnyVector<T, U>(vec, file, size, close_file, allow_downcasting);
  return vec;
}

/// Read any type of vector and store it into the provided buffer vector.
template <typename T, typename U>
void readNexusAnyVector(std::vector<T> &out, ::NeXus::File &file,
                        const size_t size, const bool close_file,
                        const bool allow_downcasting) {
  doReadNexusAnyVector<T, U>(out, file, size, close_file, allow_downcasting);
}

/** Templated function to read any type of slab and (potentially) convert it to
 * another type. If the two types are the same, the conversion is skipped.
 */
template <typename T, typename U>
void doReadNexusAnySlab(std::vector<T> &out, ::NeXus::File &file,
                        const std::vector<int64_t> &start,
                        const std::vector<int64_t> &size, const bool close_file,
                        const bool allow_downcasting) {
  if (sizeof(T) < sizeof(U) && !allow_downcasting) {
    if (close_file)
      file.closeData();
    throw std::runtime_error(
        "Downcasting is forbidden in NeXusIOHelper::readNexusAnySlab");
  } else if constexpr (std::is_same<T, U>::value) {
    if (size[0] > 0)
      callGetSlab(file, out, start, size, close_file);
  } else {
    if (size[0] > 0) {
      std::vector<U> buf(size[0]);
      callGetSlab(file, buf, start, size, close_file);
      std::transform(buf.begin(), buf.end(), out.begin(),
                     [](U a) -> T { return static_cast<T>(a); });
    }
  }
}

/// Read any type of slab and return it as a new vector.
template <typename T, typename U>
std::vector<T>
readNexusAnySlab(::NeXus::File &file, const std::vector<int64_t> &start,
                 const std::vector<int64_t> &size, const bool close_file,
                 const bool allow_downcasting) {
  std::vector<T> vec(size[0]);
  doReadNexusAnySlab<T, U>(vec, file, start, size, close_file,
                           allow_downcasting);
  return vec;
}

/// Read any type of slab and store it into the provided buffer vector.
template <typename T, typename U>
void readNexusAnySlab(std::vector<T> &out, ::NeXus::File &file,
                      const std::vector<int64_t> &start,
                      const std::vector<int64_t> &size, const bool close_file,
                      const bool allow_downcasting) {
  doReadNexusAnySlab<T, U>(out, file, start, size, close_file,
                           allow_downcasting);
}

/** Templated function to read any type of variable and (potentially) convert it
 * to another type. If the two types are the same, the conversion is skipped.
 */
template <typename T, typename U>
T readNexusAnyVariable(::NeXus::File &file, const bool close_file,
                       const bool allow_downcasting) {
  if (sizeof(T) < sizeof(U) && !allow_downcasting) {
    if (close_file)
      file.closeData();
    throw std::runtime_error(
        "Downcasting is forbidden in NeXusIOHelper::readAnyVariable");
  } else if constexpr (std::is_same<T, U>::value) {
    T buf;
    callGetData(file, buf, close_file);
    return buf;
  } else {
    U buf;
    callGetData(file, buf, close_file);
    return static_cast<T>(buf);
  }
}

} // end of anonymous namespace

/** Opens the data group if needed, finds the data type, computes the data size,
 * and calls readNexusAnyVector via the RUN_NEXUSIOHELPER_FUNCTION macro.
 */
template <typename T>
std::vector<T> readNexusVector(::NeXus::File &file,
                               const std::string &entry = "",
                               const bool allow_downcasting = false) {
  const auto info_and_close =
      checkIfOpenAndGetInfo(file, std::move(std::move(entry)));
  const auto dims = (info_and_close.first).dims;
  const auto total_size = std::accumulate(dims.begin(), dims.end(), int64_t{1},
                                          std::multiplies<>());
  RUN_NEXUSIOHELPER_FUNCTION((info_and_close.first).type, readNexusAnyVector,
                             file, total_size, info_and_close.second,
                             allow_downcasting);
}

/** Opens the data group if needed, finds the data type, computes the data size,
 * and calls readNexusAnyVector via the RUN_NEXUSIOHELPER_FUNCTION macro.
 * The provided output buffer is filled.
 */
template <typename T>
std::vector<T> readNexusVector(std::vector<T> &out, ::NeXus::File &file,
                               const std::string &entry = "",
                               const bool allow_downcasting = false) {
  const auto info_and_close =
      checkIfOpenAndGetInfo(file, std::move(std::move(entry)));
  const auto dims = (info_and_close.first).dims;
  const auto total_size = std::accumulate(dims.begin(), dims.end(), int64_t{1},
                                          std::multiplies<>());
  RUN_NEXUSIOHELPER_FUNCTION((info_and_close.first).type, readNexusAnyVector,
                             out, file, total_size, info_and_close.second,
                             allow_downcasting);
}

/** Opens the data group if needed, finds the data type, and calls
 * readNexusAnySlab via the RUN_NEXUSIOHELPER_FUNCTION macro.
 */
template <typename T>
std::vector<T> readNexusSlab(::NeXus::File &file, const std::string &entry,
                             const std::vector<int64_t> &start,
                             const std::vector<int64_t> &size,
                             const bool allow_downcasting = false) {
  const auto info_and_close =
      checkIfOpenAndGetInfo(file, std::move(std::move(entry)));
  RUN_NEXUSIOHELPER_FUNCTION((info_and_close.first).type, readNexusAnySlab,
                             file, start, size, info_and_close.second,
                             allow_downcasting);
}

/** Opens the data group if needed, finds the data type, and calls
 * readNexusAnySlab via the RUN_NEXUSIOHELPER_FUNCTION macro.
 * The provided output buffer is filled.
 */
template <typename T>
void readNexusSlab(std::vector<T> &out, ::NeXus::File &file,
                   const std::string &entry, const std::vector<int64_t> &start,
                   const std::vector<int64_t> &size,
                   const bool allow_downcasting = false) {
  const auto info_and_close =
      checkIfOpenAndGetInfo(file, std::move(std::move(entry)));
  RUN_NEXUSIOHELPER_FUNCTION((info_and_close.first).type, readNexusAnySlab, out,
                             file, start, size, info_and_close.second,
                             allow_downcasting);
}

template <typename T>
T readNexusValue(::NeXus::File &file, const std::string &entry = "",
                 const bool allow_downcasting = false) {
  const auto info_and_close =
      checkIfOpenAndGetInfo(file, std::move(std::move(entry)));
  RUN_NEXUSIOHELPER_FUNCTION((info_and_close.first).type, readNexusAnyVariable,
                             file, info_and_close.second, allow_downcasting);
}

} // namespace NeXusIOHelper
} // namespace NeXus
} // namespace Mantid
