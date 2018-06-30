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

  Copyright &copy; 2014 ISIS Rutherford Appleton Laboratory, NScD Oak Ridge
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
