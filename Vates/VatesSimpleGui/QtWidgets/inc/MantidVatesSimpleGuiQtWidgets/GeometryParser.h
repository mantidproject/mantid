#ifndef GEOMETRYPARSER_H_
#define GEOMETRYPARSER_H_

#include "MantidVatesSimpleGuiQtWidgets/WidgetDllOption.h"

#include <Poco/DOM/AutoPtr.h>
#include <Poco/DOM/Document.h>

#include <string>

namespace Mantid
{
namespace Vates
{
namespace SimpleGui
{

class AxisInformation;

/**
 *
  This class takes the associated XML information from a given dataset and
  interrogates it for the current dataset axis information. An operator like
  the RebinnerCutter needs to be applied to the data before use. Otherwise,
  the dataset must provide the same information in a similar manner.

  @author Michael Reuter
  @date 24/05/2011

  Copyright &copy; 2011 ISIS Rutherford Appleton Laboratory, NScD Oak Ridge National Laboratory & European Spallation Source

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

  File change history is stored at: <https://github.com/mantidproject/mantid>
  Code Documentation is available at: <http://doxygen.mantidproject.org>
 */
class EXPORT_OPT_MANTIDVATES_SIMPLEGUI_QTWIDGETS GeometryParser {
public:
  /**
   * Default constructor.
   * @param xml the string containing the XML information to be parsed
   */
  GeometryParser(const char *xml);
  /// Default destructor.
  virtual ~GeometryParser() {};

  /// Insert a value into the time dimension XML.
  std::string addTDimValue(double time);
  /**
   * Parse the dataset XML for information on the dataset axis.
   * @param dimension the XML string containing the axis information
   * @return an axis information object containing the given information
   */
  AxisInformation *getAxisInfo(const std::string dimension);

private:
  /**
   * A private function that converts a string bound to a double.
   * @param val the axis bound to convert
   * @return the double representation of the bound
   */
  double convertBounds(Poco::XML::XMLString val);

  Poco::AutoPtr<Poco::XML::Document> pDoc; ///< A XML document handle
};

}
}
}

#endif // GEOMETRYPARSER_H_
