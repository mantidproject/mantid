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
#include "MantidAPI/ImplicitFunction.h"
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
    template<typename ViewType>
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

      MDHistogramRebinningPresenter(vtkDataSet* input, RebinningActionManager* request, ViewType* view, Clipper* clipper, const WorkspaceProvider& wsProvider);

    private:

      std::string constructGeometryXML(
        Mantid::Geometry::VecIMDDimension_sptr dimensions,
        Mantid::Geometry::IMDDimension_sptr ,
        Mantid::Geometry::IMDDimension_sptr ,
        Mantid::Geometry::IMDDimension_sptr ,
        Mantid::Geometry::IMDDimension_sptr );

      void forumulateBinChangeRequest(Mantid::Geometry::MDGeometryXMLParser& old_geometry, Mantid::Geometry::MDGeometryXMLParser& new_geometry);

      /// Construct a box from the interactor.
      Mantid::API::ImplicitFunction_sptr constructBoxFromVTKBox(vtkBox* box) const;

      /// Construct a box from the input dataset metadata.
      Mantid::API::ImplicitFunction_sptr constructBoxFromInput() const;

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
      ViewType* m_view;
      ///Box implicit function used to determine boundaries via evaluation.
      Mantid::API::ImplicitFunction_sptr m_box;
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

    /*---------------------------------------------------------------------------------------------------------
    Templated Implementations.
    ---------------------------------------------------------------------------------------------------------*/

    /** Constructor
    * @param input : Input vtkdataset.
    * @param request : Request managing object
    * @param view : MVP view
    * @param clipper : Clipper for determining boundaries.
    */
    template<typename ViewType>
    MDHistogramRebinningPresenter<ViewType>::MDHistogramRebinningPresenter(vtkDataSet* input, RebinningActionManager* request, ViewType* view, Clipper* clipper, const WorkspaceProvider& wsProvider) :
    m_inputParser(input), 
      m_input(input), 
      m_request(request), 
      m_view(view), 
      m_box(new Mantid::MDAlgorithms::NullImplicitFunction()), 
      m_clipper(clipper),
      m_maxThreshold(0),
      m_minThreshold(0),
      m_applyClip(false),
      m_timestep(0),
      m_wsGeometry("")
    {
      using namespace Mantid::API;
      vtkFieldData* fd = input->GetFieldData();
      if(NULL == fd || NULL == fd->GetArray(XMLDefinitions::metaDataId().c_str()))
      {
        throw std::logic_error("Rebinning operations require Rebinning Metadata");
      }
      std::string wsName = vtkDataSetToWsName::exec(m_input);

      if(!wsProvider.canProvideWorkspace(wsName))
      {
        throw std::invalid_argument("Wrong type of Workspace stored. Cannot handle with this presenter");
      }

      Mantid::API::FrameworkManager::Instance();

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
      m_serializer.setWorkspaceLocation(vtkDataSetToWsLocation::exec(m_input));
      //Set-up a default box.
      m_box = constructBoxFromInput();
    }

    /** constructs geometry xml string from dimensions.
    * @param dimensions : all dimension
    * @param dimensionX : x mapping
    * @param dimensionY : y mapping
    * @param dimensionZ : z mapping
    * @param dimensionT : t mapping
    */
    template<typename ViewType>
    std::string MDHistogramRebinningPresenter<ViewType>::constructGeometryXML(
      DimensionVec dimensions,
      Dimension_sptr dimensionX,
      Dimension_sptr dimensionY,
      Dimension_sptr dimensionZ,
      Dimension_sptr dimensiont)
    {
      using Mantid::Geometry::MDGeometryBuilderXML;
      using Mantid::Geometry::StrictDimensionPolicy;
      MDGeometryBuilderXML<StrictDimensionPolicy> xmlBuilder;
      DimensionVec::iterator it = dimensions.begin();
      for(;it != dimensions.end(); ++it)
      {
        xmlBuilder.addOrdinaryDimension(*it);
      }
      xmlBuilder.addXDimension(dimensionX);
      xmlBuilder.addYDimension(dimensionY);
      xmlBuilder.addZDimension(dimensionZ);
      xmlBuilder.addTDimension(dimensiont);
      return xmlBuilder.create();
    }

    /** Uses changes in the number of bins for each mapped dimension to determine when to perform rebinning.
    * @param old_geometry : previous instance geometry
    * @param new_geometry : view instance geometry
    */
    template<typename ViewType>
    void MDHistogramRebinningPresenter<ViewType>::forumulateBinChangeRequest(Mantid::Geometry::MDGeometryXMLParser& old_geometry, Mantid::Geometry::MDGeometryXMLParser& new_geometry)
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


    /** Converts a vtkbox into an implicitfunction box.
    * @param box : vtkImplicitFunction.
    * @return ImplicitFunction_sptr containing ImplicitFunction box.
    */
    template<typename ViewType>
    Mantid::API::ImplicitFunction_sptr MDHistogramRebinningPresenter<ViewType>::constructBoxFromVTKBox(vtkBox* box) const
    {
      using namespace Mantid::MDAlgorithms;

      double originX, originY, originZ, width, height, depth;

      //To get the box bounds, we actually need to evaluate the box function. There is not this restriction on planes.
      m_clipper->SetInput(m_input);
      m_clipper->SetClipFunction(box);
      m_clipper->SetInsideOut(true);
      m_clipper->Update();
      vtkDataSet* clipperOutput = m_clipper->GetOutput();
      //Now we can get the bounds.
      double* bounds = clipperOutput->GetBounds();

      originX = (bounds[1] + bounds[0]) / 2;
      originY = (bounds[3] + bounds[2]) / 2;
      originZ = (bounds[5] + bounds[4]) / 2;
      width = sqrt(pow(bounds[1] - bounds[0], 2));
      height = sqrt(pow(bounds[3] - bounds[2], 2));
      depth = sqrt(pow(bounds[5] - bounds[4], 2));

      //Create domain parameters.
      OriginParameter originParam = OriginParameter(originX, originY, originZ);
      WidthParameter widthParam = WidthParameter(width);
      HeightParameter heightParam = HeightParameter(height);
      DepthParameter depthParam = DepthParameter(depth);

      //Create the box. This is specific to this type of presenter and this type of filter. Other rebinning filters may use planes etc.
      BoxImplicitFunction* boxFunction = new BoxImplicitFunction(widthParam, heightParam, depthParam,
        originParam);

      return Mantid::API::ImplicitFunction_sptr(boxFunction);
    }

    /** Constructs a box from the inputs vtkdataset.
    * @return ImplicitFunction_sptr containing ImplicitFunction box.
    */
    template<typename ViewType>
    Mantid::API::ImplicitFunction_sptr MDHistogramRebinningPresenter<ViewType>::constructBoxFromInput() const
    {
      using namespace Mantid::MDAlgorithms;

      double originX, originY, originZ, width, height, depth;

      vtkDataSetToGeometry metaDataProcessor(m_input);
      metaDataProcessor.execute();

      originX = (metaDataProcessor.getXDimension()->getMaximum() + metaDataProcessor.getXDimension()->getMinimum()) / 2;
      originY = (metaDataProcessor.getYDimension()->getMaximum() + metaDataProcessor.getYDimension()->getMinimum()) / 2;
      originZ = (metaDataProcessor.getZDimension()->getMaximum() + metaDataProcessor.getZDimension()->getMinimum()) / 2;
      width = metaDataProcessor.getXDimension()->getMaximum() - metaDataProcessor.getXDimension()->getMinimum();
      height = metaDataProcessor.getYDimension()->getMaximum() - metaDataProcessor.getYDimension()->getMinimum();
      depth = metaDataProcessor.getZDimension()->getMaximum() - metaDataProcessor.getZDimension()->getMinimum();

      //Create domain parameters.
      OriginParameter originParam = OriginParameter(originX, originY, originZ);
      WidthParameter widthParam = WidthParameter(width);
      HeightParameter heightParam = HeightParameter(height);
      DepthParameter depthParam = DepthParameter(depth);

      //Create the box. This is specific to this type of presenter and this type of filter. Other rebinning filters may use planes etc.
      BoxImplicitFunction* boxFunction = new BoxImplicitFunction(widthParam, heightParam, depthParam,
        originParam);

      return Mantid::API::ImplicitFunction_sptr(boxFunction);
    }

    /** Update the MVP model, forumulates and hive-off a request for rebinning
    */
    template<typename ViewType>
    void MDHistogramRebinningPresenter<ViewType>::updateModel()
    {
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
        //check it's a box.
        //extract a box.
        //compare boxes.
        vtkBox* box = dynamic_cast<vtkBox*>(m_view->getImplicitFunction());
        if(NULL != box && hasAppliedClipping)
        {
          m_box = constructBoxFromVTKBox(box);
        }
        else
        {
          m_box = constructBoxFromInput();
        }
        //m_request->ask(RecalculateAll);

        m_applyClip = m_view->getApplyClip();
      }

      //Should always do clipping comparison
      if(hasAppliedClipping == true)
      {
        vtkBox* box = dynamic_cast<vtkBox*>(m_view->getImplicitFunction());
        if(NULL != box)
        {
          boost::shared_ptr<Mantid::MDAlgorithms::BoxImplicitFunction> boxA = boost::dynamic_pointer_cast<Mantid::MDAlgorithms::BoxImplicitFunction>(constructBoxFromVTKBox(box));
          boost::shared_ptr<Mantid::MDAlgorithms::BoxImplicitFunction> boxB = boost::dynamic_pointer_cast<Mantid::MDAlgorithms::BoxImplicitFunction>(m_box);
          if(boxA->operator!=(*boxB.get()))
          {
            m_box = boxA;
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
        //Detect dimension swapping only of 4 dimensions.
        if(old_geometry.getNonIntegratedDimensions().size() == 4 && new_geometry.getNonIntegratedDimensions().size() == 4)
        {
          //TODO, can we add the proxy view onto the front of the m_chain here!
          m_request->ask(RecalculateVisualDataSetOnly);
        }
        else
        {
          m_request->ask(RecalculateAll);
        }
        forumulateBinChangeRequest(old_geometry, new_geometry);

        m_serializer.setGeometryXML( m_view->getAppliedGeometryXML() );
      }
    }

    template<typename ViewType>
    void MDHistogramRebinningPresenter<ViewType>::addFunctionKnowledge()
    {
      //Add existing functions.
      Mantid::MDAlgorithms::CompositeImplicitFunction* compFunction = new Mantid::MDAlgorithms::CompositeImplicitFunction;
      compFunction->addFunction(m_box);
      Mantid::API::ImplicitFunction* existingFunctions = vtkDataSetToImplicitFunction::exec(m_input);
      if (existingFunctions != NULL)
      {
        compFunction->addFunction(Mantid::API::ImplicitFunction_sptr(existingFunctions));
      }
      //Apply the implicit function.
      m_serializer.setImplicitFunction(Mantid::API::ImplicitFunction_sptr(compFunction));
    }

    /** Coordinate the production of a vtkDataSet matching the request
    @parameter factory : vtkDataSet producing factory chain.
    @parameter eventHandler : observer/listenter 
    */
    template<typename ViewType>
    vtkDataSet* MDHistogramRebinningPresenter<ViewType>::execute(vtkDataSetFactory* factory, ProgressAction& eventHandler)
    {
      using namespace Mantid::API;
      const std::string outputWorkspace = XMLDefinitions::RebinnedWSName();
      //Rebin using member variables.
      if(RecalculateAll == m_request->action())
      {
        //Get the input workspace location and name.
        std::string wsLocation = m_serializer.getWorkspaceLocation();
        std::string wsName = m_serializer.getWorkspaceName();

        Mantid::API::IMDWorkspace_sptr baseWs = constructMDWorkspace(wsLocation);
        AnalysisDataService::Instance().addOrReplace(wsName, baseWs);

        Mantid::MDAlgorithms::DynamicRebinFromXML xmlRebinAlg;
        xmlRebinAlg.setRethrows(true);
        xmlRebinAlg.initialize();

        xmlRebinAlg.setPropertyValue("OutputWorkspace", outputWorkspace);

        //Use the serialisation utility to generate well-formed xml expressing the rebinning operation.
        std::string xmlString = m_serializer.createXMLString();
        xmlRebinAlg.setPropertyValue("XMLInputString", xmlString);

        Poco::NObserver<ProgressAction, Mantid::API::Algorithm::ProgressNotification> observer(eventHandler, &ProgressAction::handler);
        //Add observer.
        xmlRebinAlg.addObserver(observer);
        //Run the rebinning algorithm.
        xmlRebinAlg.execute();
        //Remove observer
        xmlRebinAlg.removeObserver(observer);
      }

      //Use the generated workspace to access the underlying image, which may be rendered.
      IMDWorkspace_sptr outputWs = boost::dynamic_pointer_cast<IMDWorkspace>(
        AnalysisDataService::Instance().retrieve(outputWorkspace));

      Mantid::Geometry::MDGeometryXMLParser sourceGeometry(m_view->getAppliedGeometryXML());
      sourceGeometry.execute();
      if((m_request->action() == RecalculateVisualDataSetOnly) && sourceGeometry.hasXDimension() && sourceGeometry.hasYDimension() && sourceGeometry.hasZDimension() && sourceGeometry.hasTDimension())
      {
        IMDWorkspace_sptr proxy = IMDWorkspaceProxy::New(
          outputWs, 
          sourceGeometry.getXDimension(),
          sourceGeometry.getYDimension(),
          sourceGeometry.getZDimension(),
          sourceGeometry.getTDimension());
        factory->initialize(proxy);
      }
      else
      {
        factory->initialize(outputWs);
      }
      //vtkUnstructuredGrid* temp = static_cast<vtkUnstructuredGrid*>(factory->create());
      vtkDataSet* temp = factory->create();

      persistReductionKnowledge(temp, this->m_serializer, XMLDefinitions::metaDataId().c_str());
      m_request->reset();
      return temp;
    }

    template<typename ViewType>
    const std::string& MDHistogramRebinningPresenter<ViewType>::getAppliedGeometryXML() const
    {
      return m_serializer.getWorkspaceGeometry();
    }

    template<typename ViewType>
    MDHistogramRebinningPresenter<ViewType>::~MDHistogramRebinningPresenter()
    {
    }

    template<typename ViewType>
    bool
      MDHistogramRebinningPresenter<ViewType>::hasTDimensionAvailable() const
    {
      Mantid::Geometry::MDGeometryXMLParser sourceGeometry(m_serializer.getWorkspaceGeometry());
      sourceGeometry.execute();
      return sourceGeometry.getTDimension();
    }

    template<typename ViewType>
    std::vector<double> MDHistogramRebinningPresenter<ViewType>::getTimeStepValues() const
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
    Mantid::API::IMDWorkspace_sptr MDHistogramRebinningPresenter<ViewType>::constructMDWorkspace(const std::string& wsLocation)
    {
      using namespace Mantid::MDDataObjects;
      using namespace Mantid::Geometry;
      using namespace Mantid::API;

      Mantid::MDAlgorithms::Load_MDWorkspace wsLoaderAlg;
      wsLoaderAlg.initialize();
      std::string wsId = "InputMDWs";
      wsLoaderAlg.setPropertyValue("inFilename", wsLocation);
      wsLoaderAlg.setPropertyValue("MDWorkspace", wsId);
      wsLoaderAlg.execute();
      Workspace_sptr result=AnalysisDataService::Instance().retrieve(wsId);
      MDWorkspace_sptr workspace = boost::dynamic_pointer_cast<MDWorkspace>(result);

      return workspace;
    }

    template<typename ViewType>
    void MDHistogramRebinningPresenter<ViewType>::persistReductionKnowledge(vtkDataSet* out_ds, const
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
