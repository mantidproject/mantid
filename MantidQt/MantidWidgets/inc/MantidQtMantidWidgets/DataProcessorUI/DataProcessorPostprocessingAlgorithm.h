#ifndef MANTIDQTMANTIDWIDGETS_DATAPROCESSORPOSTPROCESSINGALGORITHM_H
#define MANTIDQTMANTIDWIDGETS_DATAPROCESSORPOSTPROCESSINGALGORITHM_H

#include "MantidQtMantidWidgets/WidgetDllOption.h"
#include "MantidQtMantidWidgets/DataProcessorUI/DataProcessorProcessingAlgorithmBase.h"

#include <QString>

namespace MantidQt {
namespace MantidWidgets {
/** @class DataPostprocessorAlgorithm

DataPostprocessorAlgorithm defines a post-processor algorithm responsible for
post-processing rows belonging to the same group in a Data Processor UI.

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
class EXPORT_OPT_MANTIDQT_MANTIDWIDGETS DataProcessorPostprocessingAlgorithm
    : public DataProcessorProcessingAlgorithmBase {
public:
  // Constructor
  DataProcessorPostprocessingAlgorithm(
      const std::string &name, const std::string &prefix = "",
      const std::set<std::string> &blacklist = std::set<std::string>());
  // Delegating constructor
  DataProcessorPostprocessingAlgorithm(const QString &name,
                                       const QString &prefix,
                                       const QString &blacklist);
  // Default constructor
  DataProcessorPostprocessingAlgorithm();
  // Destructor
  virtual ~DataProcessorPostprocessingAlgorithm();
  // The name of the input workspace property
  std::string inputProperty() const;
  // The name of the output workspace property
  std::string outputProperty() const;
  // The number of output workspace properties (currently only 1)
  size_t numberOfOutputProperties() const;
  // The prefix of the output property
  std::string prefix() const;

private:
  // The prefix of the output workspace
  std::string m_prefix;
  // The name of the input property
  std::string m_inputProp;
  // The name of the output property
  std::string m_outputProp;
};
}
}
#endif /*MANTIDQTMANTIDWIDGETS_DATAPROCESSORPOSTPROCESSINGALGORITHM_H*/
