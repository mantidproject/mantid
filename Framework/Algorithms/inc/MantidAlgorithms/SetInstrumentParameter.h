// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2014 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#ifndef MANTID_ALGORITHMS_SETINSTRUMENTPARAMETER_H_
#define MANTID_ALGORITHMS_SETINSTRUMENTPARAMETER_H_

#include "MantidAPI/Algorithm.h"
#include "MantidKernel/System.h"

namespace Mantid {

namespace Geometry {
class ParameterMap;
class IComponent;
} // namespace Geometry

namespace Algorithms {

/** SetInstrumentParameter : A simple algorithm to add or set the value of an
  instrument parameter
*/
class DLLExport SetInstrumentParameter : public API::Algorithm {
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

  void addParameter(Mantid::Geometry::ParameterMap &pmap,
                    const Mantid::Geometry::IComponent *cmptId,
                    const std::string &paramName, const std::string &paramType,
                    const std::string &paramValue) const;
};

} // namespace Algorithms
} // namespace Mantid

#endif /* MANTID_ALGORITHMS_SETINSTRUMENTPARAMETER_H_ */
