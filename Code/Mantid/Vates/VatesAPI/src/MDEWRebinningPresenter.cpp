#include "MantidVatesAPI/MDEWRebinningPresenter.h"
#include "MantidVatesAPI/MDRebinningView.h"
#include "MantidGeometry/MDGeometry/MDGeometryXMLDefinitions.h"
#include "MantidGeometry/MDGeometry/MDGeometryXMLBuilder.h"
#include "MantidGeometry/MDGeometry/CompositeImplicitFunction.h"
#include "MantidGeometry/MDGeometry/NullImplicitFunction.h"
#include "MantidVatesAPI/RebinningActionManager.h"
#include "MantidVatesAPI/ProgressAction.h"
#include "MantidVatesAPI/vtkDataSetToGeometry.h"
#include "MantidVatesAPI/RebinningCutterXMLDefinitions.h"
#include "MantidVatesAPI/FieldDataToMetadata.h"
#include "MantidVatesAPI/MetadataToFieldData.h"
#include "MantidVatesAPI/MetadataJsonManager.h"
#include "MantidVatesAPI/vtkDataSetFactory.h"
#include "MantidVatesAPI/WorkspaceProvider.h"
#include "MantidVatesAPI/vtkDataSetToImplicitFunction.h"
#include "MantidVatesAPI/vtkDataSetToNonOrthogonalDataSet.h"
#include "MantidVatesAPI/vtkDataSetToWsLocation.h"
#include "MantidVatesAPI/vtkDataSetToWsName.h"
#include "MantidVatesAPI/VatesConfigurations.h"
#include "MantidVatesAPI/Common.h"
#include "MantidAPI/AlgorithmManager.h"
#include "MantidAPI/ImplicitFunctionFactory.h"
#include "MantidKernel/VMD.h"
#include <vtkDataSet.h>
#include <vtkFieldData.h>
#include <vtkPlane.h>

using namespace Mantid::API;
using namespace Mantid::Kernel;

namespace Mantid
{
  namespace VATES
  {
    const std::string MDEWRebinningPresenter::rb_tag = "_visual_md";

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
      m_function(Mantid::Geometry::MDImplicitFunction_sptr(new Mantid::Geometry::NullImplicitFunction())),
      m_applyClipping(false),
      m_lengthB1(1),
      m_lengthB2(1),
      m_lengthB3(1),
      m_ForceOrthogonal(true),
      m_bOutputHistogramWS(true),
      m_instrument(""),
      m_metadataJsonManager(new MetadataJsonManager()),
      m_vatesConfigurations(new VatesConfigurations())
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
      using Mantid::Geometry::NoDimensionPolicy;
      MDGeometryBuilderXML<NoDimensionPolicy> xmlBuilder;

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

      // Extract Json metadata from the field data
      if(NULL == fd || NULL == fd->GetArray(m_vatesConfigurations->getMetadataIdJson().c_str()))
      {
        throw std::logic_error("Rebinning operations require Rebinning Json Metadata");
      }

      FieldDataToMetadata fieldDataToMetadata;

      std::string jsonString = fieldDataToMetadata(fd, m_vatesConfigurations->getMetadataIdJson());
      m_metadataJsonManager->readInSerializedJson(jsonString);

      m_instrument = m_metadataJsonManager->getInstrument();
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
      Mantid::Geometry::CompositeImplicitFunction* compFunction = new Mantid::Geometry::CompositeImplicitFunction;
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
      const bool bOutputHistogramWS = m_view->getOutputHistogramWS();
      if(bOutputHistogramWS != m_bOutputHistogramWS)
      {
        m_request->ask(RecalculateAll);
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
        m_bOutputHistogramWS = m_view->getOutputHistogramWS();
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
      m_bOutputHistogramWS = bOutputHistogramWS;
      addFunctionKnowledge(); //calls m_serializer.setImplicitFunction()
      m_serializer.setGeometryXML( m_view->getAppliedGeometryXML() );
    }

    /** Mantid properties for rebinning algorithm require formatted information.
     * This is for the AlignedDim0... props
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

    /**  Mantid properties for rebinning algorithm require formatted information.
     * This is for the BasisVector0... parameters
     *
    @param basis : basis vector.
    @param totalNDims : total number of dimensions.
    @param length : length of vector?
    @param dimension : dimension to extract property value for.
    @return true available, false otherwise.
    */
    std::string MDEWRebinningPresenter::extractFormattedPropertyFromDimension(const Mantid::Kernel::V3D& basis, const size_t totalNDims, double length,  Mantid::Geometry::IMDDimension_sptr dimension) const
    {
      UNUSED_ARG(length);

      MantidVec localBasis(3);
      localBasis[0] = basis.X();
      localBasis[1] = basis.Y();
      localBasis[2] = basis.Z();
      size_t nAdditionalDimensions = totalNDims - 3;
      if(nAdditionalDimensions >= 1)
      {
        for(size_t i = 0; i < nAdditionalDimensions; ++i)
        {
          localBasis.push_back(0);
        }
      }

      std::string units = dimension->getUnits();
      std::string id = dimension->getDimensionId();
      return boost::str(boost::format("%s, %s, %s") %id %units %VMD(localBasis).toString(",") );
    }

    /**
    Direct Mantid Algorithms and Workspaces to produce a visual dataset.
    @param factory : Factory, or chain of factories used for Workspace to VTK dataset conversion.
    @param rebinningProgressUpdate : Handler for GUI updates while algorithm progresses.
    @param drawingProgressUpdate : Handler for GUI updates while vtkDataSetFactory::create occurs.
    */
    vtkDataSet* MDEWRebinningPresenter::execute(vtkDataSetFactory* factory, ProgressAction& rebinningProgressUpdate, ProgressAction& drawingProgressUpdate)
    {
      std::string wsName = m_serializer.getWorkspaceName();

      std::string outWsName = wsName + rb_tag;

      using namespace Mantid::API;
      if(RecalculateAll == m_request->action())
      {
        Mantid::Geometry::MDGeometryXMLParser sourceGeometry(m_view->getAppliedGeometryXML());
        sourceGeometry.execute();

        IAlgorithm_sptr binningAlg  =  m_bOutputHistogramWS  ? AlgorithmManager::Instance().create("BinMD") : AlgorithmManager::Instance().create("SliceMD");
        binningAlg->initialize();
        binningAlg->setPropertyValue("InputWorkspace", wsName); 
        if(!m_bOutputHistogramWS)
        {
          //SliceMD only! This means that iterators will only go through top-level boxes. So iterators will always hit boxes worth visualising.
          binningAlg->setProperty("TakeMaxRecursionDepthFromInput", false);
          binningAlg->setProperty("MaxRecursionDepth", 1) ;
        }

        if(m_view->getApplyClip())
        {
          using namespace Mantid::Kernel;
          const size_t totalNDims = sourceGeometry.getAllDimensions().size();
          V3D b3 = m_b1.cross_prod(m_b2);
          binningAlg->setPropertyValue("Translation", VMD(m_origin).toString(",") );
          binningAlg->setProperty("AxisAligned", false);
          binningAlg->setProperty("ForceOrthogonal", m_ForceOrthogonal );
          std::vector<int> OutputBins;
          std::vector<double> OutputExtents;
          if(sourceGeometry.hasXDimension())
          {
            binningAlg->setPropertyValue("BasisVector0", extractFormattedPropertyFromDimension(m_b1, totalNDims, m_lengthB1, sourceGeometry.getXDimension()));
            OutputExtents.push_back(0);
            OutputExtents.push_back(m_lengthB1);
            OutputBins.push_back(int(sourceGeometry.getXDimension()->getNBins()));
          }
          if(sourceGeometry.hasYDimension())
          {
            binningAlg->setPropertyValue("BasisVector1", extractFormattedPropertyFromDimension(m_b2, totalNDims, m_lengthB2, sourceGeometry.getYDimension()));
            OutputExtents.push_back(0);
            OutputExtents.push_back(m_lengthB2);
            OutputBins.push_back(int(sourceGeometry.getYDimension()->getNBins()));
          }
          if(sourceGeometry.hasZDimension())
          {
            binningAlg->setPropertyValue("BasisVector2", extractFormattedPropertyFromDimension(b3, totalNDims, m_lengthB3, sourceGeometry.getZDimension()));
            OutputExtents.push_back(0);
            OutputExtents.push_back(m_lengthB3);
            OutputBins.push_back(int(sourceGeometry.getYDimension()->getNBins()));
          }
          if(sourceGeometry.hasTDimension())
          {
            // Create a basis vector parallel to the current time vector.
            auto dimT = sourceGeometry.getTDimension();
            std::string units = dimT->getUnits();
            std::string id = dimT->getDimensionId();
            std::string formattedTInput = boost::str(boost::format("%s, %s, 0,0,0,1") %id %units); 
            binningAlg->setPropertyValue("BasisVector3", formattedTInput);

            // Set up extents and bins for this dimension.
            OutputExtents.push_back(dimT->getMinimum());
            OutputExtents.push_back(dimT->getMaximum());
            OutputBins.push_back(int(dimT->getNBins()));

            // Overwrite the translation.
            binningAlg->setPropertyValue("Translation", boost::str(boost::format("%s, 0") %VMD(m_origin).toString(",")) );
          }
          binningAlg->setProperty("OutputExtents", OutputExtents);
          binningAlg->setProperty("OutputBins", OutputBins);
        }
        else
        {
          binningAlg->setProperty("AxisAligned", true);
          if(sourceGeometry.hasXDimension())
          {
            binningAlg->setPropertyValue("AlignedDim0",  extractFormattedPropertyFromDimension(sourceGeometry.getXDimension()));
          }
          if(sourceGeometry.hasYDimension())
          {
            binningAlg->setPropertyValue("AlignedDim1",  extractFormattedPropertyFromDimension(sourceGeometry.getYDimension()));
          }
          if(sourceGeometry.hasZDimension())
          {
            binningAlg->setPropertyValue("AlignedDim2",  extractFormattedPropertyFromDimension(sourceGeometry.getZDimension()));
          }
          if(sourceGeometry.hasTDimension())
          {
            binningAlg->setPropertyValue("AlignedDim3", extractFormattedPropertyFromDimension(sourceGeometry.getTDimension()));
          }
        }

        binningAlg->setPropertyValue("OutputWorkspace", outWsName);
        Poco::NObserver<ProgressAction, Mantid::API::Algorithm::ProgressNotification> observer(rebinningProgressUpdate, &ProgressAction::handler);
        //Add observer.
        binningAlg->addObserver(observer);
        //Run the rebinning algorithm.
        binningAlg->setRethrows(true);
        binningAlg->execute();
        //Remove observer
        binningAlg->removeObserver(observer);
      }

      Mantid::API::Workspace_sptr result=Mantid::API::AnalysisDataService::Instance().retrieve(outWsName);

      vtkDataSet* temp = factory->oneStepCreate(result, drawingProgressUpdate); 

      // Extract min and max range of the data set and update the json store
      double* range = temp->GetScalarRange();
      
      if (range)
      {
        m_metadataJsonManager->setMinValue(range[0]);
        m_metadataJsonManager->setMaxValue(range[1]);
      }

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
      return sourceGeometry.hasTDimension();
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

    /**
     * Create a label for the "time" coordinate
     * @return the "time" coordinate label
     */
    std::string MDEWRebinningPresenter::getTimeStepLabel() const
    {
      Mantid::Geometry::MDGeometryXMLParser sourceGeometry(m_view->getAppliedGeometryXML());
      sourceGeometry.execute();
      std::string label = sourceGeometry.getTDimension()->getName();
      label += " (";
      label += sourceGeometry.getTDimension()->getUnits();
      label += ")";
      return label;
    }

    void MDEWRebinningPresenter::makeNonOrthogonal(vtkDataSet *visualDataSet)
    {
      std::string wsName = m_serializer.getWorkspaceName() + rb_tag;
      vtkDataSetToNonOrthogonalDataSet converter(visualDataSet, wsName);
      converter.execute();
    }

    void MDEWRebinningPresenter::setAxisLabels(vtkDataSet *visualDataSet)
    {
      Mantid::Geometry::MDGeometryXMLParser sourceGeometry(m_view->getAppliedGeometryXML());
      sourceGeometry.execute();
      vtkFieldData* fieldData = visualDataSet->GetFieldData();
      setAxisLabel("AxisTitleForX",
                   makeAxisTitle(sourceGeometry.getXDimension()), fieldData);
      setAxisLabel("AxisTitleForY",
                   makeAxisTitle(sourceGeometry.getYDimension()), fieldData);
      setAxisLabel("AxisTitleForZ",
                   makeAxisTitle(sourceGeometry.getZDimension()), fieldData);
    }

    void MDEWRebinningPresenter::persistReductionKnowledge(vtkDataSet* out_ds, const
      RebinningKnowledgeSerializer& xmlGenerator, const char* id)
    {
      vtkFieldData* fd = vtkFieldData::New();

      MetadataToFieldData convert;
      convert(fd, xmlGenerator.createXMLString().c_str(), id);

      // Add second entry for Json metadata
      std::string jsonMetadata = m_metadataJsonManager->getSerializedJson();
      convert(fd, jsonMetadata, m_vatesConfigurations->getMetadataIdJson());

      out_ds->SetFieldData(fd);
      fd->Delete();
    }

    /**
     * Get the maximum value of the data range.
     * @returns The maximum value.
     */
    double MDEWRebinningPresenter::getMaxValue() const
    {
      return this->m_metadataJsonManager->getMaxValue();
    }

    /**
     * Get the minimum value of the data range.
     * @retruns The maximum value of the data range.
     */
    double MDEWRebinningPresenter::getMinValue() const
    {
      return this->m_metadataJsonManager->getMinValue();
    }

    /**
     * Get the instrument associated with the workspace
     * @returns The instrument 
     */
    const std::string& MDEWRebinningPresenter::getInstrument() const
    {
      return m_instrument;
    }
  }
}
