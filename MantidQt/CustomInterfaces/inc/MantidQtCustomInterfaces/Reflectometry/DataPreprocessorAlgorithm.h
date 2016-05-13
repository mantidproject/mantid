#ifndef MANTID_CUSTOMINTERFACES_DATAPREPROCESSORALGORITHM_H
#define MANTID_CUSTOMINTERFACES_DATAPREPROCESSORALGORITHM_H

#include "MantidAPI/AlgorithmManager.h"
#include "MantidQtCustomInterfaces/DllConfig.h"

#include <set>
#include <string>

namespace MantidQt {
namespace CustomInterfaces {
/** @class DataPreprocessorAlgorithm

DataPreprocessorAlgorithm defines a pre-processor algorithm that will be
responsible for pre-processsing a specific column in a Data Processor UI.

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
class MANTIDQT_CUSTOMINTERFACES_DLL DataPreprocessorAlgorithm {
public:
  // Constructor
  DataPreprocessorAlgorithm(
      const std::string &name, const std::string &prefix = "",
      const std::set<std::string> &blacklist = std::set<std::string>(),
      bool show = true);
  // Default constructor
  DataPreprocessorAlgorithm();
  // Destructor
  virtual ~DataPreprocessorAlgorithm();
  // The name of this algorithm
  std::string name() const;
  // The name of the lhs input property
  std::string lhsProperty() const;
  // The name of the rhs input property
  std::string rhsProperty() const;
  // The name of the output property
  std::string outputProperty() const;
  // The prefix to add to the output property
  std::string prefix() const;
  // If we want to show the info associated with this pre-processor
  bool show() const;
  // The blacklist
  std::set<std::string> blacklist() const;

private:
  // The name of this algorithm
  std::string m_name;
  // The prefix of the output workspace
  std::string m_prefix;
  // The name of the LHS input property
  std::string m_lhs;
  // The name of the RHS input property
  std::string m_rhs;
  // The name of the output proerty
  std::string m_outProperty;
  // The blacklist
  std::set<std::string> m_blacklist;
  // Indicates wheter or not the information will appear in the output ws name
  bool m_show;
};
}
}
#endif /*MANTID_CUSTOMINTERFACES_DATAPREPROCESSORALGORITHM_H*/
