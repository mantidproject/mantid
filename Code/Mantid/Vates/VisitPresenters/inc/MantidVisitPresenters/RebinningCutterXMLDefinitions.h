#ifndef REBINNINGCUTTERXMLDEFINITIONS_H_
#define REBINNINGCUTTERXMLDEFINITIONS_H_

#include <string>

namespace Mantid
{
namespace VATES
{

/**

 This type contains definitions that will be found in the xml schema for the rebinning instructions, but must be used in
 code as part of the peristance/fetching routines. This file provides a single location for definitions to aid future maintenance.

 @author Owen Arnold, Tessella plc
 @date 16/12/2010

 Copyright &copy; 2010 ISIS Rutherford Appleton Laboratory & NScD Oak Ridge National Laboratory

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

 File change history is stored at: <https://svn.mantidproject.org/mantid/trunk/Code/Mantid>
 Code Documentation is available at: <http://doxygen.mantidproject.org> */

//TODO: these definitions may be more appropriate in API/Geometry where they can also be used as part of the parsing.

class XMLDefinitions
{
public:
  //XML schema tag definitions for generating xml.
  static const std::string workspaceNameXMLTagStart;
  static const std::string workspaceNameXMLTagEnd;
  static const std::string workspaceLocationXMLTagStart;
  static const std::string workspaceLocationXMLTagEnd;
  static const std::string workspaceInstructionXMLTagStart;
  static const std::string workspaceInstructionXMLTagEnd;

  //XML schema tag definitions for finding xml.
  static const std::string workspaceNameElementName;
  static const std::string functionElementName;
  static const std::string workspaceLocationElementName;
  static const std::string workspaceGeometryElementName;

  //An id for recognising specific vtkFieldData objects on inbound and outbound datasets.
  static const std::string metaDataId;
  static const std::string signalName;
  static const std::string geometryNodeName;
  static const std::string geometryOperatorInfo;
};

}
}
#endif /* REBINNINGCUTTERXMLDEFINITIONS_H_ */
