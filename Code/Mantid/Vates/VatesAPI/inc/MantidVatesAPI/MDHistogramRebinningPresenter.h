#ifndef MANTID_VATES_MD_REBINNING_HISTOGRAM_PRESENTER
#define MANTID_VATES_MD_REBINNING_HISTOGRAM_PRESENTER

#include "MantidGeometry/MDGeometry/MDGeometryXMLParser.h"
#include "MantidAPI/ImplicitFunction.h"
#include "MantidAPI/IMDWorkspace.h"
#include "MantidVatesAPI/MDRebinningPresenter.h"
#include "MantidVatesAPI/vtkDataSetToGeometry.h"
#include "MantidVatesAPI/RebinningKnowledgeSerializer.h"
#include <boost/scoped_ptr.hpp>
#include <boost/shared_ptr.hpp>

//Forward declarations
class vtkDataSet;
class vtkBox;

namespace Mantid
{
  namespace VATES
  {
    ///Share pointer over implicit functions.
    typedef boost::shared_ptr<Mantid::API::ImplicitFunction> ImplicitFunction_sptr;

    //Forward declarations.
    class vtkDataSetFactory;
    class RebinningActionManager;
    class MDRebinningView;
    class Clipper;

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
    class DLLExport MDHistogramRebinningPresenter : public MDRebinningPresenter
    {
    public:
      virtual void updateModel();

      virtual vtkUnstructuredGrid* execute(ProgressAction& eventHandler);

      virtual std::string getAppliedGeometryXML() const;

      virtual ~MDHistogramRebinningPresenter();

      MDHistogramRebinningPresenter(vtkDataSetFactory* factoryChain, vtkDataSet* input, RebinningActionManager* request, MDRebinningView* view, Clipper* clipper);

    private:

      std::string constructGeometryXML(
        Mantid::Geometry::VecIMDDimension_sptr dimensions,
        Mantid::Geometry::IMDDimension_sptr ,
        Mantid::Geometry::IMDDimension_sptr ,
        Mantid::Geometry::IMDDimension_sptr ,
        Mantid::Geometry::IMDDimension_sptr );

      void forumulateBinChangeRequest(Mantid::Geometry::MDGeometryXMLParser& old_geometry, Mantid::Geometry::MDGeometryXMLParser& new_geometry);
      ImplicitFunction_sptr constructBoxFromVTKBox(vtkBox* box) const;
      ImplicitFunction_sptr constructBoxFromInput() const;

      MDHistogramRebinningPresenter(const MDHistogramRebinningPresenter& other);

      MDHistogramRebinningPresenter& operator=(const MDHistogramRebinningPresenter& other);

      boost::scoped_ptr<vtkDataSetFactory> m_factory;
      boost::scoped_ptr<RebinningActionManager> m_request;
      MDRebinningView* m_view;
      vtkDataSetToGeometry m_inputParser;

      vtkDataSet* m_input;
      double m_timestep;
      double m_maxThreshold;
      double m_minThreshold;
      bool m_applyClip;
      ImplicitFunction_sptr m_box;
      RebinningKnowledgeSerializer m_serializer;
      boost::scoped_ptr<Clipper> m_clipper;

      //TODO fix ---
      Mantid::API::ImplicitFunction* findExistingRebinningDefinitions(vtkDataSet* inputDataSet, const char* id);

      std::string MDHistogramRebinningPresenter::findExistingWorkspaceName(vtkDataSet *inputDataSet, const char* id);

      std::string MDHistogramRebinningPresenter::findExistingWorkspaceLocation(vtkDataSet *inputDataSet, const char* id);

      void addFunctionKnowledge();

      Mantid::API::IMDWorkspace_sptr constructMDWorkspace(const std::string& wsLocation);

      void persistReductionKnowledge(vtkDataSet* out_ds, const RebinningKnowledgeSerializer& xmlGenerator, const char* id);
      //TODO end fix --
    };
  }
}

#endif