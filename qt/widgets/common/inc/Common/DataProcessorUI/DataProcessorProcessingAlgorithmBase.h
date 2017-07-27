#ifndef MANTIDQTMANTIDWIDGETS_DATAPROCESSORALGORITHMBASE_H
#define MANTIDQTMANTIDWIDGETS_DATAPROCESSORALGORITHMBASE_H

#include "MantidAPI/AlgorithmManager.h"
#include "MantidQtMantidWidgets/WidgetDllOption.h"

#include <set>
#include <string>

namespace MantidQt {
namespace MantidWidgets {
/** @class DataProcessorProcessingAlgorithmBase

DataProcessorProcessingAlgorithmBase defines shared code to be used by derived
classes
(DataProcessorPreprocessingAlgorithm, DataProcessorProcessingAlgorithm and
DataProcessorPostprocessingAlgorithm).

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
class EXPORT_OPT_MANTIDQT_MANTIDWIDGETS DataProcessorProcessingAlgorithmBase {
public:
  // Default constructor
  DataProcessorProcessingAlgorithmBase();

  // Constructor
  DataProcessorProcessingAlgorithmBase(
      const std::string &name,
      const std::set<std::string> &blacklist = std::set<std::string>());

  // Destructor
  ~DataProcessorProcessingAlgorithmBase();

  // Returns the input workspaces properties defined for this algorithm
  virtual std::vector<std::string> getInputWsProperties() final;

  // Returns the input str list properties defined for this algorithm
  virtual std::vector<std::string> getInputStrListProperties() final;

  // Returns the output workspaces properties defined for this algorithm
  virtual std::vector<std::string> getOutputWsProperties() final;

  // Returns the name of this algorithm
  virtual std::string name() const final { return m_algName; };

  // Returns the blacklist
  virtual std::set<std::string> blacklist() const final { return m_blacklist; };

private:
  // Counts number of workspace properties
  void countWsProperties();

  // The name of this algorithm
  std::string m_algName;
  // The blacklist
  std::set<std::string> m_blacklist;
  // Input ws properties
  std::vector<std::string> m_inputWsProperties;
  // Input str list properties
  std::vector<std::string> m_inputStrListProperties;
  // Output ws properties
  std::vector<std::string> m_OutputWsProperties;

protected:
  // Converts a string to a vector of strings
  static std::vector<std::string>
  convertStringToVector(const std::string &text);
  // Converts a string to a set of strings
  static std::set<std::string> convertStringToSet(const std::string &text);
};
}
}
#endif /*MANTIDQTMANTIDWIDGETS_DATAPROCESSORALGORITHMBASE_H*/
