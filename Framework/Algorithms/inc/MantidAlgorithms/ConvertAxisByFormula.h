#ifndef MANTID_ALGORITHMS_CONVERTAXISBYFORMULA_H_
#define MANTID_ALGORITHMS_CONVERTAXISBYFORMULA_H_

#include "MantidKernel/System.h"
#include "MantidAlgorithms/ConvertUnits.h"
#include "MantidAPI/Workspace_fwd.h"

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

  Copyright &copy; 2013 ISIS Rutherford Appleton Laboratory, NScD Oak Ridge
  National Laboratory & European Spallation Source

  This file is part of Mantid.

  Mantid is free software; you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation; either version 3 of the License, or
  (at your option) any later version.

  Mantid is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public License for more details.

  You should have received a copy of the GNU General Public License
  along with this program.  If not, see <http://www.gnu.org/licenses/>.

  File change history is stored at: <https://github.com/mantidproject/mantid>
  Code Documentation is available at: <http://doxygen.mantidproject.org>
*/
class DLLExport ConvertAxisByFormula : public ConvertUnits {
public:
  const std::string name() const override;
  /// Summary of algorithms purpose
  const std::string summary() const override {
    return "Converts the X or Y axis of a MatrixWorkspace via a user defined "
           "math formula.";
  }

  int version() const override;
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
    Variable(const std::string &name)
        : name(name), value(0.0), isGeometric(false) {}
    Variable(const std::string &name, bool isGeometric)
        : name(name), value(0.0), isGeometric(isGeometric) {}
    std::string name;
    double value;
    bool isGeometric;
  };
  using Variable_ptr = boost::shared_ptr<Variable>;

  void setAxisValue(const double &value, std::vector<Variable_ptr> &variables);
  void calculateValues(mu::Parser &p, std::vector<double> &vec,
                       std::vector<Variable_ptr> variables);
  void setGeometryValues(const API::SpectrumInfo &specInfo, const size_t index,
                         std::vector<Variable_ptr> &variables);
  double evaluateResult(mu::Parser &p);
};

} // namespace Algorithms
} // namespace Mantid

#endif /* MANTID_ALGORITHMS_CONVERTAXISBYFORMULA_H_ */
