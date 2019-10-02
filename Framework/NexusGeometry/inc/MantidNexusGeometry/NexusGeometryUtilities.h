// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2019 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#ifndef MANTID_NEXUSGEOMETRY_NEXUSGEOMETRYUTILITIES_H_
#define MANTID_NEXUSGEOMETRY_NEXUSGEOMETRYUTILITIES_H_

#include "MantidNexusGeometry/DllConfig.h"
#include <H5Cpp.h>
#include <boost/optional.hpp>
#include <vector>

namespace Mantid {
namespace NexusGeometry {

// Utilities common for parsing and saving
namespace utilities {
template <typename T, typename R>
std::vector<R> convertVector(const std::vector<T> &toConvert) {
  std::vector<R> target(toConvert.size());
  for (size_t i = 0; i < target.size(); ++i) {
    target[i] = R(toConvert[i]);
  }
  return target;
}

boost::optional<H5::DataSet> findDataset(const H5::Group &parentGroup,
                                         const H5std_string &name);

boost::optional<H5::Group> findGroup(const H5::Group &parentGroup,
                                     const H5std_string &classType);

H5::Group findGroupOrThrow(const H5::Group &parentGroup,
                           const H5std_string &classType);

std::vector<H5::Group> findGroups(const H5::Group &parentGroup,
                                  const H5std_string &classType);

boost::optional<H5::Group> findGroupByName(const H5::Group &parentGroup,
                                           const H5std_string &name);
bool hasNXAttribute(const H5::Group &group, const std::string &attributeValue);
bool isNamed(const H5::H5Object &obj, const std::string &name);
} // namespace utilities
} // namespace NexusGeometry
} // namespace Mantid

#endif /* MANTID_NEXUSGEOMETRY_NEXUSGEOMETRYUTILITIES_H_ */
