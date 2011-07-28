#ifndef MANTID_VATES_MDEW_LOADING_REBINNING_PRESENTER 
#define MANTID_VATES_MDEW_LOADING_REBINNING_PRESENTER

#include "MantidVatesAPI/MDLoadingRebinningPresenter.h"
#include "MantidVatesAPI/MDLoadingRebinningView.h"
#include "MantidVatesAPI/vtkDataSetFactory.h"
#include "MantidVatesAPI/RebinningKnowledgeSerializer.h"
#include "MantidVatesAPI/RebinningActionManager.h"
#include "MantidVatesAPI/ProgressAction.h"
#include "MantidVatesAPI/RebinningCutterXMLDefinitions.h"
#include "MantidVatesAPI/FilteringUpdateProgressAction.h"
#include "MantidVatesAPI/MetadataToFieldData.h"

#include "MantidGeometry/MDGeometry/IMDDimension.h"
#include "MantidGeometry/MDGeometry/MDGeometryXMLParser.h"
#include "MantidGeometry/MDGeometry/MDGeometryXMLBuilder.h"
#include "MantidAPI/ImplicitFunction.h"

#include "MantidMDAlgorithms/NullImplicitFunction.h"
#include "MantidMDAlgorithms/PlaneImplicitFunction.h"
#include "MantidMDAlgorithms/CompositeImplicitFunction.h"

#include "MantidMDEvents/BinToMDHistoWorkspace.h"

#include <vtkPlane.h>
#include <vtkFieldData.h>
#include <boost/scoped_ptr.hpp>

namespace Mantid
{
  namespace VATES
  {
    ///Share pointer over implicit functions.
    typedef boost::shared_ptr<Mantid::API::ImplicitFunction> ImplicitFunction_sptr;

    /** 
    @class MDEWLoadingRebinningPresenter containing common code for MDEW based loading and rebinning.
    @author Owen Arnold, Tessella plc
    @date 25/07/2011

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
    template<typename ViewType>
    class DLLExport MDEWLoadingRebinningPresenter : public MDLoadingRebinningPresenter
    {
    public:

      MDEWLoadingRebinningPresenter(std::string fileName, RebinningActionManager* request, ViewType* view);
      virtual void updateModel();
      virtual vtkDataSet* execute(vtkDataSetFactory* factory, ProgressAction& eventHandler);
      virtual const std::string& getAppliedGeometryXML() const;
      virtual bool hasTDimensionAvailable() const;
      virtual std::vector<double> getTimeStepValues() const;
      virtual ~MDEWLoadingRebinningPresenter();

    protected:
      ///Name + path to file to load.
      std::string m_filename;
      ///Request, encapsulating priorisation of requests made for rebinning/redrawing.
      boost::scoped_ptr<RebinningActionManager> m_request;
      ///The view of this MVP pattern.
      ViewType* m_view;
      ///Box implicit function used to determine boundaries via evaluation.
      ImplicitFunction_sptr m_ifunction;
      ///Maximum threshold
      signal_t m_maxThreshold;
      ///Minimum threshold
      signal_t m_minThreshold;
      ///Flag indicating that clipping should be applied.
      bool m_applyClip;
      ///The current timestep.
      double m_timestep;
      ///The workspace geometry. Cached value.
      mutable std::string m_wsGeometry;
      ///Serializer, which may generate and store the rebinning knowledge.
      RebinningKnowledgeSerializer m_serializer;
      //Identifier for generated histogram workspaces
      const std::string m_histogrammedWsId;
      //Identifier for generated event workspaces.
      const std::string m_mdEventWsId;
      //Workspace geometry object.
      Mantid::Geometry::MDGeometryBuilderXML<Mantid::Geometry::StrictDimensionPolicy> m_geometryXmlBuilder;
      //Flag indicating that file loading has occured completely in memory
      bool m_loadInMemory;
      //Flag indicating that executLoad has been called.
      bool m_hasLoaded;

    private:

      ImplicitFunction_sptr constructPlaneFromVTKPlane(vtkPlane* vPlane) const;

      std::string extractFormattedPropertyFromDimension(Mantid::Geometry::IMDDimension_sptr dimension) const;

      void persistReductionKnowledge(vtkDataSet* out_ds, const
        RebinningKnowledgeSerializer& xmlGenerator, const char* id);

      void addFunctionKnowledge();

      void forumulateBinChangeRequest(Mantid::Geometry::MDGeometryXMLParser& old_geometry, Mantid::Geometry::MDGeometryXMLParser& new_geometry);

    };

    /** Constructor
    * @param filename : File to load.
    * @param request : Request managing object
    * @param view : MVP view
    */
    template<typename ViewType>
    MDEWLoadingRebinningPresenter<ViewType>::MDEWLoadingRebinningPresenter(std::string fileName, RebinningActionManager* request, ViewType* view) :
    m_filename(fileName),
      m_request(request), 
      m_view(view), 
      m_ifunction(new Mantid::MDAlgorithms::NullImplicitFunction()), 
      m_maxThreshold(0),
      m_minThreshold(0),
      m_applyClip(false),
      m_timestep(0),
      m_wsGeometry(""),
      m_histogrammedWsId("histo_event_ws_id"),
      m_mdEventWsId("event_ws_id"),
      m_loadInMemory(false),
      m_hasLoaded(false)
    {
    }

    ///** constructs geometry xml string from dimensions.
    //* @param dimensions : all dimension
    //* @param dimensionX : x mapping
    //* @param dimensionY : y mapping
    //* @param dimensionZ : z mapping
    //* @param dimensionT : t mapping
    //*/
    //template<typename ViewType>
    //std::string MDEWLoadingRebinningPresenter<ViewType>::constructGeometryXML(
    //  DimensionVec dimensions,
    //  Dimension_sptr dimensionX,
    //  Dimension_sptr dimensionY,
    //  Dimension_sptr dimensionZ,
    //  Dimension_sptr dimensiont)
    //{
    //  using Mantid::Geometry::MDGeometryBuilderXML;
    //  using Mantid::Geometry::StrictDimensionPolicy;
    //  MDGeometryBuilderXML<StrictDimensionPolicy> xmlBuilder;
    //  DimensionVec::iterator it = dimensions.begin();
    //  for(;it != dimensions.end(); ++it)
    //  {
    //    xmlBuilder.addOrdinaryDimension(*it);
    //  }
    //  xmlBuilder.addXDimension(dimensionX);
    //  xmlBuilder.addYDimension(dimensionY);
    //  xmlBuilder.addZDimension(dimensionZ);
    //  xmlBuilder.addTDimension(dimensiont);
    //  return xmlBuilder.create();
    //}

    /** Uses changes in the number of bins for each mapped dimension to determine when to perform rebinning.
    * @param old_geometry : previous instance geometry
    * @param new_geometry : view instance geometry
    */
    template<typename ViewType>
    void MDEWLoadingRebinningPresenter<ViewType>::forumulateBinChangeRequest(Mantid::Geometry::MDGeometryXMLParser& old_geometry, Mantid::Geometry::MDGeometryXMLParser& new_geometry)
    {
      if(old_geometry.hasXDimension() && new_geometry.hasXDimension())
      {
        if(old_geometry.getXDimension()->getNBins() != new_geometry.getXDimension()->getNBins())
        {
          m_request->ask(RecalculateAll);
        }
      }
      if(old_geometry.hasYDimension() && new_geometry.hasYDimension())
      {
        if(old_geometry.getYDimension()->getNBins() != new_geometry.getYDimension()->getNBins())
        {
          m_request->ask(RecalculateAll);
        }
      }
      if(old_geometry.hasZDimension() && new_geometry.hasZDimension())
      {
        if(old_geometry.getZDimension()->getNBins() != new_geometry.getZDimension()->getNBins())
        {
          m_request->ask(RecalculateAll);
        }
      }
      if(old_geometry.hasTDimension() && new_geometry.hasTDimension())
      {
        if(old_geometry.getTDimension()->getNBins() != new_geometry.getTDimension()->getNBins())
        {
          m_request->ask(RecalculateAll);
        }
      }
    }


    /** Converts a vtkplane into an implicitfunction plane.
    * @param plane : vtkImplicitFunction.
    * @return ImplicitFunction_sptr containing ImplicitFunction plane.
    */
    template<typename ViewType>
    ImplicitFunction_sptr MDEWLoadingRebinningPresenter<ViewType>::constructPlaneFromVTKPlane(vtkPlane* plane) const
    {
      using namespace Mantid::MDAlgorithms;

      double* pNormal = plane->GetNormal();
      double* pOrigin = plane->GetOrigin();

      //Create domain parameters.
      OriginParameter originParam = OriginParameter(pOrigin[0], pOrigin[1], pOrigin[2]);
      NormalParameter normalParam = NormalParameter(pNormal[0], pNormal[1], pNormal[3]);
      WidthParameter widthParam = WidthParameter(1); //TODO make user configurable.

      //Create the plane,.
      PlaneImplicitFunction* planeFunction = new PlaneImplicitFunction(normalParam, originParam, widthParam);

      return ImplicitFunction_sptr(planeFunction);
    }


    /** Update the MVP model, forumulates and hive-off a request for rebinning
    */
    template<typename ViewType>
    void MDEWLoadingRebinningPresenter<ViewType>::updateModel()
    {
      if(!m_hasLoaded)
      {
        throw std::runtime_error("There is no model to update. Call ::executeLoad on MDEWLoadingRebinningPresenter first!");
      }
      if(m_view->getLoadInMemory() != m_loadInMemory)
      {
        m_request->ask(ReloadAndRecalculateAll);
        m_loadInMemory = m_view->getLoadInMemory();
      }
      if(m_view->getTimeStep() != m_timestep)
      {
        m_request->ask(RecalculateVisualDataSetOnly);
        m_timestep = m_view->getTimeStep();
      }
      if(m_view->getMaxThreshold() != m_maxThreshold)
      {
        m_request->ask(RecalculateVisualDataSetOnly);
        m_maxThreshold = m_view->getMaxThreshold();
      }
      if(m_view->getMinThreshold() != m_minThreshold)
      {
        m_request->ask(RecalculateVisualDataSetOnly);
        m_minThreshold = m_view->getMinThreshold();
      }
      bool hasAppliedClipping = m_view->getApplyClip();
      if(hasAppliedClipping != m_applyClip)
      {
        //check it's a plane.
        //extract a plane.
        //compare planes.
        vtkPlane* plane = dynamic_cast<vtkPlane*>(m_view->getImplicitFunction());
        if(NULL != plane && hasAppliedClipping)
        {
          m_ifunction = constructPlaneFromVTKPlane(plane); 
        }
        m_applyClip = m_view->getApplyClip();
      }

      //Should always do clipping comparison
      if(hasAppliedClipping == true)
      {
        using Mantid::MDAlgorithms::PlaneImplicitFunction;
        vtkPlane* plane = dynamic_cast<vtkPlane*>(m_view->getImplicitFunction());
        if(NULL != plane)
        {
          boost::shared_ptr<PlaneImplicitFunction> planeA = boost::dynamic_pointer_cast<PlaneImplicitFunction>(constructPlaneFromVTKPlane(plane)); 
          boost::shared_ptr<PlaneImplicitFunction> planeB = boost::dynamic_pointer_cast<PlaneImplicitFunction>(m_ifunction); 
          if(planeA->operator!=(*planeB.get()))
          {
            m_ifunction = planeA;
            m_request->ask(RecalculateAll);
          }
        }
      }

      addFunctionKnowledge();

      if(m_view->getAppliedGeometryXML() != m_serializer.getWorkspaceGeometry())
      {
        Mantid::Geometry::MDGeometryXMLParser old_geometry(m_serializer.getWorkspaceGeometry());
        old_geometry.execute();
        Mantid::Geometry::MDGeometryXMLParser new_geometry(m_view->getAppliedGeometryXML());
        new_geometry.execute();
        
        m_request->ask(RecalculateAll);
       
        forumulateBinChangeRequest(old_geometry, new_geometry);
        m_serializer.setGeometryXML( m_view->getAppliedGeometryXML() );
      }
    }


    /// Collect implicit function knowledge together.
    template<typename ViewType>
    void MDEWLoadingRebinningPresenter<ViewType>::addFunctionKnowledge()
    {
      //Add existing functions.
      using Mantid::MDAlgorithms::CompositeImplicitFunction;
      CompositeImplicitFunction* compFunction = new CompositeImplicitFunction;

      compFunction->addFunction(m_ifunction);
      //Apply the implicit function.
      m_serializer.setImplicitFunction(ImplicitFunction_sptr(compFunction));
    }

    /**
    Mantid properties for rebinning algorithm require formatted information.
    @param dimension : dimension to extract property value for.
    @return true available, false otherwise.
    */
    template<typename ViewType>
    std::string MDEWLoadingRebinningPresenter<ViewType>::extractFormattedPropertyFromDimension(Mantid::Geometry::IMDDimension_sptr dimension) const
    {
      double min = dimension->getMinimum();
      double max = dimension->getMaximum();
      size_t nbins = dimension->getNBins();
      std::string id = dimension->getDimensionId();
      return boost::str(boost::format("%s, %f, %f, %d") %id %min %max % nbins);
    }


    /** Coordinate the production of a vtkDataSet matching the request
    @parameter factory : vtkDataSet producing factory chain.
    @parameter eventHandler : observer/listenter 
    */
    template<typename ViewType>
    vtkDataSet* MDEWLoadingRebinningPresenter<ViewType>::execute(vtkDataSetFactory* factory, ProgressAction&)
    {
      using namespace Mantid::API;
      using namespace Mantid::MDEvents;

      const std::string outputWorkspace =  XMLDefinitions::RebinnedWSName();
      //Rebin using member variables.
      if(RecalculateAll == m_request->action())
      {
        Mantid::Geometry::MDGeometryXMLParser parser(this->m_wsGeometry);
        parser.execute();

        //Get the input workspace location and name.
        std::string wsLocation = m_serializer.getWorkspaceLocation();
        std::string wsName = m_serializer.getWorkspaceName();

        Mantid::API::AnalysisDataService::Instance().remove(m_histogrammedWsId);

        Mantid::MDEvents::BinToMDHistoWorkspace hist_alg;
        hist_alg.initialize();
        hist_alg.setPropertyValue("InputWorkspace", m_mdEventWsId);
        std::string id; 
        if(parser.hasXDimension())
        {

          hist_alg.setPropertyValue("DimX",  extractFormattedPropertyFromDimension(parser.getXDimension())); 
        }
        if(parser.hasYDimension())
        {
          hist_alg.setPropertyValue("DimY",  extractFormattedPropertyFromDimension(parser.getYDimension())); 
        }
        if(parser.hasZDimension())
        {
          hist_alg.setPropertyValue("DimZ",  extractFormattedPropertyFromDimension(parser.getZDimension())); 
        }
        if(parser.hasTDimension())
        {
          hist_alg.setPropertyValue("DimT",  extractFormattedPropertyFromDimension(parser.getTDimension())); 
        }
        hist_alg.setPropertyValue("OutputWorkspace", m_histogrammedWsId);

        //TODO: Add implicit function

        FilterUpdateProgressAction<ViewType> updatehandler(m_view);

        Poco::NObserver<ProgressAction, Mantid::API::Algorithm::ProgressNotification> observer(updatehandler, &ProgressAction::handler);
        //Add observer.
        hist_alg.addObserver(observer);
        //Run the rebinning algorithm.
        hist_alg.execute();
        //Remove observer
        hist_alg.removeObserver(observer);
      }

      //Use the generated workspace to access the underlying image, which may be rendered.
      IMDWorkspace_sptr outputWs = boost::dynamic_pointer_cast<IMDWorkspace>(
        AnalysisDataService::Instance().retrieve(outputWorkspace));

      factory->initialize(outputWs);

      //vtkUnstructuredGrid* temp = static_cast<vtkUnstructuredGrid*>(factory->create());
      vtkDataSet* temp = factory->create();

      //persistReductionKnowledge(temp, this->m_serializer, XMLDefinitions::metaDataId().c_str()); //TODO
      m_request->reset();

      return temp;
    }

    template<typename ViewType>
    const std::string& MDEWLoadingRebinningPresenter<ViewType>::getAppliedGeometryXML() const
    {
      return m_serializer.getWorkspaceGeometry();
    }

    template<typename ViewType>
    MDEWLoadingRebinningPresenter<ViewType>::~MDEWLoadingRebinningPresenter()
    {
    }

    template<typename ViewType>
    bool
      MDEWLoadingRebinningPresenter<ViewType>::hasTDimensionAvailable() const
    {
      Mantid::Geometry::MDGeometryXMLParser sourceGeometry(m_serializer.getWorkspaceGeometry());
      sourceGeometry.execute();
      return sourceGeometry.getTDimension();
    }

    template<typename ViewType>
    std::vector<double> MDEWLoadingRebinningPresenter<ViewType>::getTimeStepValues() const
    {
      Mantid::Geometry::MDGeometryXMLParser sourceGeometry(m_view->getAppliedGeometryXML());
      sourceGeometry.execute();

      double min = sourceGeometry.getTDimension()->getMinimum();
      double max = sourceGeometry.getTDimension()->getMaximum();
      unsigned int nBins = static_cast<int>(sourceGeometry.getTDimension()->getNBins() );
      double increment = (max - min) / nBins;
      std::vector<double> timeStepValues(nBins);
      for (unsigned int i = 0; i < nBins; i++)
      {
        timeStepValues[i] = min + (i * increment);
      }
      return timeStepValues;
    }


    template<typename ViewType>
    void MDEWLoadingRebinningPresenter<ViewType>::persistReductionKnowledge(vtkDataSet* out_ds, const
      RebinningKnowledgeSerializer& xmlGenerator, const char* id)
    {
      vtkFieldData* fd = vtkFieldData::New();

      MetadataToFieldData convert;
      convert(fd, xmlGenerator.createXMLString().c_str(), id);

      out_ds->SetFieldData(fd);
      fd->Delete();
    }



  }
}

#endif