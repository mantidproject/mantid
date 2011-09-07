#ifndef MDEW_REBINNING_PRESENTER_H
#define MDEW_REBINNING_PRESENTER_H

#include "MantidGeometry/MDGeometry/MDGeometryXMLDefinitions.h"
#include "MantidGeometry/MDGeometry/MDGeometryXMLBuilder.h"
#include "MantidMDAlgorithms/CompositeImplicitFunction.h"
#include "MantidMDAlgorithms/PlaneImplicitFunction.h"
#include "MantidMDAlgorithms/NullImplicitFunction.h"
#include "MantidVatesAPI/vtkDataSetToGeometry.h"
#include "MantidVatesAPI/MDRebinningPresenter.h"
#include "MantidVatesAPI/RebinningActionManager.h"
#include "MantidVatesAPI/ProgressAction.h"
#include "MantidVatesAPI/vtkDataSetToGeometry.h"
#include "MantidVatesAPI/RebinningCutterXMLDefinitions.h"
#include "MantidVatesAPI/RebinningKnowledgeSerializer.h"
#include "MantidVatesAPI/FieldDataToMetadata.h"
#include "MantidVatesAPI/MetadataToFieldData.h"
#include "MantidVatesAPI/vtkDataSetFactory.h"
#include "MantidVatesAPI/WorkspaceProvider.h"
#include "MantidVatesAPI/vtkDataSetToImplicitFunction.h"
#include "MantidVatesAPI/vtkDataSetToWsLocation.h"
#include "MantidVatesAPI/vtkDataSetToWsName.h"
#include "MantidMDEvents/BinToMDHistoWorkspace.h"
#include "MantidAPI/ImplicitFunctionFactory.h"
#include <vtkDataSet.h>
#include <vtkFieldData.h>
#include <vtkPlane.h>

namespace Mantid
{
  namespace VATES
  {

    /** 
    @class MDEWRebinningPresenter, concrete MDRebinningPresenter using centre piece rebinning directly on MDEWs producing Histogrammed MDWs.
    @author Owen Arnold, Tessella plc
    @date 10/08/2011

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
    class DLLExport MDEWRebinningPresenter : public MDRebinningPresenter
    {
    public:

      /*----------------------------------- MDRebinningPresenter methods  -------------------------------------*/

      virtual void updateModel();

      virtual vtkDataSet* execute(vtkDataSetFactory* factory, ProgressAction& eventHandler);

      virtual const std::string& getAppliedGeometryXML() const;

      bool hasTDimensionAvailable() const;

      std::vector<double> getTimeStepValues() const;

      /*-----------------------------------End MDRebinningPresenter methods -------------------------------------*/

      MDEWRebinningPresenter(vtkDataSet* input, RebinningActionManager* request, MDRebinningView* view, const WorkspaceProvider& wsProvider);

      virtual ~MDEWRebinningPresenter();

    private:

      Mantid::Geometry::MDImplicitFunction_sptr constructPlaneFromVTKPlane(vtkPlane* plane, Mantid::MDAlgorithms::WidthParameter& width);
      void persistReductionKnowledge(vtkDataSet* out_ds, const RebinningKnowledgeSerializer& xmlGenerator, const char* id);
      std::string extractFormattedPropertyFromDimension(Mantid::Geometry::IMDDimension_sptr dimension) const;
      void addFunctionKnowledge();

      ///Parser used to process input vtk to extract metadata.
      vtkDataSetToGeometry m_inputParser;
      ///Input vtk dataset.
      vtkDataSet* m_input;
      ///Request, encapsulating priorisation of requests made for rebinning/redrawing.
      boost::scoped_ptr<RebinningActionManager> m_request;
      ///The view of this MVP pattern.
      MDRebinningView* m_view;
      ///Maximum threshold
      signal_t m_maxThreshold;
      ///Minimum threshold
      signal_t m_minThreshold;
      ///The current timestep.
      double m_timestep;
      ///The workspace geometry. Cached value.
      mutable std::string m_wsGeometry;
      ///Serializer of rebinning 
      RebinningKnowledgeSerializer m_serializer;
      /// Plane with width
      Mantid::Geometry::MDImplicitFunction_sptr m_plane;
      /// Flag indicating that clipping should be used.
      bool m_applyClipping;
    };
  }
}

#endif
