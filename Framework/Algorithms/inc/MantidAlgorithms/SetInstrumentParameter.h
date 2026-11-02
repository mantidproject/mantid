// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2014 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include "MantidAPI/Algorithm.h"
#include "MantidAlgorithms/DllConfig.h"

#include <string>
#include <vector>

namespace Mantid {

namespace Geometry {
class ComponentInfo;
} // namespace Geometry

namespace Algorithms {

/** SetInstrumentParameter : A simple algorithm to add or set the value of an
  instrument parameter
*/
class MANTID_ALGORITHMS_DLL SetInstrumentParameter final : public API::Algorithm {
public:
  const std::string name() const override;
  /// Summary of algorithms purpose
  const std::string summary() const override {
    return "Add or replace an parameter attached to an instrument component.";
  }

  int version() const override;
  const std::vector<std::string> seeAlso() const override {
    return {"RotateInstrumentComponent", "MoveInstrumentComponent"};
  }
  const std::string category() const override;
  std::map<std::string, std::string> validateInputs() override;

private:
  void init() override;
  void exec() override;

  static std::vector<size_t> componentsWithName(const Mantid::Geometry::ComponentInfo &componentInfo,
                                                const std::string &name);
  void addParameter(Mantid::Geometry::ComponentInfo &componentInfo, const size_t componentIndex,
                    const std::string &paramName, const std::string &paramType, const std::string &paramValue) const;
};

} // namespace Algorithms
} // namespace Mantid
