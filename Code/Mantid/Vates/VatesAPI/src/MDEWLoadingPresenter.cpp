#include "MantidVatesAPI/MDEWLoadingPresenter.h"
#include "MantidVatesAPI/MDLoadingView.h"
#include "MantidAPI/FrameworkManager.h"

#include "MantidGeometry/MDGeometry/NullImplicitFunction.h"
#include "MantidVatesAPI/RebinningKnowledgeSerializer.h"
#include "MantidVatesAPI/MetadataToFieldData.h"
#include "MantidVatesAPI/RebinningCutterXMLDefinitions.h"
#include "MantidVatesAPI/Common.h"

#include <boost/algorithm/string.hpp>
#include <vtkFieldData.h>
#include <vtkDataSet.h>

namespace Mantid
{
  namespace VATES
  {
    /// Constructor
    MDEWLoadingPresenter::MDEWLoadingPresenter(MDLoadingView* view) : 
    m_view(view), 
    m_isSetup(false), 
    m_time(-1),
    m_recursionDepth(0),
    m_loadInMemory(false),
    m_firstLoad(true),
    m_instrument("")
    {
      Mantid::API::FrameworkManager::Instance();
    }

    /// Destructor
    MDEWLoadingPresenter::~MDEWLoadingPresenter()
    {
    }

     /*
    Extract the geometry and function information 
    @param eventWs : event workspace to get the information from.
    */
    void MDEWLoadingPresenter::extractMetadata(Mantid::API::IMDEventWorkspace_sptr eventWs)
    {
      using namespace Mantid::Geometry;
      MDGeometryBuilderXML<NoDimensionPolicy> refresh;
      xmlBuilder= refresh; //Reassign.
      std::vector<MDDimensionExtents<coord_t> > ext = eventWs->getMinimumExtents(5);
      std::vector<IMDDimension_sptr> dimensions;
      size_t nDimensions = eventWs->getNumDims();
      for (size_t d=0; d<nDimensions; d++)
      {
        IMDDimension_const_sptr inDim = eventWs->getDimension(d);
        coord_t min = ext[d].getMin();
        coord_t max = ext[d].getMax();
        if (min > max)
        {
          min = 0.0;
          max = 1.0;
        }
        //std::cout << "dim " << d << min << " to " <<  max << std::endl;
        axisLabels.push_back(makeAxisTitle(inDim));
        MDHistoDimension_sptr dim(new MDHistoDimension(inDim->getName(), inDim->getName(), inDim->getUnits(), min, max, inDim->getNBins()));
        dimensions.push_back(dim);
      }

      //Configuring the geometry xml builder allows the object panel associated with this reader to later
      //determine how to display all geometry related properties.
      if(nDimensions > 0)
      {
        xmlBuilder.addXDimension( dimensions[0] );
      }
      if(nDimensions > 1)
      {
        xmlBuilder.addYDimension( dimensions[1] );
      }
      if(nDimensions > 2)
      {
        xmlBuilder.addZDimension( dimensions[2]  );
      }
      if(nDimensions > 3)
      {
        tDimension = dimensions[3];
        xmlBuilder.addTDimension(tDimension);
      }
      m_isSetup = true;
    }

    /**
    Method determines whether loading/re-loading is necessary.
    */
    bool MDEWLoadingPresenter::shouldLoad()
    {
      double viewTime = m_view->getTime();
      size_t viewDepth = m_view->getRecursionDepth();
      bool viewLoadInMemory = m_view->getLoadInMemory();

      bool bExecute = false;
      if(m_time != viewTime)
      {
        bExecute = false; //Time has changed. This DOES NOT require reloading.
      }
      if(m_recursionDepth != viewDepth)
      {
        bExecute = false; //Recursion depth has changed. This is a vtkDataSet factory concern.
      }
      if(m_loadInMemory != viewLoadInMemory)
      {
        bExecute = true; //Must reload with memory/file option.
      }
      if(m_firstLoad)
      {
        bExecute = true; //First time round. should execute underlying algorithm.
      }

      // Save state.
      m_time = viewTime;
      m_recursionDepth = viewDepth;
      m_loadInMemory = viewLoadInMemory;
      m_firstLoad = false;
      //Return decision.
      return bExecute;
    }

    /**
    Determines wheter the file can be loaded based on it's extension.
    @param filename containing the extension
    @param expectedExtension expected extension for the file to have
    @return TRUE, only if the extension is approved.
    */
    bool MDEWLoadingPresenter::canLoadFileBasedOnExtension(const std::string& filename, const std::string& expectedExtension) const
    {
       // Quick check based on extension.
      const size_t startExtension = filename.find_last_of('.');
      const size_t endExtension = filename.length();
      std::string extension = filename.substr(startExtension, endExtension - startExtension);
      boost::algorithm::to_lower(extension);
      boost::algorithm::trim(extension);
      return extension == expectedExtension;
    }
    
    /*
    Append the geometry and function information onto the outgoing vtkDataSet.
    @param visualDataSet : outgoing dataset on which to append metadata.
    @param wsName : name of the workspace.
    */
    void MDEWLoadingPresenter::appendMetadata(vtkDataSet* visualDataSet, const std::string& wsName)
    {
      using namespace Mantid::API;

      vtkFieldData* outputFD = vtkFieldData::New();
      
      //Serialize metadata
      RebinningKnowledgeSerializer serializer(LocationNotRequired);
      serializer.setWorkspaceName(wsName);
      serializer.setGeometryXML(xmlBuilder.create());
      serializer.setImplicitFunction( Mantid::Geometry::MDImplicitFunction_sptr(new Mantid::Geometry::NullImplicitFunction()));
      std::string xmlString = serializer.createXMLString();

      //Add metadata to dataset.
      MetadataToFieldData convert;
      convert(outputFD, xmlString, XMLDefinitions::metaDataId().c_str());
      visualDataSet->SetFieldData(outputFD);
      outputFD->Delete();
    }

    /**
     * Set the axis labels from the current dimensions
     * @param visualDataSet: The VTK dataset to update
     */
    void MDEWLoadingPresenter::setAxisLabels(vtkDataSet *visualDataSet)
    {
      vtkFieldData* fieldData = visualDataSet->GetFieldData();
      setAxisLabel("AxisTitleForX", axisLabels[0], fieldData);
      setAxisLabel("AxisTitleForY", axisLabels[1], fieldData);
      setAxisLabel("AxisTitleForZ", axisLabels[2], fieldData);
    }

    /**
    Gets the geometry in a string format.
    @return geometry string ref.
    @throw runtime_error if execute has not been run first.
    */
    const std::string& MDEWLoadingPresenter::getGeometryXML() const
    {
      if(!m_isSetup)
      {
        throw std::runtime_error("Have not yet run extractMetaData!");
      }
      return xmlBuilder.create();
    }

            /**
    @return boolean indicating whether the T dimension is available.
    @throw runtime_error if execute has not been run first.
    */
    bool MDEWLoadingPresenter::hasTDimensionAvailable() const
    {
      if(!m_isSetup)
      {
        throw std::runtime_error("Have not yet run ::extractMetaData!");
      }
      return xmlBuilder.hasTDimension() && !xmlBuilder.hasIntegratedTDimension();
    }

       /*
    @return timestep values.
    @throw runtime_error if execute has not been run first.
    */
    std::vector<double> MDEWLoadingPresenter::getTimeStepValues() const
    {
      if(!m_isSetup)
      {
        throw std::runtime_error("Have not yet run ::extractMetaData!");
      }
      std::vector<double> result;
      for(size_t i = 0; i < tDimension->getNBins(); i++)
      {
        result.push_back(tDimension->getX(i));
      }
      return result;
    }

    /**
     * Create a label for the "time" coordinate
     * @return the "time" coordinate label
     * @throw runtime_error if execute has not been run first.
     */
    std::string MDEWLoadingPresenter::getTimeStepLabel() const
    {
      if (!m_isSetup)
      {
        throw std::runtime_error("Have not yet run ::extractMetaData!");
      }
      return tDimension->getName() + " (" + tDimension->getUnits().ascii() + ")";
    }

    /**
     * Getter for the instrument.
     * @returns The name of the instrument which is associated with the workspace.
     */
    const std::string& MDEWLoadingPresenter::getInstrument()
    {
      return m_instrument;
    }
  }
}
