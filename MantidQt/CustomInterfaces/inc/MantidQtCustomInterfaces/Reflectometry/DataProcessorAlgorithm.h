#ifndef MANTID_CUSTOMINTERFACES_DATAPROCESSORALGORITHM_H
#define MANTID_CUSTOMINTERFACES_DATAPROCESSORALGORITHM_H

#include "MantidQtCustomInterfaces/DllConfig.h"
#include "MantidQtCustomInterfaces/Reflectometry/DataProcessorAlgorithmBase.h"

namespace MantidQt {
namespace CustomInterfaces {
/** @class DataProcessorAlgorithm

DataProcessorAlgorithm defines a processing algorithm that will perform the
reduction in a Data Processor UI.

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
class MANTIDQT_CUSTOMINTERFACES_DLL DataProcessorAlgorithm
    : public DataProcessorAlgorithmBase {
public:
  DataProcessorAlgorithm() = delete;
  // Constructor
  DataProcessorAlgorithm(
      const std::string &name, const std::vector<std::string> &prefix,
      const std::set<std::string> &blacklist = std::set<std::string>());
  // Destructor
  virtual ~DataProcessorAlgorithm();
  // The number of output properties
  size_t numberOfOutputProperties() const;
  // The prefix for this output property
  std::string prefix(size_t index) const;
  // The name of this input property
  std::string inputPropertyName(size_t index) const;
  // The name of this output property
  std::string outputPropertyName(size_t index) const;

private:
  // The prefix of the output workspace(s)
  std::vector<std::string> m_prefix;
  // The names of the input workspace properties
  std::vector<std::string> m_inputProperties;
  // The names of the output workspace properties
  std::vector<std::string> m_outputProperties;
};
}
}
#endif /*MANTID_CUSTOMINTERFACES_DATAPREPROCESSORALGORITHM_H*/
