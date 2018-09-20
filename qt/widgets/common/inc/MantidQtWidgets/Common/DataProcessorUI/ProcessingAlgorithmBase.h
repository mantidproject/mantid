#ifndef MANTIDQTMANTIDWIDGETS_DATAPROCESSORALGORITHMBASE_H
#define MANTIDQTMANTIDWIDGETS_DATAPROCESSORALGORITHMBASE_H

#include "MantidAPI/AlgorithmManager.h"
#include "MantidQtWidgets/Common/DllOption.h"

#include <QString>

#include <set>
#include <string>

namespace MantidQt {
namespace MantidWidgets {
namespace DataProcessor {
/** @class ProcessingAlgorithmBase

ProcessingAlgorithmBase defines shared code to be used by derived
classes
(PreprocessingAlgorithm, ProcessingAlgorithm and
PostprocessingAlgorithm).

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
class EXPORT_OPT_MANTIDQT_COMMON ProcessingAlgorithmBase {
public:
  // Default constructor
  ProcessingAlgorithmBase();

  // Constructor
  ProcessingAlgorithmBase(
      const QString &name,
      const std::set<QString> &blacklist = std::set<QString>());

  // Destructor
  ~ProcessingAlgorithmBase();

  // Returns the input workspaces properties defined for this algorithm
  virtual std::vector<QString> getInputWsProperties() final;

  // Returns the input str list properties defined for this algorithm
  virtual std::vector<QString> getInputStrListProperties() final;

  // Returns the output workspaces properties defined for this algorithm
  virtual std::vector<QString> getOutputWsProperties() final;

  // Returns the name of this algorithm
  virtual QString name() const final { return m_algName; };

  // Returns the blacklist
  virtual std::set<QString> blacklist() const final { return m_blacklist; };

private:
  // Counts number of workspace properties
  void countWsProperties();

  // The name of this algorithm
  QString m_algName;
  // The blacklist
  std::set<QString> m_blacklist;
  // Input ws properties
  std::vector<QString> m_inputWsProperties;
  // Input str list properties
  std::vector<QString> m_inputStrListProperties;
  // Output ws properties
  std::vector<QString> m_OutputWsProperties;

protected:
  // Converts a string to a vector of strings
  static std::vector<QString> convertStringToVector(const QString &text);
  // Converts a string to a set of strings
  static std::set<QString> convertStringToSet(const QString &text);
};
} // namespace DataProcessor
} // namespace MantidWidgets
} // namespace MantidQt
#endif /*MANTIDQTMANTIDWIDGETS_DATAPROCESSORALGORITHMBASE_H*/
