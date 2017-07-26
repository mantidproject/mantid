#ifndef MANTIDQTMANTIDWIDGETS_DATAPROCESSORVECTORSTRING_H
#define MANTIDQTMANTIDWIDGETS_DATAPROCESSORVECTORSTRING_H

/** @class DataProcessorVectorString

    Copyright &copy; 2007-8 ISIS Rutherford Appleton Laboratory, NScD Oak Ridge
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

#include <QStringList>
#include <QString>
#include <sstream>
#include <vector>

namespace MantidQt {
namespace MantidWidgets {

/**
Create string of comma separated list of values from a vector
@param param_vec : vector of values
@return string of comma separated list of values
*/
template <typename T, typename A>
QString vectorString(const std::vector<T, A> &param_vec) {
  std::ostringstream vector_string;
  const char* separator = "";
  for (auto paramIt = param_vec.begin(); paramIt != param_vec.end();
       ++paramIt) {
    vector_string << separator << *paramIt;
    separator = ", ";
  }
  return QString::fromStdString(vector_string.str());
}

QString vectorString(const QStringList &param);

/**
Create string of comma separated list of parameter values from a vector
@param param_name : name of the parameter we are creating a list of
@param param_vec : vector of parameter values
@return string of comma separated list of parameter values
*/
template <typename T, typename A>
QString vectorParamString(const QString &param_name,
                          const std::vector<T, A> &param_vec) {
  auto param_vector_string = param_name + " = '";
  param_vector_string += vectorString(param_vec);
  param_vector_string += "'";
  return param_vector_string;
}
}
}

#endif // MANTIDQTMANTIDWIDGETS_DATAPROCESSORVECTORSTRING_H
