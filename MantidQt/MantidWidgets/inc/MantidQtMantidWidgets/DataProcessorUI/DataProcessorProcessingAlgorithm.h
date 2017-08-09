#ifndef MANTIDQTMANTIDWIDGETS_DATAPROCESSORPROCESSINGALGORITHM_H
#define MANTIDQTMANTIDWIDGETS_DATAPROCESSORPROCESSINGALGORITHM_H

#include "MantidQtMantidWidgets/WidgetDllOption.h"
#include "MantidQtMantidWidgets/DataProcessorUI/DataProcessorProcessingAlgorithmBase.h"

#include <QString>

namespace MantidQt {
namespace MantidWidgets {
/** @class DataProcessorProcessingAlgorithm

DataProcessorProcessingAlgorithm defines a processing algorithm that will
perform the
reduction in a Data ProcessorProcessing UI.

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
class EXPORT_OPT_MANTIDQT_MANTIDWIDGETS DataProcessorProcessingAlgorithm
    : public DataProcessorProcessingAlgorithmBase {
public:
  DataProcessorProcessingAlgorithm() = delete;
  // Constructor
  DataProcessorProcessingAlgorithm(
      const std::string &name, const std::vector<std::string> &prefix,
      const std::set<std::string> &blacklist = std::set<std::string>());
  // Delegating constructor
  DataProcessorProcessingAlgorithm(const QString &name, const QString &prefix,
                                   const QString &blacklist = "");
  // Destructor
  virtual ~DataProcessorProcessingAlgorithm();
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
#endif /*MANTIDQTMANTIDWIDGETS_DATAPROCESSORPROCESSINGALGORITHM_H*/
