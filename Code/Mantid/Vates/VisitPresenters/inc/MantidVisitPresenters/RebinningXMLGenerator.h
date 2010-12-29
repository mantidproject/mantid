#ifndef VATES_REBINNING_XML_GENERATOR_H
#define VATES_REBINNING_XML_GENERATOR_H
#include <boost/shared_ptr.hpp>
#include <string>

namespace Mantid
{
/// Forward Declarations;
namespace API
{
class ImplicitFunction;
class IMDWorkspace;
}

namespace VATES
{
/**

 This type assists with the generation of well-formed xml meeting the xsd scehema layed-out for
 Rebinning/cutting type operations. The individual components utilised here may not be able to form well-formed
 xml in their own right and therefore do not have a toXMLString method.

 This implementation is based on a builder pattern using the create mechanism for xml string generation.

 @author Owen Arnold, Tessella plc
 @date 14/12/2010

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

class RebinningXMLGenerator
{

private:

  boost::shared_ptr<const Mantid::API::ImplicitFunction>  m_spFunction;
  std::string m_wsLocationXML;
  std::string m_wsNameXML;
  std::string m_wsLocation;
  std::string m_wsName;
  std::string m_geomXML;


public:

  /// Set the implicit function to use called.
  void setImplicitFunction(boost::shared_ptr<const Mantid::API::ImplicitFunction> spFunction);

  /// Set the workspace name to apply.
  void setWorkspace(boost::shared_ptr<const Mantid::API::IMDWorkspace> workspace);

  /// Set the workspace name to apply.
  void setWorkspaceName(std::string wsName);

  /// Set the location to apply.
  void setWorkspaceLocation(std::string wsLocation);

  /// Set the geometry xml to apply.
  void setGeometryXML(std::string geomXML);

  /// Create the xml string correponding to the set values.
  std::string createXMLString() const;

  /// Get the underlying workspace location.
  const std::string& getWorkspaceLocation() const;

  /// Get the underlying workspace name.
  const std::string& getWorkspaceName() const;
};




}
}
#endif
