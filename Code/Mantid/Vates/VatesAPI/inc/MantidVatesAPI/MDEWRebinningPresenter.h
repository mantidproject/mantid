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
    template<typename ViewType>
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

      MDEWRebinningPresenter(vtkDataSet* input, RebinningActionManager* request, ViewType* view, const WorkspaceProvider& wsProvider);

      virtual ~MDEWRebinningPresenter();

    private:

      Mantid::API::ImplicitFunction_sptr constructPlaneFromVTKPlane(vtkPlane* plane, Mantid::MDAlgorithms::WidthParameter& width);
      std::string findExistingWorkspaceName(vtkDataSet *inputDataSet, const char* id);
      std::string findExistingWorkspaceLocation(vtkDataSet *inputDataSet, const char* id);
      Mantid::API::ImplicitFunction* findExistingRebinningDefinitions(vtkDataSet* inputDataSet, const char* id);
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
      ViewType* m_view;
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
      Mantid::API::ImplicitFunction_sptr m_plane;
      /// Flag indicating that clipping should be used.
      bool m_applyClipping;
    };

    /**
    Constructor.
    @param input : input vtk dataset containing existing metadata.
    @param request : object performing decision making on what rebinning action to take.
    @param view : mvp view handle to use.
    @param wsProvider : ref to object used to determine the availability of the correct ws for this prsenter to work on.
    */
    template<typename ViewType>
    MDEWRebinningPresenter<ViewType>::MDEWRebinningPresenter(vtkDataSet* input, RebinningActionManager* request, ViewType* view, const WorkspaceProvider& wsProvider) :
    m_inputParser(input),
      m_input(input), 
      m_request(request), 
      m_view(view), 
      m_maxThreshold(0),
      m_minThreshold(0),
      m_timestep(0),
      m_wsGeometry(""),
      m_serializer(LocationNotRequired),
      m_plane(Mantid::API::ImplicitFunction_sptr(new Mantid::MDAlgorithms::NullImplicitFunction())),
      m_applyClipping(false)
    {
      using namespace Mantid::API;
      vtkFieldData* fd = input->GetFieldData();
      if(NULL == fd || NULL == fd->GetArray(XMLDefinitions::metaDataId().c_str()))
      {
        throw std::logic_error("Rebinning operations require Rebinning Metadata");
      }
      std::string wsName = findExistingWorkspaceName(m_input, XMLDefinitions::metaDataId().c_str());
      if(!wsProvider.canProvideWorkspace(wsName))
      {
        throw std::invalid_argument("Wrong type of Workspace stored. Cannot handle with this presenter");
      }

      vtkDataSetToGeometry parser(input);
      parser.execute();

      using Mantid::Geometry::MDGeometryBuilderXML;
      using Mantid::Geometry::StrictDimensionPolicy;
      MDGeometryBuilderXML<StrictDimensionPolicy> xmlBuilder;

      Mantid::Geometry::VecIMDDimension_sptr dimensions =parser.getAllDimensions();
      DimensionVec::iterator it = dimensions.begin();
      for(;it != dimensions.end(); ++it)
      {
        xmlBuilder.addOrdinaryDimension(*it);
      }
      if(parser.hasXDimension())
      {
        xmlBuilder.addXDimension(parser.getXDimension());
      }
      if(parser.hasYDimension())
      {
        xmlBuilder.addYDimension(parser.getYDimension());
      }
      if(parser.hasZDimension())
      {
        xmlBuilder.addZDimension(parser.getZDimension());
      }
      if(parser.hasTDimension())
      {
        xmlBuilder.addTDimension(parser.getTDimension());
      }

      //Apply the geometry.
      m_serializer.setGeometryXML(xmlBuilder.create());
      //Apply the workspace name after extraction from the input xml.
      m_serializer.setWorkspaceName( wsName);
      //Apply the workspace location after extraction from the input xml.
      m_serializer.setWorkspaceLocation( findExistingWorkspaceLocation(m_input, XMLDefinitions::metaDataId().c_str()));

    }

    /// Destructor
    template<typename ViewType>
    MDEWRebinningPresenter<ViewType>::~MDEWRebinningPresenter()
    {
    }

    /**
    Constructs a PlaneImplicitFunction from a vtkPlane
    @param plane: vtkPlane
    @param width: plane width
    */
    template<typename ViewType>
    Mantid::API::ImplicitFunction_sptr MDEWRebinningPresenter<ViewType>::constructPlaneFromVTKPlane(vtkPlane* plane, Mantid::MDAlgorithms::WidthParameter& width)
    {
      double* pOrigin = plane->GetOrigin();
      double* pNormal = plane->GetNormal();
      Mantid::MDAlgorithms::OriginParameter origin(pOrigin[0], pOrigin[1], pOrigin[2]);
      Mantid::MDAlgorithms::NormalParameter normal(pNormal[0], pNormal[1], pNormal[2]);
      return Mantid::API::ImplicitFunction_sptr(new Mantid::MDAlgorithms::PlaneImplicitFunction(normal, origin, width));
    }

    /*
    Records and accumulates function knowledge so that it can be seralized to xml later.
    */
    template<typename ViewType>
    void MDEWRebinningPresenter<ViewType>::addFunctionKnowledge()
    {
      //Add existing functions.
      Mantid::MDAlgorithms::CompositeImplicitFunction* compFunction = new Mantid::MDAlgorithms::CompositeImplicitFunction;
      compFunction->addFunction(m_plane);
      Mantid::API::ImplicitFunction* existingFunctions = findExistingRebinningDefinitions(m_input, XMLDefinitions::metaDataId().c_str());
      if (existingFunctions != NULL)
      {
        compFunction->addFunction(Mantid::API::ImplicitFunction_sptr(existingFunctions));
      }
      //Apply the implicit function.
      m_serializer.setImplicitFunction(Mantid::API::ImplicitFunction_sptr(compFunction));
    }

    /**
    Uses the state of the MVP view to determine what rebinning action to take next. Also updates the internal
    members according to the state of the view so that the 'Delta' between the view and this presenter can 
    be compared and determined again at a later point.
    */
    template<typename ViewType>
    void MDEWRebinningPresenter<ViewType>::updateModel()
    {
      using namespace Mantid::MDAlgorithms;


      if(m_view->getTimeStep() != m_timestep)
      {
        m_request->ask(RecalculateVisualDataSetOnly);
      }
      if(m_view->getMaxThreshold() != m_maxThreshold)
      {
        m_request->ask(RecalculateVisualDataSetOnly); 
      }
      if(m_view->getMinThreshold() != m_minThreshold)
      {
        m_request->ask(RecalculateVisualDataSetOnly);
      }

      bool hasAppliedClipping = m_view->getApplyClip();

      //Recalculation is always required if this property is toggled.
      if(m_applyClipping != hasAppliedClipping)
      {
        m_request->ask(RecalculateAll);
      }

      //Should always do clipping comparison if clipping has been set to on.
      if(m_applyClipping == true)
      {
        vtkPlane* plane = dynamic_cast<vtkPlane*>(m_view->getImplicitFunction());
        WidthParameter width(m_view->getWidth());
        if(NULL != plane)
        {
          boost::shared_ptr<Mantid::MDAlgorithms::PlaneImplicitFunction> planeA = boost::dynamic_pointer_cast<Mantid::MDAlgorithms::PlaneImplicitFunction>(constructPlaneFromVTKPlane(plane, width));
          boost::shared_ptr<Mantid::MDAlgorithms::PlaneImplicitFunction> planeB = boost::dynamic_pointer_cast<Mantid::MDAlgorithms::PlaneImplicitFunction>(m_plane);
          if(!planeB || planeA->operator!=(*planeB.get()))
          {
            m_plane = planeA;
            m_request->ask(RecalculateAll);
          }
        }
      }

      if(m_view->getAppliedGeometryXML() != m_serializer.getWorkspaceGeometry())
      {
        m_request->ask(RecalculateAll);
      }

      //Update the presenter fields.
      m_timestep = m_view->getTimeStep();
      m_maxThreshold = m_view->getMaxThreshold(); 
      m_minThreshold = m_view->getMinThreshold();
      m_applyClipping = hasAppliedClipping;
      addFunctionKnowledge(); //calls m_serializer.setImplicitFunction()
      m_serializer.setGeometryXML( m_view->getAppliedGeometryXML() );
    }

    /**
    Mantid properties for rebinning algorithm require formatted information.
    @param dimension : dimension to extract property value for.
    @return true available, false otherwise.
    */
    template<typename ViewType>
    std::string MDEWRebinningPresenter<ViewType>::extractFormattedPropertyFromDimension(Mantid::Geometry::IMDDimension_sptr dimension) const
    {
      double min = dimension->getMinimum();
      double max = dimension->getMaximum();
      size_t nbins = dimension->getNBins();
      std::string id = dimension->getDimensionId();
      return boost::str(boost::format("%s, %f, %f, %d") %id %min %max % nbins);
    }

    /**
    Direct Mantid Algorithms and Workspaces to produce a visual dataset.
    @param factory : Factory, or chain of factories used for Workspace to VTK dataset conversion.
    @param eventHandler : Handler for GUI updates while algorithm progresses.
    */
    template<typename ViewType>
    vtkDataSet* MDEWRebinningPresenter<ViewType>::execute(vtkDataSetFactory* factory, ProgressAction& eventHandler)
    {
      using namespace Mantid::API;
      if(RecalculateAll == m_request->action())
      {

        std::string wsName = m_serializer.getWorkspaceName();

        Mantid::Geometry::MDGeometryXMLParser sourceGeometry(m_view->getAppliedGeometryXML());
        sourceGeometry.execute();

        Mantid::MDEvents::BinToMDHistoWorkspace hist_alg;
        hist_alg.initialize();

        hist_alg.setPropertyValue("InputWorkspace", wsName);
        std::string id; 
        if(sourceGeometry.hasXDimension())
        {
          hist_alg.setPropertyValue("DimX",  extractFormattedPropertyFromDimension(sourceGeometry.getXDimension())); 
        }
        if(sourceGeometry.hasYDimension())
        {
          hist_alg.setPropertyValue("DimY",  extractFormattedPropertyFromDimension(sourceGeometry.getYDimension())); 
        }
        if(sourceGeometry.hasZDimension())
        {
          hist_alg.setPropertyValue("DimZ",  extractFormattedPropertyFromDimension(sourceGeometry.getZDimension())); 
        }
        if(sourceGeometry.hasTDimension())
        {
          hist_alg.setPropertyValue("DimT",  extractFormattedPropertyFromDimension(sourceGeometry.getTDimension())); 
        }
        if(m_view->getApplyClip())
        {
          hist_alg.setPropertyValue("ImplicitFunctionXML", m_plane->toXMLString());
        }
        hist_alg.setPropertyValue("OutputWorkspace", "Histo_ws");
        Poco::NObserver<ProgressAction, Mantid::API::Algorithm::ProgressNotification> observer(eventHandler, &ProgressAction::handler);
        //Add observer.
        hist_alg.addObserver(observer);
        //Run the rebinning algorithm.
        hist_alg.execute();
        //Remove observer
        hist_alg.removeObserver(observer);
      }

      Mantid::API::Workspace_sptr result=Mantid::API::AnalysisDataService::Instance().retrieve("Histo_ws");

      factory->initialize(result);

      //vtkUnstructuredGrid* temp = static_cast<vtkUnstructuredGrid*>(factory->create());
      vtkDataSet* temp = factory->create();

      persistReductionKnowledge(temp, this->m_serializer, XMLDefinitions::metaDataId().c_str());
      m_request->reset();
      return temp;
    }


    template<typename ViewType>
    const std::string& MDEWRebinningPresenter<ViewType>::getAppliedGeometryXML() const
    {
      return m_serializer.getWorkspaceGeometry();
    }

    template<typename ViewType>
    bool MDEWRebinningPresenter<ViewType>::hasTDimensionAvailable() const
    {
      Mantid::Geometry::MDGeometryXMLParser sourceGeometry(m_serializer.getWorkspaceGeometry());
      sourceGeometry.execute();
      return sourceGeometry.hasTDimension();
    }

    template<typename ViewType>
    std::vector<double> MDEWRebinningPresenter<ViewType>::getTimeStepValues() const
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
    std::string MDEWRebinningPresenter<ViewType>::findExistingWorkspaceName(vtkDataSet *inputDataSet, const char* id)
    {
      using Mantid::Geometry::MDGeometryXMLDefinitions;
      FieldDataToMetadata convert;
      std::string xmlString = convert(inputDataSet->GetFieldData(), id);

      Poco::XML::DOMParser pParser;
      Poco::XML::Document* pDoc = pParser.parseString(xmlString);
      Poco::XML::Element* pRootElem = pDoc->documentElement();
      Poco::XML::Element* wsNameElem = pRootElem->getChildElement(MDGeometryXMLDefinitions::workspaceNameElementName());
      if(wsNameElem == NULL)
      {
        throw std::runtime_error("The element containing the workspace name must be present.");
      }
      return wsNameElem->innerText();
    }

    template<typename ViewType>
    std::string MDEWRebinningPresenter<ViewType>::findExistingWorkspaceLocation(vtkDataSet *inputDataSet, const char* id)
    {
      using Mantid::Geometry::MDGeometryXMLDefinitions;
      FieldDataToMetadata convert;
      std::string xmlString = convert(inputDataSet->GetFieldData(), id);

      Poco::XML::DOMParser pParser;
      Poco::XML::Document* pDoc = pParser.parseString(xmlString);
      Poco::XML::Element* pRootElem = pDoc->documentElement();
      Poco::XML::Element* wsLocationElem = pRootElem->getChildElement(MDGeometryXMLDefinitions::workspaceLocationElementName());
      if(wsLocationElem == NULL)
      {
        return "";
      }
      return wsLocationElem->innerText();
    }

    template<typename ViewType>
    void MDEWRebinningPresenter<ViewType>::persistReductionKnowledge(vtkDataSet* out_ds, const
      RebinningKnowledgeSerializer& xmlGenerator, const char* id)
    {
      vtkFieldData* fd = vtkFieldData::New();

      MetadataToFieldData convert;
      convert(fd, xmlGenerator.createXMLString().c_str(), id);

      out_ds->SetFieldData(fd);
      fd->Delete();
    }

    template<typename ViewType>
    Mantid::API::ImplicitFunction*  MDEWRebinningPresenter<ViewType>::findExistingRebinningDefinitions(
      vtkDataSet* inputDataSet, const char* id)
    {
      using Mantid::Geometry::MDGeometryXMLDefinitions;
      Mantid::API::ImplicitFunction* function = NULL;

      FieldDataToMetadata convert;
      std::string xmlString = convert(inputDataSet->GetFieldData(), id);
      if (false == xmlString.empty())
      {
        Poco::XML::DOMParser pParser;
        Poco::XML::Document* pDoc = pParser.parseString(xmlString);
        Poco::XML::Element* pRootElem = pDoc->documentElement();
        Poco::XML::Element* functionElem = pRootElem->getChildElement(MDGeometryXMLDefinitions::functionElementName());
        if(NULL != functionElem)
        {
          function = Mantid::API::ImplicitFunctionFactory::Instance().createUnwrapped(functionElem);
        }
      }
      return function;
    }

  }
}

#endif