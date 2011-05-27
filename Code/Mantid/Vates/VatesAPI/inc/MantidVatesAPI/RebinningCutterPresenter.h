#ifndef VATES_REBINNING_PRESENTER_H_
#define VATES_REBINNING_PRESENTER_H_

#include <vtkUnstructuredGrid.h>
#include <vtkBox.h>

#include "MantidAPI/IMDWorkspace.h"
#include <MantidVatesAPI/Common.h>
#include <MantidAPI/ImplicitFunctionFactory.h>
#include <MantidAPI/ImplicitFunction.h>
#include <MantidMDAlgorithms/CompositeImplicitFunction.h>
#include <MantidGeometry/MDGeometry/IMDDimension.h>
#include <boost/shared_ptr.hpp>
#include <MantidVatesAPI/RebinningKnowledgeSerializer.h>
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
  RebinningKnowledgeSerializer m_serializer;

  /// Create a geometry from dimensions and then serialise it.
  std::string constructGeometryXML(DimensionVec dimensions,
      Dimension_sptr dimensionX,
      Dimension_sptr dimensionY,
      Dimension_sptr dimensionZ,
      Dimension_sptr dimensiont);

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

	Mantid::API::IMDWorkspace_sptr applyRebinningAction(
	    Mantid::VATES::RebinningIterationAction action
	    ,Mantid::VATES::ProgressAction& eventHandler) const;

  /// Apply reduction knowledge to create a vtk dataset.
  vtkDataSet* createVisualDataSet(boost::shared_ptr<vtkDataSetFactory> spvtkDataSetFactory);

  /// Get the dimension from the image with the id.
  Dimension_const_sptr getDimensionFromWorkspace(const std::string& id);

  /// Get the workspace geometry as an xml string.
  const std::string& getWorkspaceGeometry() const;

  /// Verify that construct method has been called before anything else.
  void VerifyInitalization() const;
};

//Non-member helper functions.

  /// Save reduction knowledge object. Serialise to xml and pass to dependent filters.
  void persistReductionKnowledge(vtkDataSet * out_ds,
      const RebinningKnowledgeSerializer& xmlGenerator, const char* id);

  /// Look for and extract exisiting reduction knowledge in input visualisation dataset.
  DLLExport Mantid::API::ImplicitFunction* findExistingRebinningDefinitions(vtkDataSet *in_ds, const char* id);

  //Get the workspace location from the xmlstring. xmlstring is present of vtkFieldData on vtkDataSet.
  DLLExport std::string findExistingWorkspaceName(vtkDataSet *in_ds, const char* id);

  //Get the workspace location from the xmlstring. xmlstring is present of vtkFieldData on vtkDataSet.
  DLLExport std::string findExistingWorkspaceLocation(vtkDataSet *in_ds, const char* id);

  //Get the workspace geometry from the xmlstring. xmlstring is present of vtkFieldData on vtkDataSet.
  DLLExport Poco::XML::Element* findExistingGeometryInformation(vtkDataSet* inputDataSet, const char* id);

  /// Construct an input MDWorkspace by loading from a file. This should be achieved via a seperate loading algorithm.
  Mantid::API::IMDWorkspace_sptr constructMDWorkspace(const std::string& wsLocation);

  /// Helper method. is used to determine whether processing of an input data set is possible.
  DLLExport bool canProcessInput(vtkDataSet* inputDataSet);

}
}
#endif
