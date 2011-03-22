#ifndef VATES_REBINNING_PRESENTER_H_
#define VATES_REBINNING_PRESENTER_H_

#include <vtkUnstructuredGrid.h>
#include <vtkBox.h>

#include "MDDataObjects/MDWorkspace.h"
#include <MantidVatesAPI/Common.h>
#include <MantidAPI/ImplicitFunctionFactory.h>
#include <MantidAPI/ImplicitFunction.h>
#include <MantidMDAlgorithms/CompositeImplicitFunction.h>
#include <MantidGeometry/MDGeometry/IMDDimension.h>
#include <boost/shared_ptr.hpp>
#include <MantidVatesAPI/RebinningXMLGenerator.h>
#include <MantidVatesAPI/ProgressAction.h>

#include <Poco/ActiveMethod.h>
#include <Poco/NotificationCenter.h>
#include <Poco/Notification.h>
#include <Poco/NObserver.h>
#include <MantidAPI/Algorithm.h>

//Forward declarations
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

/// Flags what should be don on the current iteration.
enum RebinningIterationAction {
  UseCache, //There is no delta here. Use a cached vtkDataSet.
  RecalculateVisualDataSetOnly, // 4D data set has not altered so create a new visual 3D slice only.
  RecalculateAll // Rebin and create 3D visualisation slice from 4D dataset.
};

/// Forward declarations
class vtkDataSetFactory;

/// RebinningCutterPresenter does the work of implemening requests/information provided by pipeline filters. Generates new datasets from
/// current and historical rebinning knowledge accumulated in the pipeline.
class DLLExport RebinningCutterPresenter
{
private:

  /// Implicit function representing current and historical operations
  boost::shared_ptr<Mantid::API::ImplicitFunction> m_function;

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
	boost::shared_ptr<Mantid::API::ImplicitFunction> getFunction() const;

  /// Construct reduction knowledge objects, specifically for VisIT, where all setup is per-request.
	void constructReductionKnowledge(
      DimensionVec dimensions,
      Dimension_sptr dimensionX,
      Dimension_sptr dimensionY,
      Dimension_sptr dimensionZ,
      Dimension_sptr dimensiont,
      Mantid::MDAlgorithms::CompositeImplicitFunction* compositeFunction,
      vtkDataSet* inputDataSet);

	/// Construct reduction knowledge objects. This is done per pipeline-execution.
	void constructReductionKnowledge(
	    DimensionVec dimensions,
	    Dimension_sptr dimensionX,
	    Dimension_sptr dimensionY,
	    Dimension_sptr dimensionZ,
	    Dimension_sptr dimensiont,
	    vtkDataSet* inputDataSet);

	/// Add function knowledge, this is always done per request.
	void addFunctionKnowledge(Mantid::MDAlgorithms::CompositeImplicitFunction* compFunction, vtkDataSet* inputDataSet);

	Mantid::MDDataObjects::MDWorkspace_sptr applyRebinningAction(
	    Mantid::VATES::RebinningIterationAction action
	    ,Mantid::VATES::ProgressAction& eventHandler) const;

  /// Apply reduction knowledge to create a vtk dataset.
  vtkDataSet* createVisualDataSet(boost::shared_ptr<vtkDataSetFactory> spvtkDataSetFactory);

  /// Get the x dimension from vtkDataSet field data.
  Dimension_sptr getXDimensionFromDS(vtkDataSet* vtkDataSetInput) const;

  /// Get the y dimension from vtkDataSet field data.
  Dimension_sptr getYDimensionFromDS(vtkDataSet* vtkDataSetInput) const;

  /// Get the z dimension from vtkDataSet field data.
  Dimension_sptr getZDimensionFromDS(vtkDataSet* vtkDataSetInput) const;

  /// Get the t dimension from vtkDataSet field data.
  Dimension_sptr getTDimensionFromDS(vtkDataSet* vtkDataSetInput) const;

  /// Get the workspace geometry as an xml string.
  const std::string& getWorkspaceGeometry() const;

  /// Verify that construct method has been called before anything else.
  void VerifyInitalization() const;
};

//Non-member helper functions.

  /// Save reduction knowledge object. Serialise to xml and pass to dependent filters.
  void persistReductionKnowledge(vtkDataSet * out_ds,
      const RebinningXMLGenerator& xmlGenerator, const char* id);

  /// Look for and extract exisiting reduction knowledge in input visualisation dataset.
  DLLExport Mantid::API::ImplicitFunction* findExistingRebinningDefinitions(vtkDataSet *in_ds, const char* id);

  //Get the workspace location from the xmlstring. xmlstring is present of vtkFieldData on vtkDataSet.
  DLLExport std::string findExistingWorkspaceName(vtkDataSet *in_ds, const char* id);

  //Get the workspace location from the xmlstring. xmlstring is present of vtkFieldData on vtkDataSet.
  DLLExport std::string findExistingWorkspaceLocation(vtkDataSet *in_ds, const char* id);

  //Get the workspace geometry from the xmlstring. xmlstring is present of vtkFieldData on vtkDataSet.
  DLLExport Poco::XML::Element* findExistingGeometryInformation(vtkDataSet* inputDataSet, const char* id);

  /// Construct an input MDWorkspace by loading from a file. This should be achieved via a seperate loading algorithm.
  Mantid::MDDataObjects::MDWorkspace_sptr constructMDWorkspace(const std::string& wsLocation);

  /// Create a geometry from dimensions and then serialise it.
  DLLExport std::string constructGeometryXML(DimensionVec dimensions,
      Dimension_sptr dimensionX,
      Dimension_sptr dimensionY,
      Dimension_sptr dimensionZ,
      Dimension_sptr dimensiont);

  /// Helper method to get dimensions from a geometry xml element.
  DLLExport std::vector<boost::shared_ptr<Mantid::Geometry::IMDDimension> > getDimensions(Poco::XML::Element* geometryElement, bool nonIntegratedOnly = false);

  /// Helper method to get dimensions from a geometry xml string.
  DLLExport std::vector<boost::shared_ptr<Mantid::Geometry::IMDDimension> > getDimensions(const std::string& geometryXMLString, bool nonIntegratedOnly = false);

  /// helper method to get a dimension from a dimension xml element.
  DLLExport Mantid::Geometry::MDDimension* createDimension(Poco::XML::Element* dimensionXML);

  /// helper method to get a dimension from a dimension xml element string.
  DLLExport Mantid::VATES::Dimension_sptr createDimension(const std::string& dimensionXMLString);

  /// helper method to get a dimension from a dimension xml element string and set the number of bins for that dimension to a specified value.
  DLLExport Mantid::VATES::Dimension_sptr createDimension(const std::string& dimensionXMLString, int nBins);

  /// helper method to extract the bounding box.
  DLLExport std::vector<double> getBoundingBox(const std::string& functionXMLString);

  /// Helper method. is used to determine wheter processing of an input data set is possible.
  DLLExport bool canProcessInput(vtkDataSet* inputDataSet);

}
}
#endif
