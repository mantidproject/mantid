#ifndef VATES_REBINNING_PRESENTER_H_
#define VATES_REBINNING_PRESENTER_H_

#include <vtkUnstructuredGrid.h>
#include <vtkBox.h>

#include <MantidAPI/ImplicitFunctionFactory.h>
#include <MantidAPI/ImplicitFunction.h>
#include <MantidMDAlgorithms/CompositeImplicitFunction.h>
#include <MantidGeometry/MDGeometry/IMDDimension.h>
#include <boost/shared_ptr.hpp>
#include <MantidVisitPresenters/RebinningXMLGenerator.h>

namespace Mantid
{
namespace VATES
{
/**

 Non-member helper methods used for mappings between vtk and mantid.

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

/// Clipper, forms base of Adapter pattern.
class Clipper
{
public:
  virtual void SetInput(vtkDataSet* in_ds) = 0;
  virtual void SetClipFunction(vtkImplicitFunction* func) = 0;
  virtual void SetInsideOut(bool insideout) = 0;
  virtual void SetRemoveWholeCells(bool removeWholeCells) = 0;
  virtual void SetOutput(vtkUnstructuredGrid* out_ds) = 0;
  virtual void Update() = 0;
  virtual ~Clipper()
  {
  }
  virtual void Delete() = 0;
};

typedef std::vector<boost::shared_ptr<Mantid::Geometry::IMDDimension> > DimensionVec;
typedef boost::shared_ptr<Mantid::Geometry::IMDDimension> Dimension_sptr;

/// RebinningCutterPresenter does the work of implemening requests/information provided by pipeline filters. Generates new datasets from
/// current and historical rebinning knowledge accumulated in the pipeline.
class RebinningCutterPresenter
{
private:

  /// Implicit function representing current and historical operations
  boost::shared_ptr<Mantid::API::ImplicitFunction> m_function;

  /// Rebining xml generator for serialising the current and historical operations.
  RebinningXMLGenerator m_serializing;

  vtkDataSet* m_inputDataSet;

  /// initalised flag
  bool m_initalized;

public:

	/// Constructor
	RebinningCutterPresenter(vtkDataSet* inputDataSet);

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
      double height,
      double width,
      double depth,
      std::vector<double>& origin);

  /// Apply reduction knowledge to create a vtk dataset.
  vtkUnstructuredGrid* applyReductionKnowledge(Clipper* clipper);

};

//Non-member helper functions.

  /// Save reduction knowledge object. Serialise to xml and pass to dependent filters.
  void persistReductionKnowledge(vtkUnstructuredGrid * out_ds,
      const RebinningXMLGenerator& xmlGenerator, const char* id);

  /// Walk composite functions and apply their operations to the visualisation dataset.
  void applyReductionKnowledgeToComposite(Clipper* clipper, vtkDataSet* in_ds,
      vtkUnstructuredGrid * out_ds, Mantid::API::ImplicitFunction* function);

  /// Convert field data to xml string meta data.
  std::string fieldDataToMetaData(vtkFieldData* fieldData, const char* id);

  /// Look for and extract exisiting reduction knowledge in input visualisation dataset.
  Mantid::API::ImplicitFunction* findExistingRebinningDefinitions(vtkDataSet *in_ds, const char* id);

  //Get the workspace location from the xmlstring.
  std::string findExistingWorkspaceNameFromXML(vtkDataSet *in_ds, const char* id);

  //Get the workspace location from the xmlstring.
  std::string findExistingWorkspaceLocationFromXML(vtkDataSet *in_ds, const char* id);

  /// Gets the effective constant meta data id for keying cutting information.
  const char*  getMetadataID();

  /// Converts field data into metadata xml/string.
  void metaDataToFieldData(vtkFieldData* fieldData, std::string metaData, const char* id);

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




}
}
#endif
