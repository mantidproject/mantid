#ifndef MANTIDQTMANTIDWIDGETS_DATAPROCESSORVECTORSTRING_H
#define MANTIDQTMANTIDWIDGETS_DATAPROCESSORVECTORSTRING_H

/** @class VectorString

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

#include <QString>
#include <QStringList>
#include <sstream>
#include <vector>

namespace MantidQt {
namespace MantidWidgets {
namespace DataProcessor {

namespace {
template <typename A>
std::vector<std::string>
toStdStringVector(std::vector<QString, A> const &inVec) {
  std::vector<std::string> outVec;
  std::transform(
      inVec.begin(), inVec.end(), std::back_inserter(outVec),
      [](QString const &in) -> std::string { return in.toStdString(); });
  return outVec;
}
} // namespace

/**
Create string of comma separated list of items from a vector
@param items : Values in the list.
@return The comma separated list of items.
*/
template <typename T, typename A>
QString vectorString(const std::vector<T, A> &items) {
  std::ostringstream vector_string;
  const char *separator = "";
  for (auto paramIt = items.begin(); paramIt != items.end(); ++paramIt) {
    vector_string << separator << *paramIt;
    separator = ", ";
  }
  return QString::fromStdString(vector_string.str());
}

template <typename A>
QString vectorString(const std::vector<QString, A> &items) {
  return vectorString(toStdStringVector(items));
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
} // namespace DataProcessor
} // namespace MantidWidgets
} // namespace MantidQt
#endif // MANTIDQTMANTIDWIDGETS_DATAPROCESSORVECTORSTRING_H
