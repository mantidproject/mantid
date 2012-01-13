#include "MantidVatesAPI/MDEWRebinningPresenter.h"
#include "MantidVatesAPI/MDRebinningView.h"
#include "MantidGeometry/MDGeometry/MDGeometryXMLDefinitions.h"
#include "MantidGeometry/MDGeometry/MDGeometryXMLBuilder.h"
#include "MantidMDAlgorithms/CompositeImplicitFunction.h"
#include "MantidMDAlgorithms/NullImplicitFunction.h"
#include "MantidVatesAPI/RebinningActionManager.h"
#include "MantidVatesAPI/ProgressAction.h"
#include "MantidVatesAPI/vtkDataSetToGeometry.h"
#include "MantidVatesAPI/RebinningCutterXMLDefinitions.h"
#include "MantidVatesAPI/FieldDataToMetadata.h"
#include "MantidVatesAPI/MetadataToFieldData.h"
#include "MantidVatesAPI/vtkDataSetFactory.h"
#include "MantidVatesAPI/WorkspaceProvider.h"
#include "MantidVatesAPI/vtkDataSetToImplicitFunction.h"
#include "MantidVatesAPI/vtkDataSetToWsLocation.h"
#include "MantidVatesAPI/vtkDataSetToWsName.h"
#include "MantidMDEvents/BinMD.h"
#include "MantidAPI/ImplicitFunctionFactory.h"
#include "MantidKernel/VMD.h"
#include <vtkDataSet.h>
#include <vtkFieldData.h>
#include <vtkPlane.h>


namespace Mantid
{
  namespace VATES
  {
    
    /**
    Constructor.
    @param input : input vtk dataset containing existing metadata.
    @param request : object performing decision making on what rebinning action to take.
    @param view : mvp view handle to use.
    @param wsProvider : ref to object used to determine the availability of the correct ws for this prsenter to work on.
    */
    MDEWRebinningPresenter::MDEWRebinningPresenter(vtkDataSet* input, RebinningActionManager* request, MDRebinningView* view, const WorkspaceProvider& wsProvider) :
    m_inputParser(input),
      m_input(input), 
      m_request(request), 
      m_view(view), 
      m_maxThreshold(0),
      m_minThreshold(0),
      m_timestep(0),
      m_wsGeometry(""),
      m_serializer(LocationNotRequired),
      m_function(Mantid::Geometry::MDImplicitFunction_sptr(new Mantid::MDAlgorithms::NullImplicitFunction())),
      m_applyClipping(false)
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

    }

    /// Destructor
    MDEWRebinningPresenter::~MDEWRebinningPresenter()
    {
      delete m_view;
    }

    /*
    Records and accumulates function knowledge so that it can be seralized to xml later.
    */
    void MDEWRebinningPresenter::addFunctionKnowledge()
    {
      //Add existing functions.
      Mantid::MDAlgorithms::CompositeImplicitFunction* compFunction = new Mantid::MDAlgorithms::CompositeImplicitFunction;
      compFunction->addFunction(m_function);
      Mantid::Geometry::MDImplicitFunction* existingFunctions = vtkDataSetToImplicitFunction::exec(m_input);
      if (existingFunctions != NULL)
      {
        compFunction->addFunction(Mantid::Geometry::MDImplicitFunction_sptr(existingFunctions));
      }
      //Apply the implicit function.
      m_serializer.setImplicitFunction(Mantid::Geometry::MDImplicitFunction_sptr(compFunction));
    }

    /**
    Uses the state of the MVP view to determine what rebinning action to take next. Also updates the internal
    members according to the state of the view so that the 'Delta' between the view and this presenter can 
    be compared and determined again at a later point.
    */
    void MDEWRebinningPresenter::updateModel()
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
        m_applyClipping = hasAppliedClipping;
        m_request->ask(RecalculateAll);
      }

      //Should always do clipping comparison if clipping has been set to on.
      if(m_applyClipping == true)
      {
        using Mantid::Kernel::V3D;
        //Check all parameters, which define the clipping.
        V3D temp_origin = m_view->getOrigin();
        V3D temp_b1 = m_view->getB1();
        V3D temp_b2 = m_view->getB2();
        double temp_length_b1 = m_view->getLengthB1();
        double temp_length_b2 = m_view->getLengthB2();
        double temp_length_b3 = m_view->getLengthB3();

        if(temp_origin != m_origin)
        {
          m_request->ask(RecalculateAll);
        }
        if(temp_b1 != m_b1)
        {
          m_request->ask(RecalculateAll);
        }
        if(temp_b2 != m_b2)
        {
          m_request->ask(RecalculateAll);
        }
        if(temp_length_b1 != m_lengthB1)
        {
          m_request->ask(RecalculateAll);
        }
        if(temp_length_b2 != m_lengthB2)
        {
          m_request->ask(RecalculateAll);
        }
        if(temp_length_b3 != m_lengthB3)
        {
          m_request->ask(RecalculateAll);
        }
        if(m_view->getForceOrthogonal() != m_ForceOrthogonal)
        {
          m_request->ask(RecalculateAll);
        }
        //Update coord transform fields.
        m_origin = temp_origin;
        m_b1 = temp_b1;
        m_b2 = temp_b2;
        m_lengthB1 = temp_length_b1;
        m_lengthB2 = temp_length_b2;
        m_lengthB3 = temp_length_b3;
        m_ForceOrthogonal = m_view->getForceOrthogonal();
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
    std::string MDEWRebinningPresenter::extractFormattedPropertyFromDimension(Mantid::Geometry::IMDDimension_sptr dimension) const
    {
      double min = dimension->getMinimum();
      double max = dimension->getMaximum();
      size_t nbins = dimension->getNBins();
      std::string id = dimension->getDimensionId();
      return boost::str(boost::format("%s, %f, %f, %d") %id %min %max % nbins);
    }

    /**
    Mantid properties for rebinning algorithm require formatted information.
    @param basis : basis vector.
    @param length : length of vector?
    @param dimension : dimension to extract property value for.
    @return true available, false otherwise.
    */
    std::string MDEWRebinningPresenter::extractFormattedPropertyFromDimension(const Mantid::Kernel::VMD& basis, double length,  Mantid::Geometry::IMDDimension_sptr dimension) const
    {
      std::string units = dimension->getUnits();
      size_t nbins = dimension->getNBins();
      std::string id = dimension->getDimensionId();
      return boost::str(boost::format("%s, %s, %s, %d, %f") %id %units %basis.toString(",") %length % nbins);
    }

    /**
    Direct Mantid Algorithms and Workspaces to produce a visual dataset.
    @param factory : Factory, or chain of factories used for Workspace to VTK dataset conversion.
    @param eventHandler : Handler for GUI updates while algorithm progresses.
    */
    vtkDataSet* MDEWRebinningPresenter::execute(vtkDataSetFactory* factory, ProgressAction& eventHandler)
    {
      std::string wsName = m_serializer.getWorkspaceName();
      // Create a private output workspace name
      std::string outWsName = wsName + "_mdhisto";

      using namespace Mantid::API;
      if(RecalculateAll == m_request->action())
      {
        Mantid::Geometry::MDGeometryXMLParser sourceGeometry(m_view->getAppliedGeometryXML());
        sourceGeometry.execute();

        Mantid::MDEvents::BinMD hist_alg;
        hist_alg.initialize();

        hist_alg.setPropertyValue("InputWorkspace", wsName); 

        if(m_view->getApplyClip())
        {
          using namespace Mantid::Kernel;

          V3D b3 = m_b1.cross_prod(m_b2);
          hist_alg.setPropertyValue("Origin", VMD(m_origin).toString(",") );
          hist_alg.setProperty("AxisAligned", false);
          hist_alg.setProperty("ForceOrthogonal", m_ForceOrthogonal );
          if(sourceGeometry.hasXDimension())
          {
            hist_alg.setPropertyValue("BasisVectorX", extractFormattedPropertyFromDimension(VMD(m_b1), m_lengthB1, sourceGeometry.getXDimension()));
          }
          if(sourceGeometry.hasYDimension())
          {
            hist_alg.setPropertyValue("BasisVectorY", extractFormattedPropertyFromDimension(VMD(m_b2), m_lengthB2, sourceGeometry.getYDimension()));
          }
          if(sourceGeometry.hasZDimension())
          {
            hist_alg.setPropertyValue("BasisVectorZ", extractFormattedPropertyFromDimension(VMD(b3), m_lengthB3, sourceGeometry.getZDimension()));
          }
          if(sourceGeometry.hasTDimension())
          {
            hist_alg.setPropertyValue("BasisVectorT", "");
          }
        }
        else
        {
          hist_alg.setProperty("AxisAligned", true);
          if(sourceGeometry.hasXDimension())
          {
            hist_alg.setPropertyValue("AlignedDimX",  extractFormattedPropertyFromDimension(sourceGeometry.getXDimension()));
          }
          if(sourceGeometry.hasYDimension())
          {
            hist_alg.setPropertyValue("AlignedDimY",  extractFormattedPropertyFromDimension(sourceGeometry.getYDimension()));
          }
          if(sourceGeometry.hasZDimension())
          {
            hist_alg.setPropertyValue("AlignedDimZ",  extractFormattedPropertyFromDimension(sourceGeometry.getZDimension()));
          }
          if(sourceGeometry.hasTDimension())
          {
            hist_alg.setPropertyValue("AlignedDimT",  extractFormattedPropertyFromDimension(sourceGeometry.getTDimension()));
          }
        }
        hist_alg.setPropertyValue("Parallel", "1");

        hist_alg.setPropertyValue("OutputWorkspace", outWsName);
        Poco::NObserver<ProgressAction, Mantid::API::Algorithm::ProgressNotification> observer(eventHandler, &ProgressAction::handler);
        //Add observer.
        hist_alg.addObserver(observer);
        //Run the rebinning algorithm.
        hist_alg.execute();
        //Remove observer
        hist_alg.removeObserver(observer);
      }

      Mantid::API::Workspace_sptr result=Mantid::API::AnalysisDataService::Instance().retrieve(outWsName);

      factory->initialize(result);

      vtkDataSet* temp = factory->create();

      persistReductionKnowledge(temp, this->m_serializer, XMLDefinitions::metaDataId().c_str());
      m_request->reset();
      return temp;
    }

    const std::string& MDEWRebinningPresenter::getAppliedGeometryXML() const
    {
      return m_serializer.getWorkspaceGeometry();
    }

    bool MDEWRebinningPresenter::hasTDimensionAvailable() const
    {
      Mantid::Geometry::MDGeometryXMLParser sourceGeometry(m_view->getAppliedGeometryXML());
      sourceGeometry.execute();
      return sourceGeometry.hasTDimension() && (sourceGeometry.getTDimension()->getNBins() > 1);
    }

    std::vector<double> MDEWRebinningPresenter::getTimeStepValues() const
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

    void MDEWRebinningPresenter::persistReductionKnowledge(vtkDataSet* out_ds, const
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
