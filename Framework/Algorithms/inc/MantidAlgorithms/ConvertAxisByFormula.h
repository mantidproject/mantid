// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2013 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include "MantidAPI/Workspace_fwd.h"
#include "MantidAlgorithms/ConvertUnits.h"
#include "MantidAlgorithms/DllConfig.h"

// forward declaration
namespace mu {
class Parser;
}

namespace Mantid {

namespace API {
class SpectrumInfo;
}

namespace Algorithms {
/** ConvertAxisByFormula : Performs a unit conversion based on a supplied
  formula
*/
class MANTID_ALGORITHMS_DLL ConvertAxisByFormula : public ConvertUnits {
public:
  const std::string name() const override;
  /// Summary of algorithms purpose
  const std::string summary() const override {
    return "Converts the X or Y axis of a MatrixWorkspace via a user defined "
           "math formula.";
  }

  int version() const override;
  const std::vector<std::string> seeAlso() const override { return {"ConvertUnits"}; }
  const std::string category() const override;

protected:
  const std::string workspaceMethodName() const override { return ""; }
  const std::string workspaceMethodInputProperty() const override { return ""; }

private:
  void init() override;
  void exec() override;

  /// A simple internal structure to hold information on variables
  class Variable {
  public:
    Variable(const std::string &name) : name(name), value(0.0), isGeometric(false) {}
    Variable(const std::string &name, bool isGeometric) : name(name), value(0.0), isGeometric(isGeometric) {}
    std::string name;
    double value;
    bool isGeometric;
  };
  using Variable_ptr = std::shared_ptr<Variable>;

  void setAxisValue(const double &value, std::vector<Variable_ptr> &variables);
  void calculateValues(mu::Parser &p, std::vector<double> &vec, std::vector<Variable_ptr> variables);
  void setGeometryValues(const API::SpectrumInfo &specInfo, const size_t index, std::vector<Variable_ptr> &variables);
  double evaluateResult(mu::Parser &p);
};

} // namespace Algorithms
} // namespace Mantid
