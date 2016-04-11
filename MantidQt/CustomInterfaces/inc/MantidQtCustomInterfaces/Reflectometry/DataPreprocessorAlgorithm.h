#ifndef MANTID_CUSTOMINTERFACES_DATAPREPROCESSORALGORITHM_H
#define MANTID_CUSTOMINTERFACES_DATAPREPROCESSORALGORITHM_H

#include <string>

namespace MantidQt {
namespace CustomInterfaces {
/** @class DataPreprocessorAlgorithm

DataPreprocessorAlgorithm is an class which defines a pre-processor algorithm. A
pre-processor algorithm is defined by its name, the lhs and rhs input
properties, and the output property. Additionally, we can specify whether or not
we want to apply the options in the "Options" column.

Example 1: DataPreprocessorAlgorithm("Plus", "LHSWorkspace", "RHSWorkspace",
"OutputWorkspace", false) indicates that our pre-processor alg is "Plus", its
lhs input property is called "LHSWorkspace", its rhs input property is called
"RHSWorkspace" and its output property is called "OutputWorkspace".
Additionally, we don't want to apply "Options" to this pre-processor alg.

Example 2: DataPreprocessorAlgorithm("CreateTransmissionWorkspaceAuto",
"FirstTransmissionRun", "SecondTransmissionRun", "OutputWorkspace", true)
indicates that our pre-processor alg is called
"CreateTransmissionWorkspaceAuto", its lhs property is called
"FirstTransmissionRun", its rhs input property is called "SecondTransmissionRun"
and "Options" should be applied when possible. This is useful in Reflectometry
because "CreateTransmissionWorkspaceAuto" shares some of its input properties
with the Data Processor Algorithm ("ReflectometryReductionOneAuto") and
scientists often want to apply "Options" to both.

Copyright &copy; 2011-14 ISIS Rutherford Appleton Laboratory, NScD Oak Ridge
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

File change history is stored at: <https://github.com/mantidproject/mantid>.
Code Documentation is available at: <http://doxygen.mantidproject.org>
*/
class DataPreprocessorAlgorithm final {
public:
  /** Constructor
  * @param name : The name of the pre-processing algorithm
  * @param firstProperty : The name of the lhs input property
  * @param secondProperty : The name of the rhs input property
  * @param outputProperty : The name of the output property for this alg
  * @param applyOptions : Whether to apply options in the "Options" column or
  * not. This option is defaulted to false.
  */
  DataPreprocessorAlgorithm(const std::string &name,
                            const std::string &firstProperty,
                            const std::string &secondProperty,
                            const std::string &outputProperty,
                            bool applyOptions = false)
      : m_name(name), m_firstProperty(firstProperty),
        m_secondProperty(secondProperty), m_outputProperty(outputProperty),
        m_applyOptions(applyOptions){};
  // Destructor
  virtual ~DataPreprocessorAlgorithm(){};
  // The name of this pre-processor alg
  std::string name() const { return m_name; };
  // The name of the lhs input property
  std::string firstInputProperty() const { return m_firstProperty; };
  // The name of the rhs input property
  std::string secondInputProperty() const { return m_secondProperty; };
  // The name of the output property
  std::string outputProperty() const { return m_outputProperty; };
  // Whether to apply options to this pre-processor alg or not
  bool applyOptions() const { return m_applyOptions; };

private:
  std::string m_name;
  std::string m_firstProperty;
  std::string m_secondProperty;
  std::string m_outputProperty;
  bool m_applyOptions;
};
}
}
#endif /*MANTID_CUSTOMINTERFACES_DATAPREPROCESSORALGORITHM_H*/