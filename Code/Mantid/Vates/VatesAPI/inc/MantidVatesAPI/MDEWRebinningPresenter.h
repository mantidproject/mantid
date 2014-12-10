#ifndef MDEW_REBINNING_PRESENTER_H
#define MDEW_REBINNING_PRESENTER_H

#include "MantidVatesAPI/MDRebinningPresenter.h"
#include "MantidGeometry/MDGeometry/MDImplicitFunction.h"
#include "MantidVatesAPI/MetadataJsonManager.h"
#include "MantidVatesAPI/RebinningKnowledgeSerializer.h"
#include "MantidVatesAPI/VatesConfigurations.h"
#include "MantidVatesAPI/vtkDataSetToGeometry.h"
#include <boost/scoped_ptr.hpp>
#include "MantidKernel/VMD.h"

class vtkPlane;
namespace Mantid
{

  namespace VATES
  {

    /** 
    @class MDEWRebinningPresenter
    Concrete MDRebinningPresenter using centre piece rebinning directly on MDEWs producing Histogrammed MDWs.
    @author Owen Arnold, Tessella plc
    @date 10/08/2011

    Copyright &copy; 2010 ISIS Rutherford Appleton Laboratory, NScD Oak Ridge National Laboratory & European Spallation Source

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

    class MDRebinningView;
    class RebinningActionManager;
    class WorkspaceProvider;

    class DLLExport MDEWRebinningPresenter : public MDRebinningPresenter
    {
    public:

      /*----------------------------------- MDRebinningPresenter methods  -------------------------------------*/

      virtual void updateModel();

      virtual vtkDataSet* execute(vtkDataSetFactory* factory, ProgressAction& rebinningProgressUpdate, ProgressAction& drawingProgressUpdate);

      virtual const std::string& getAppliedGeometryXML() const;

      bool hasTDimensionAvailable() const;

      std::vector<double> getTimeStepValues() const;

      std::string getTimeStepLabel() const;

      virtual void makeNonOrthogonal(vtkDataSet* visualDataSet);

      virtual void setAxisLabels(vtkDataSet* visualDataSet);

      virtual const std::string& getInstrument() const;

      virtual double getMaxValue() const;

      virtual double getMinValue() const;

      /*-----------------------------------End MDRebinningPresenter methods -------------------------------------*/

      MDEWRebinningPresenter(vtkDataSet* input, RebinningActionManager* request, MDRebinningView* view, const WorkspaceProvider& wsProvider);

      virtual ~MDEWRebinningPresenter();

    private:

      void persistReductionKnowledge(vtkDataSet* out_ds, const RebinningKnowledgeSerializer& xmlGenerator, const char* id);
      std::string extractFormattedPropertyFromDimension(Mantid::Geometry::IMDDimension_sptr dimension) const;
      std::string extractFormattedPropertyFromDimension(const Mantid::Kernel::V3D& basis, const size_t totalNDims, double length, Mantid::Geometry::IMDDimension_sptr dimension) const;
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
      /// Function
      Mantid::Geometry::MDImplicitFunction_sptr m_function;
      /// Flag indicating that clipping should be used.
      bool m_applyClipping;
      /// Origin
      Mantid::Kernel::V3D m_origin;
      /// b1 direction vector
      Mantid::Kernel::V3D m_b1;
      /// b2 direction vector
      Mantid::Kernel::V3D m_b2;
      /// length b1
      double m_lengthB1;
      /// length b2
      double m_lengthB2;
      /// length b3
      double m_lengthB3;
      /// ForceOrthogonal coords
      bool m_ForceOrthogonal;
      /// Force output in terms of a histogram workspace. Decides which rebinning algorithm to use.
      bool m_bOutputHistogramWS;
      /// Tag for the rebinned workspace
      static const std::string rb_tag;
      /// Pointer to the manager for json metadata
      boost::scoped_ptr<MetadataJsonManager> m_metadataJsonManager;
      /// Pointer to the vates configuration object
      boost::scoped_ptr<VatesConfigurations> m_vatesConfigurations;
      /// Store for the instrument
      mutable std::string m_instrument;
    };
  }
}

#endif
