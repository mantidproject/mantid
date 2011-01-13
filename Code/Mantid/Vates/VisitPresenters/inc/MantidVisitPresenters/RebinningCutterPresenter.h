#ifndef VATES_REBINNING_PRESENTER_H_
#define VATES_REBINNING_PRESENTER_H_

#include <vtkUnstructuredGrid.h>
#include <vtkBox.h>

#include "MDDataObjects/MDWorkspace.h"
#include <MantidAPI/ImplicitFunctionFactory.h>
#include <MantidAPI/ImplicitFunction.h>
#include <MantidMDAlgorithms/CompositeImplicitFunction.h>
#include <MantidGeometry/MDGeometry/IMDDimension.h>
#include <boost/shared_ptr.hpp>
#include <MantidVisitPresenters/RebinningXMLGenerator.h>

namespace Poco
{
namespace XML
{
class XMLElement;
}
}

namespace Mantid
{
namespace VATES
{
/**

 Applies indirection between for mappings between third-party visualisation framework and mantid.
 This type supports rebinning operations.

 @author Owen Arnold, Tessella plc
 @date 03/11/2010

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
 Code Documentation is available at: <http://doxygen.mantidproject.org>
 */


typedef std::vector<boost::shared_ptr<Mantid::Geometry::IMDDimension> > DimensionVec;
typedef boost::shared_ptr<Mantid::Geometry::IMDDimension> Dimension_sptr;

/// RebinningCutterPresenter does the work of implemening requests/information provided by pipeline filters. Generates new datasets from
/// current and historical rebinning knowledge accumulated in the pipeline.
class DLLExport RebinningCutterPresenter
{
private:

  /// Implicit function representing current and historical operations
  boost::shared_ptr<Mantid::API::ImplicitFunction> m_function;

  /// Pointer to the rebinned workspace.
  Mantid::MDDataObjects::MDWorkspace_sptr m_rebinnedWs;

  /// initalised flag
  bool m_initalized;

  /// Serializer to create and pass on rebinning metadata.
  RebinningXMLGenerator m_serializer;


public:

	/// Constructor
	RebinningCutterPresenter();

	/// Destructor
	~RebinningCutterPresenter();

	/// Get the generated function.
	boost::shared_ptr<const Mantid::API::ImplicitFunction> getFunction() const;

  /// Construct reduction knowledge objects
  void constructReductionKnowledge(
      DimensionVec dimensions,
      Dimension_sptr dimensionX,
      Dimension_sptr dimensionY,
      Dimension_sptr dimensionZ,
      Dimension_sptr dimensiont,
      const double width,
      const double height,
      const double depth,
      std::vector<double>& origin, vtkDataSet* inputDataSet);

  /// Apply reduction knowledge to create a vtk dataset.
  vtkDataSet* createVisualDataSet(const std::string& scalarName, bool isUnstructured, int timestep);

  Dimension_sptr getXDimensionFromXML(vtkDataSet* vtkDataSetInput) const;
  Dimension_sptr getYDimensionFromXML(vtkDataSet* vtkDataSetInput) const;
  Dimension_sptr getZDimensionFromXML(vtkDataSet* vtkDataSetInput) const;
  Dimension_sptr getTDimensionFromXML(vtkDataSet* vtkDataSetInput) const;
};

//Non-member helper functions.

  /// Rebin
  Mantid::MDDataObjects::MDWorkspace_sptr rebin(const RebinningXMLGenerator& serializingUtility);

  /// Save reduction knowledge object. Serialise to xml and pass to dependent filters.
  void persistReductionKnowledge(vtkDataSet * out_ds,
      const RebinningXMLGenerator& xmlGenerator, const char* id);

  /// Convert field data to xml string meta data.
  std::string fieldDataToMetaData(vtkFieldData* fieldData, const char* id);

  /// Look for and extract exisiting reduction knowledge in input visualisation dataset.
  Mantid::API::ImplicitFunction* findExistingRebinningDefinitions(vtkDataSet *in_ds, const char* id);

  //Get the workspace location from the xmlstring. xmlstring is present of vtkFieldData on vtkDataSet.
  std::string findExistingWorkspaceNameFromXML(vtkDataSet *in_ds, const char* id);

  //Get the workspace location from the xmlstring. xmlstring is present of vtkFieldData on vtkDataSet.
  std::string findExistingWorkspaceLocationFromXML(vtkDataSet *in_ds, const char* id);

  //Get the workspace geometry from the xmlstring. xmlstring is present of vtkFieldData on vtkDataSet.
  Poco::XML::Element* findExistingGeometryInformationFromXML(vtkDataSet* inputDataSet, const char* id);

  //Read xml and buid string from inner contents
  std::string readNestedNode(Poco::XML::Node* root);

  /// Converts field data into metadata xml/string.
  void metaDataToFieldData(vtkFieldData* fieldData, std::string metaData, const char* id);

  /// Construct an input MDWorkspace by loading from a file. This should be achieved via a seperate loading algorithm.
  Mantid::MDDataObjects::MDWorkspace_sptr constructMDWorkspace(const std::string& wsLocation);

  /// Create a geometry from dimensions and then serialise it.
  std::string constructGeometryXML(DimensionVec dimensions,
      Dimension_sptr dimensionX,
      Dimension_sptr dimensionY,
      Dimension_sptr dimensionZ,
      Dimension_sptr dimensiont,
      double height,
      double width,
      double depth,
      std::vector<double>& origin);

  vtkDataSet* generateVisualImage(Mantid::MDDataObjects::MDWorkspace_sptr rebinnedWs, const std::string& scalarName, bool isUnstructured, const int timestep);

  /// Create an unstructured vtk image, which is sparse and excludes empty signal values.
  vtkDataSet* generateVTKUnstructuredImage(Mantid::MDDataObjects::MDWorkspace_sptr spWorkspace, const std::string& scalarName, const int timestep);

  /// Create a structured vtk image, which is essentially dense.
  vtkDataSet* generateVTKStructuredImage(Mantid::MDDataObjects::MDWorkspace_sptr spWorkspace, const std::string& scalarName, const int timestep);

  /// Helper method to get dimensions from a geometry xml element.
  std::vector<boost::shared_ptr<Mantid::Geometry::IMDDimension> > getDimensions(Poco::XML::Element* geometryElement);

  /// helper method to get a dimension from a dimension xmlelement.
  Mantid::Geometry::IMDDimension* createDimension(Poco::XML::Element* dimensionXML);

}
}
#endif
