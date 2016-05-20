#include "MantidKernel/PropertyNexus.h"

#include <nexus/NeXusException.hpp>
#include <nexus/NeXusFile.hpp>

#include "MantidKernel/ArrayProperty.h"
#include "MantidKernel/DateAndTime.h"
#include "MantidKernel/PropertyWithValue.h"
#include "MantidKernel/Strings.h"
#include "MantidKernel/TimeSeriesProperty.h"

#include <boost/algorithm/string/split.hpp>
#include <boost/algorithm/string/trim.hpp>
#include <boost/scoped_array.hpp>

using namespace Mantid::Kernel;
using namespace ::NeXus;
using boost::algorithm::is_any_of;
using boost::algorithm::split;

#ifdef _WIN32
#pragma warning(push)
#pragma warning(disable : 4805)
#endif

namespace Mantid {
namespace Kernel {

namespace PropertyNexus {

//----------------------------------------------------------------------------------------------
/** Helper method to create a property
 *
 * @param file :: nexus file handle
 * @param name :: name of the property being created
 * @param times :: vector of times, empty = single property with value
 * @return Property *
 */
template <typename NumT>
Property *makeProperty(::NeXus::File *file, const std::string &name,
                       std::vector<Kernel::DateAndTime> &times) {
  std::vector<NumT> values;
  file->getData(values);
  if (times.empty()) {
    if (values.size() == 1) {
      return new PropertyWithValue<NumT>(name, values[0]);
    } else {
      return new ArrayProperty<NumT>(name, values);
    }
  } else {
    auto prop = new TimeSeriesProperty<NumT>(name);
    prop->addValues(times, values);
    return prop;
  }
}

/** Helper method to create a time series property from a boolean
*
* @param file :: nexus file handle
* @param name :: name of the property being created
* @param times :: vector of times, empty = single property with value
* @return Property *
*/
Property *makeTimeSeriesBoolProperty(::NeXus::File *file,
                                     const std::string &name,
                                     std::vector<Kernel::DateAndTime> &times) {
  std::vector<uint8_t> savedValues;
  file->getData(savedValues);
  const size_t nvals = savedValues.size();
  std::vector<bool> realValues(nvals);
  for (size_t i = 0; i < nvals; ++i) {
    realValues[i] = (savedValues[i] != 0);
  }
  auto prop = new TimeSeriesProperty<bool>(name);
  prop->addValues(times, realValues);
  return prop;
}

/** Make a string/vector\<string\> property */
Property *makeStringProperty(::NeXus::File *file, const std::string &name,
                             std::vector<Kernel::DateAndTime> &times) {
  std::vector<std::string> values;
  if (times.empty()) {
    std::string bigString = file->getStrData();
    return new PropertyWithValue<std::string>(name, bigString);
  } else {
    if (file->getInfo().dims.size() != 2)
      throw std::runtime_error("NXlog loading failed on field " + name +
                               ". Expected rank 2.");
    int64_t numStrings = file->getInfo().dims[0];
    int64_t span = file->getInfo().dims[1];
    boost::scoped_array<char> data(new char[numStrings * span]);
    file->getData(data.get());
    values.reserve(size_t(numStrings));
    for (int i = 0; i < numStrings; i++)
      values.push_back(std::string(data.get() + i * span));

    auto prop = new TimeSeriesProperty<std::string>(name);
    prop->addValues(times, values);
    return prop;
  }
}

//----------------------------------------------------------------------------------------------
/** Opens a NXlog group in a nexus file and
 * creates the correct Property object from it.
 *
 * @param file :: NXS file handle
 * @param group :: name of NXlog group to open
 * @return Property pointer
 */
Property *loadProperty(::NeXus::File *file, const std::string &group) {
  file->openGroup(group, "NXlog");

  // Times in second offsets
  std::vector<double> timeSec;
  std::string startStr = "";
  std::string unitsStr = "";

  // Get the entries so that you can check if the "time" field is present
  std::map<std::string, std::string> entries = file->getEntries();
  if (entries.find("time") != entries.end()) {
    file->openData("time");
    file->getData(timeSec);
    // Optionally get a start
    try {
      file->getAttr("start", startStr);
    } catch (::NeXus::Exception &) {
    }
    file->closeData();
  }

  // Check the type. Boolean stored as UINT8
  bool typeIsBool(false);
  // Check for boolean attribute
  if (file->hasAttr("boolean")) {
    typeIsBool = true;
  }

  std::vector<Kernel::DateAndTime> times;
  if (!timeSec.empty()) {
    // Use a default start time
    if (startStr.empty())
      startStr = "2000-01-01T00:00:00";
    // Convert time in seconds to DateAndTime
    DateAndTime start(startStr);
    times.reserve(timeSec.size());
    for (double time : timeSec) {
      times.push_back(start + time);
    }
  }

  file->openData("value");
  Property *retVal = nullptr;
  switch (file->getInfo().type) {
  case ::NeXus::FLOAT32:
    retVal = makeProperty<float>(file, group, times);
    break;
  case ::NeXus::FLOAT64:
    retVal = makeProperty<double>(file, group, times);
    break;
  case ::NeXus::INT32:
    retVal = makeProperty<int32_t>(file, group, times);
    break;
  case ::NeXus::UINT32:
    retVal = makeProperty<uint32_t>(file, group, times);
    break;
  case ::NeXus::INT64:
    retVal = makeProperty<int64_t>(file, group, times);
    break;
  case ::NeXus::UINT64:
    retVal = makeProperty<uint64_t>(file, group, times);
    break;
  case ::NeXus::CHAR:
    retVal = makeStringProperty(file, group, times);
    break;
  case ::NeXus::UINT8:
    if (typeIsBool)
      retVal = makeTimeSeriesBoolProperty(file, group, times);
    break;
  case ::NeXus::INT8:
  case ::NeXus::INT16:
  case ::NeXus::UINT16:
    retVal = nullptr;
    break;
  }

  if (file->hasAttr("units")) {
    try {
      file->getAttr("units", unitsStr);
    } catch (::NeXus::Exception &) {
    }
  }
  file->closeData();
  file->closeGroup();
  // add units
  if (retVal)
    retVal->setUnits(unitsStr);

  return retVal;
}

#ifdef _WIN32
#pragma warning(pop)
#endif

} // namespace PropertyNexus

} // namespace Mantid
} // namespace API
