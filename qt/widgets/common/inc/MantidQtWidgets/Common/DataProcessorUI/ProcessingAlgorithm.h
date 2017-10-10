#ifndef MANTIDQTMANTIDWIDGETS_DATAPROCESSORPROCESSINGALGORITHM_H
#define MANTIDQTMANTIDWIDGETS_DATAPROCESSORPROCESSINGALGORITHM_H

#include "MantidQtWidgets/Common/DllOption.h"
#include "MantidQtWidgets/Common/DataProcessorUI/ProcessingAlgorithmBase.h"

#include <QString>

namespace MantidQt {
namespace MantidWidgets {
namespace DataProcessor {
/** @class ProcessingAlgorithm

ProcessingAlgorithm defines a processing algorithm that will
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
class EXPORT_OPT_MANTIDQT_COMMON ProcessingAlgorithm
    : public ProcessingAlgorithmBase {
public:
  ProcessingAlgorithm();
  // Constructor
  ProcessingAlgorithm(const QString &name, const std::vector<QString> &prefix,
                      const std::set<QString> &blacklist = std::set<QString>());
  // Delegating constructor
  ProcessingAlgorithm(const QString &name, const QString &prefix,
                      const QString &blacklist = "");
  // Destructor
  virtual ~ProcessingAlgorithm();
  // The number of output properties
  size_t numberOfOutputProperties() const;
  // The prefix for this output property
  QString prefix(size_t index) const;
  // The name of this input property
  QString inputPropertyName(size_t index) const;
  // The name of this output property
  QString outputPropertyName(size_t index) const;

private:
  // The prefix of the output workspace(s)
  std::vector<QString> m_prefix;
  // The names of the input workspace properties
  std::vector<QString> m_inputProperties;
  // The names of the output workspace properties
  std::vector<QString> m_outputProperties;
};
}
}
}
#endif /*MANTIDQTMANTIDWIDGETS_DATAPROCESSORPROCESSINGALGORITHM_H*/
