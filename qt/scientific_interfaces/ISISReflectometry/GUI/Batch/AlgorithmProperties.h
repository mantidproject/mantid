// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2019 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#ifndef MANTID_CUSTOMINTERFACES_ALGORITHMPROPERTIES_H_
#define MANTID_CUSTOMINTERFACES_ALGORITHMPROPERTIES_H_

#include "MantidAPI/IAlgorithm_fwd.h"
#include "MantidKernel/Strings.h"

#include <boost/optional.hpp>
#include <map>
#include <string>

namespace MantidQt {
namespace CustomInterfaces {
namespace AlgorithmProperties {

using AlgorithmRuntimeProps = std::map<std::string, std::string>;

// These convenience functions convert properties of various types into
// strings to set the relevant property in an AlgorithmRuntimeProps

void update(std::string const &property, std::string const &value,
            AlgorithmRuntimeProps &properties);
void update(std::string const &property,
            boost::optional<std::string> const &value,
            AlgorithmRuntimeProps &properties);

void update(std::string const &property, bool value,
            AlgorithmRuntimeProps &properties);

void update(std::string const &property, int value,
            AlgorithmRuntimeProps &properties);

void update(std::string const &property, size_t value,
            AlgorithmRuntimeProps &properties);

void update(std::string const &property, double value,
            AlgorithmRuntimeProps &properties);

void update(std::string const &property, boost::optional<double> const &value,
            AlgorithmRuntimeProps &properties);

void updateFromMap(AlgorithmRuntimeProps &properties,
                   std::map<std::string, std::string> const &parameterMap);

std::string getOutputWorkspace(Mantid::API::IAlgorithm_sptr algorithm,
                               std::string const &property);

template <typename VALUE_TYPE>
void update(std::string const &property, std::vector<VALUE_TYPE> const &values,
            AlgorithmRuntimeProps &properties) {
  if (values.size() < 1)
    return;

  auto value =
      Mantid::Kernel::Strings::simpleJoin(values.cbegin(), values.cend(), ", ");
  update(property, value, properties);
}
} // namespace AlgorithmProperties
} // namespace CustomInterfaces
} // namespace MantidQt

#endif // MANTID_CUSTOMINTERFACES_ALGORITHMPROPERTIES_H_
