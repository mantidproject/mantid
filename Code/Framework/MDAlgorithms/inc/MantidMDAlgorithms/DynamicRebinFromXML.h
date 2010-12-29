#ifndef MD_MANTID_ALGORITHMS_DYNAMICREBINFROMXML_H
#define MD_MANTID_ALGORITHMS_DYNAMICREBINFROMXML_H

#include "MantidAPI/Algorithm.h"
#include "MantidMDAlgorithms/BoxImplicitFunction.h"

namespace Mantid
{

//Forward declaration.
namespace API
{
  class ImplicitFunction;
}

namespace Geometry
{
  class MDGeometryDescription;
}

namespace MDAlgorithms
{
/**
*  DynamicRebinFromXML Algorithm
*
*  This algorithm performs dynamic rebinning driven by the xml string passed as an input.
*
*  
*  @date 07/12/2010
*  @author Owen Arnold
*
*  Copyright &copy; 2010 ISIS Rutherford Appleton Laboratory & NScD Oak Ridge National Laboratory
*   
*  This file is part of Mantid.
*
*  Mantid is free software; you can redistribute it and/or modify
*  it under the terms of the GNU General Public License as published by
*  the Free Software Foundation; either version 3 of the License, or
*  (at your option) any later version.
*    
*  Mantid is distributed in the hope that it will be useful,
*  but WITHOUT ANY WARRANTY; without even the implied warranty of
*  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
*  GNU General Public License for more details.
*
*  You should have received a copy of the GNU General Public License
*  along with this program.  If not, see <http://www.gnu.org/licenses/>.
*
*  File change history is stored at: <https://svn.mantidproject.org/mantid/trunk/Code/Mantid>
*  Code Documentation is available at: <http://doxygen.mantidproject.org>    
*/

class DLLExport DynamicRebinFromXML : public API::Algorithm
{
public:
  /// Default constructor
  DynamicRebinFromXML();
  /// Default desctructor
  virtual ~DynamicRebinFromXML();

  virtual const std::string name() const { return "DynamicRebinFromXML"; } ///< @return the algorithms name
  virtual const std::string category() const { return "General"; } ///< @return the algorithms category
  virtual int version() const { return (1); } ///< @return version number of algorithm

protected:
      /// Get the name of the workspace from xml.
    virtual std::string getWorkspaceName(Poco::XML::Element* pRootElem) const;

    /// Get the file location of the workspace from xml.
    virtual std::string getWorkspaceLocation(Poco::XML::Element* pRootElem) const;

    /// Get the implicit function from xml.
    virtual Mantid::API::ImplicitFunction* getImplicitFunction(Poco::XML::Element* pRootElem) const;

    /// Get the geometry description from the xml.
    virtual Mantid::Geometry::MDGeometryDescription* getMDGeometryDescriptionWithoutCuts(Poco::XML::Element* pRootElem) const;

    /// Create a dimension from the xml.
    virtual Mantid::Geometry::IMDDimension* createDimension(Poco::XML::Element* dimensionXML) const;

    /// Current implementation of geometry description requires cut information associated with dimensions.
    virtual void ApplyImplicitFunctionToMDGeometryDescription(Mantid::Geometry::MDGeometryDescription* description, Mantid::API::ImplicitFunction* impFunction) const;

private:
  /// Initialise the Algorithm (declare properties)
  void init();
  /// Execute the Algorithm
  void exec();

};

typedef std::vector<boost::shared_ptr<Mantid::MDAlgorithms::BoxImplicitFunction> > boxVec;
typedef std::vector<boost::shared_ptr<Mantid::API::ImplicitFunction> > functionVec;


} // namespace Algorithms
} // namespace Mantid
#endif // MANTID_ALGORITHMS_CREATEWORKSPACE_H_