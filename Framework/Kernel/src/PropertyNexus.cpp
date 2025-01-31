// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#include "MantidKernel/PropertyNexus.h"
#include "MantidKernel/ArrayProperty.h"
#include "MantidKernel/DateAndTime.h"
#include "MantidKernel/Property.h"
#include "MantidKernel/PropertyWithValue.h"
#include "MantidKernel/TimeSeriesProperty.h"
#include "MantidNexusCpp/NeXusException.hpp"
#include "MantidNexusCpp/NeXusFile.hpp"

// PropertyWithValue implementation
#include "MantidKernel/PropertyWithValue.tcc"

#include <boost/algorithm/string/split.hpp>

#include <memory>

using namespace Mantid::Kernel;
using namespace ::NeXus;

#ifdef _WIN32
#pragma warning(push)
#pragma warning(disable : 4805)
#endif

namespace Mantid::Kernel::PropertyNexus {

namespace {
//----------------------------------------------------------------------------------------------
/** Helper method to create a property
 *
 * @param file :: nexus file handle
 * @param name :: name of the property being created
 * @param times :: vector of times, empty = single property with value
 * @return Property *
 */
template <typename NumT>
std::unique_ptr<Property> makeProperty(::NeXus::File *file, const std::string &name,
                                       const std::vector<Types::Core::DateAndTime> &times) {
  std::vector<NumT> values;
  file->getData(values);
  if (times.empty()) {
    if (values.size() == 1) {
      return std::make_unique<PropertyWithValue<NumT>>(name, values[0]);
    } else {
      return std::make_unique<ArrayProperty<NumT>>(name, std::move(values));
    }
  } else {
    auto prop = std::make_unique<TimeSeriesProperty<NumT>>(name);
    prop->addValues(times, values);
    return std::unique_ptr<Property>(std::move(prop));
  }
}

/** Helper method to create a time series property from a boolean
 *
 * @param file :: nexus file handle
 * @param name :: name of the property being created
 * @param times :: vector of times, empty = single property with value
 * @return Property *
 */
std::unique_ptr<Property> makeTimeSeriesBoolProperty(::NeXus::File *file, const std::string &name,
                                                     const std::vector<Types::Core::DateAndTime> &times) {
  std::vector<uint8_t> savedValues;
  file->getData(savedValues);
  const size_t nvals = savedValues.size();
  std::vector<bool> realValues(nvals);
  for (size_t i = 0; i < nvals; ++i) {
    realValues[i] = (savedValues[i] != 0);
  }
  auto prop = std::make_unique<TimeSeriesProperty<bool>>(name);
  prop->addValues(times, realValues);
  return std::unique_ptr<Property>(std::move(prop));
}

/** Make a string/vector\<string\> property */
std::unique_ptr<Property> makeStringProperty(::NeXus::File *file, const std::string &name,
                                             const std::vector<Types::Core::DateAndTime> &times) {
  if (times.empty()) {
    std::string bigString = file->getStrData();
    return std::make_unique<PropertyWithValue<std::string>>(name, bigString);
  } else {
    if (file->getInfo().dims.size() != 2)
      throw std::runtime_error("NXlog loading failed on field " + name + ". Expected rank 2.");
    int64_t numStrings = file->getInfo().dims[0];
    int64_t span = file->getInfo().dims[1];
    auto data = std::make_unique<char[]>(numStrings * span);
    file->getData(data.get());
    std::vector<std::string> values;
    values.reserve(static_cast<size_t>(numStrings));
    for (int64_t i = 0; i < numStrings; i++)
      values.emplace_back(data.get() + i * span);

    auto prop = std::make_unique<TimeSeriesProperty<std::string>>(name);
    prop->addValues(times, values);
    return std::unique_ptr<Property>(std::move(prop));
  }
}
/**
 * Common function to populate "time" and "start" entries from NeXus file
 */
void getTimeAndStart(::NeXus::File *file, std::vector<double> &timeSec, std::string &startStr) {
  file->openData("time");
  file->getData(timeSec);
  // Optionally get a start
  try {
    file->getAttr("start", startStr);
  } catch (::NeXus::Exception &) {
  }
  file->closeData();
}

/**
 * @brief Common function used by loadProperty overloads populating start and units if required
 * @param file in
 * @param group  in
 * @param timeSec  in
 * @param startStr  out
 * @param unitsStr  out
 * @return std::unique_ptr<Property>
 */
std::unique_ptr<Property> loadPropertyCommon(::NeXus::File *file, const std::string &group,
                                             const std::vector<double> &timeSec, std::string &startStr) {
  std::vector<Types::Core::DateAndTime> times;
  if (!timeSec.empty()) {
    // Use a default start time
    if (startStr.empty())
      startStr = "2000-01-01T00:00:00";
    // Convert time in seconds to DateAndTime
    Types::Core::DateAndTime start(startStr);
    times.reserve(timeSec.size());
    std::transform(timeSec.cbegin(), timeSec.cend(), std::back_inserter(times),
                   [&start](const auto &time) { return start + time; });
  }

  file->openData("value");
  std::unique_ptr<Property> retVal = nullptr;
  switch (file->getInfo().type) {
  case NXnumtype::FLOAT32:
    retVal = makeProperty<float>(file, group, times);
    break;
  case NXnumtype::FLOAT64:
    retVal = makeProperty<double>(file, group, times);
    break;
  case NXnumtype::INT32:
    retVal = makeProperty<int32_t>(file, group, times);
    break;
  case NXnumtype::UINT32:
    retVal = makeProperty<uint32_t>(file, group, times);
    break;
  case NXnumtype::INT64:
    retVal = makeProperty<int64_t>(file, group, times);
    break;
  case NXnumtype::UINT64:
    retVal = makeProperty<uint64_t>(file, group, times);
    break;
  case NXnumtype::CHAR:
    retVal = makeStringProperty(file, group, times);
    break;
  case NXnumtype::UINT8: {
    // Check the type at the group level. Boolean stored as UINT8
    file->closeData();
    const bool typeIsBool = file->hasAttr("boolean");
    file->openData("value");

    if (typeIsBool)
      retVal = makeTimeSeriesBoolProperty(file, group, times);
    break;
  }
  case NXnumtype::INT8:
  case NXnumtype::INT16:
  case NXnumtype::UINT16:
    retVal = nullptr;
    break;
  case NXnumtype::BAD:
    throw std::runtime_error("Invalid data type found.");
    break;
  }

  // verifying that the attribute exists makes this very slow in NeXus v4.4.3
  // because of the change in how nxgetnextattr is implemented
  std::string unitsStr;
  try {
    file->getAttr("units", unitsStr);
  } catch (::NeXus::Exception &) {
    // let it drop on the floor
  }

  file->closeData();
  file->closeGroup();
  // add units
  if (retVal)
    retVal->setUnits(unitsStr);
  return retVal;
}

} // namespace
//----------------------------------------------------------------------------------------------

std::unique_ptr<Property> loadProperty(::NeXus::File *file, const std::string &group,
                                       const Mantid::Kernel::NexusHDF5Descriptor &fileInfo, const std::string &prefix) {
  file->openGroup(group, "NXlog");

  // Times in second offsets
  std::vector<double> timeSec;
  std::string startStr;

  // Check if the "time" field is present
  if (fileInfo.isEntry(prefix + "/" + group + "/time")) {
    getTimeAndStart(file, timeSec, startStr);
  }

  return loadPropertyCommon(file, group, timeSec, startStr);
}

//----------------------------------------------------------------------------------------------
/** Opens a NXlog group in a nexus file and
 * creates the correct Property object from it.
 *
 * @param file :: NXS file handle
 * @param group :: name of NXlog group to open
 * @return Property pointer
 */
std::unique_ptr<Property> loadProperty(::NeXus::File *file, const std::string &group) {
  file->openGroup(group, "NXlog");

  // Times in second offsets
  std::vector<double> timeSec;
  std::string startStr;

  // Get the entries so that you can check if the "time" field is present
  std::map<std::string, std::string> entries = file->getEntries();
  if (entries.find("time") != entries.end()) {
    getTimeAndStart(file, timeSec, startStr);
  }

  return loadPropertyCommon(file, group, timeSec, startStr);
}

#ifdef _WIN32
#pragma warning(pop)
#endif

} // namespace Mantid::Kernel::PropertyNexus
