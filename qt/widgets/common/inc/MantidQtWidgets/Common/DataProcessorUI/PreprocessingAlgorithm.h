#ifndef MANTIDQTMANTIDWIDGETS_DATAPROCESSORPREPROCESSINGALGORITHM_H
#define MANTIDQTMANTIDWIDGETS_DATAPROCESSORPREPROCESSINGALGORITHM_H

#include "MantidQtWidgets/Common/DataProcessorUI/ProcessingAlgorithmBase.h"
#include "MantidQtWidgets/Common/DllOption.h"
#include <QString>

namespace MantidQt {
namespace MantidWidgets {
namespace DataProcessor {
/** @class PreprocessingAlgorithm

PreprocessingAlgorithm defines a pre-processor algorithm that will
be
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
class EXPORT_OPT_MANTIDQT_COMMON PreprocessingAlgorithm
    : public ProcessingAlgorithmBase {
public:
  // Constructor
  PreprocessingAlgorithm(QString name, QString prefix = "",
                         QString separator = "",
                         std::set<QString> blacklist = std::set<QString>());
  // Delegating constructor
  PreprocessingAlgorithm(QString name, QString prefix, QString separator,
                         const QString &blacklist);
  // Default constructor
  PreprocessingAlgorithm();
  // Destructor
  virtual ~PreprocessingAlgorithm();

  // The name of the lhs input property
  QString lhsProperty() const;
  // The name of the rhs input property
  QString rhsProperty() const;
  // The name of the output property
  QString outputProperty() const;
  // The prefix to add to the output property
  QString prefix() const;
  // The separator to use between values in the output property
  QString separator() const;

private:
  // A prefix to the name of the pre-processed output ws
  QString m_prefix;
  // A separator between values in the pre-processed output ws name
  QString m_separator;
  // The name of the LHS input property
  QString m_lhs;
  // The name of the RHS input property
  QString m_rhs;
  // The name of the output proerty
  QString m_outProperty;
};
}
}
}
#endif /*MANTIDQTMANTIDWIDGETS_DATAPROCESSORPREPROCESSINGALGORITHM_H*/
