#ifndef MANTID_VATES_MD_REBINNING_HISTOGRAM_PRESENTER
#define MANTID_VATES_MD_REBINNING_HISTOGRAM_PRESENTER

#include <boost/scoped_ptr.hpp>
#include <boost/shared_ptr.hpp>

#include <vtkUnstructuredGrid.h>
#include <vtkDataSet.h>
#include <vtkBox.h> 
#include <vtkFieldData.h>

#include "MantidGeometry/MDGeometry/MDGeometryXMLDefinitions.h"
#include "MantidGeometry/MDGeometry/MDGeometryXMLBuilder.h"
#include "MantidGeometry/MDGeometry/MDGeometryXMLParser.h"

#include "MantidAPI/ImplicitFunctionFactory.h"
#include "MantidAPI/IMDWorkspace.h"

#include "MantidMDAlgorithms/NullImplicitFunction.h" 
#include "MantidMDAlgorithms/BoxImplicitFunction.h"
#include "MantidMDAlgorithms/CompositeImplicitFunction.h"
#include "MantidMDAlgorithms/DynamicRebinFromXML.h"
#include "MantidMDAlgorithms/Load_MDWorkspace.h"

#include "MantidVatesAPI/MDRebinningPresenter.h"
#include "MantidVatesAPI/MDHistogramRebinningPresenter.h"
#include "MantidVatesAPI/RebinningActionManager.h"
#include "MantidVatesAPI/vtkDataSetFactory.h"
#include "MantidVatesAPI/MDRebinningView.h"
#include "MantidVatesAPI/Clipper.h"
#include "MantidVatesAPI/vtkDataSetToGeometry.h"
#include "MantidVatesAPI/RebinningCutterXMLDefinitions.h"
#include "MantidVatesAPI/FieldDataToMetadata.h"
#include "MantidVatesAPI/MetadataToFieldData.h"
#include "MantidVatesAPI/ProgressAction.h"
#include "MantidVatesAPI/IMDWorkspaceProxy.h"
#include "MantidVatesAPI/RebinningKnowledgeSerializer.h"
#include "MantidVatesAPI/WorkspaceProvider.h"
#include "MantidVatesAPI/vtkDataSetToImplicitFunction.h"
#include "MantidVatesAPI/vtkDataSetToWsLocation.h"
#include "MantidVatesAPI/vtkDataSetToWsName.h"

//Forward declarations
class vtkDataSet;
class vtkBox;

namespace Mantid
{
  namespace VATES
  {

    /** 
    @class MDRebinningPresenter, concrete MDRebinningPresenter using centre piece rebinning on histogrammed IMDWorkspaces.
    @author Owen Arnold, Tessella plc
    @date 03/06/2011

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
    class MDRebinningView;
    class DLLExport MDHistogramRebinningPresenter : public MDRebinningPresenter
    {
    public:

      /*----------------------------------- MDRebinningPresenter methods  -------------------------------------*/

      virtual void updateModel();

      virtual vtkDataSet* execute(vtkDataSetFactory* factory, ProgressAction& eventHandler);

      virtual const std::string& getAppliedGeometryXML() const;

      bool hasTDimensionAvailable() const;

      std::vector<double> getTimeStepValues() const;

       /*-----------------------------------End MDRebinningPresenter methods -------------------------------------*/
      
      virtual ~MDHistogramRebinningPresenter();

      MDHistogramRebinningPresenter(vtkDataSet* input, RebinningActionManager* request, MDRebinningView* view, Clipper* clipper, const WorkspaceProvider& wsProvider);

    private:

      std::string constructGeometryXML(
        Mantid::Geometry::VecIMDDimension_sptr dimensions,
        Mantid::Geometry::IMDDimension_sptr ,
        Mantid::Geometry::IMDDimension_sptr ,
        Mantid::Geometry::IMDDimension_sptr ,
        Mantid::Geometry::IMDDimension_sptr );

      void forumulateBinChangeRequest(Mantid::Geometry::MDGeometryXMLParser& old_geometry, Mantid::Geometry::MDGeometryXMLParser& new_geometry);

      /// Construct a box from the interactor.
      Mantid::Geometry::MDImplicitFunction_sptr constructBoxFromVTKBox(vtkBox* box) const;

      /// Construct a box from the input dataset metadata.
      Mantid::Geometry::MDImplicitFunction_sptr constructBoxFromInput() const;

      /// Disabled copy constructor.
      MDHistogramRebinningPresenter(const MDHistogramRebinningPresenter& other);

      /// Disabled assignment.
      MDHistogramRebinningPresenter& operator=(const MDHistogramRebinningPresenter& other);

      /// Add existing function knowledge onto the serilizer.
      void addFunctionKnowledge();

      Mantid::API::IMDWorkspace_sptr constructMDWorkspace(const std::string& wsLocation);

      void persistReductionKnowledge(vtkDataSet* out_ds, const RebinningKnowledgeSerializer& xmlGenerator, const char* id);
      //TODO end fix --

      ///Parser used to process input vtk to extract metadata.
      vtkDataSetToGeometry m_inputParser;
      ///Input vtk dataset.
      vtkDataSet* m_input;
      ///Request, encapsulating priorisation of requests made for rebinning/redrawing.
      boost::scoped_ptr<RebinningActionManager> m_request;
      ///The view of this MVP pattern.
      MDRebinningView* m_view;
      ///Box implicit function used to determine boundaries via evaluation.
      Mantid::Geometry::MDImplicitFunction_sptr m_box;
      ///Clipper used to determine boundaries.
      boost::scoped_ptr<Clipper> m_clipper;
      ///Maximum threshold
      double m_maxThreshold;
      ///Minimum threshold
      double m_minThreshold;
      ///Flag indicating that clipping should be applied.
      bool m_applyClip;
      ///The current timestep.
      double m_timestep;
      ///The workspace geometry. Cached value.
      mutable std::string m_wsGeometry;
      ///Serializer, which may generate and store the rebinning knowledge.
      RebinningKnowledgeSerializer m_serializer;
    };
  }
}

#endif
