#ifndef MANTID_VATES_MDEW_EVENT_NEXUS_LOADING_PRESENTER
#define MANTID_VATES_MDEW_EVENT_NEXUS_LOADING_PRESENTER

#include <vtkUnstructuredGrid.h>
#include <vtkFieldData.h>

#include "MantidGeometry/MDGeometry/MDGeometryXMLBuilder.h"
#include "MantidAPI/IMDEventWorkspace.h"
#include "MantidAPI/ImplicitFunction.h"
#include "MantidMDAlgorithms/CompositeImplicitFunction.h"
#include "MantidVatesAPI/MDLoadingPresenter.h"
#include "MantidVatesAPI/FilteringUpdateProgressAction.h"
#include "MantidVatesAPI/vtkDataSetFactory.h"
#include "MantidVatesAPI/RebinningKnowledgeSerializer.h"
#include "MantidVatesAPI/MetadataToFieldData.h"
#include "MantidVatesAPI/RebinningCutterXMLDefinitions.h"
#include "MantidNexus/NeXusFile.hpp"
#include "MantidNexus/NeXusException.hpp"
#include "MantidMDEvents/LoadMDEW.h"

namespace Mantid
{
  namespace VATES
  {
    /** 
    @class MDEWEventNexusLoadingPresenter, Abstract presenters for loading conversion of MDEW workspaces into render-able vtk objects.
    @author Owen Arnold, Tessella plc
    @date 09/08/2011

    Copyright &copy; 2011 ISIS Rutherford Appleton Laboratory & NScD Oak Ridge National Laboratory

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
    class DLLExport MDEWEventNexusLoadingPresenter : public MDLoadingPresenter
    {
    public:
      MDEWEventNexusLoadingPresenter(ViewType* view, const std::string fileName);
      virtual vtkDataSet* execute(vtkDataSetFactory* factory, ProgressAction& eventHandler);
      virtual bool hasTDimensionAvailable() const;
      virtual std::vector<double> getTimeStepValues() const;
      std::string getGeometryXML() const;
      virtual ~MDEWEventNexusLoadingPresenter();
      virtual bool canReadFile() const;
    private:
      const std::string m_filename;
      ViewType* m_view;
      Mantid::Geometry::MDGeometryBuilderXML<Mantid::Geometry::StrictDimensionPolicy> xmlBuilder;
      Mantid::Geometry::IMDDimension_sptr tDimension;
      void appendMetadata(vtkDataSet* visualDataSet, Mantid::API::IMDEventWorkspace_sptr eventWs);
      bool m_isSetup;
    };

    /*
    Constructor
    @param view : MVP view
    @param filename : name of file to load
    @throw invalid_argument if file name is empty
    @throw invalid_arument if view is null
    @throw logic_error if cannot use the reader-presenter for this filetype.
    */
    template<typename ViewType>
    MDEWEventNexusLoadingPresenter<ViewType>::MDEWEventNexusLoadingPresenter(ViewType* view, const std::string filename) : MDLoadingPresenter(), m_filename(filename), m_view(view), m_isSetup(false)
    {
      if(m_filename.empty())
      {
        throw std::invalid_argument("File name is an empty string.");
      }
      if(NULL == m_view)
      {
        throw std::invalid_argument("View is NULL.");
      }
    }

     /*
    Indicates whether this presenter is capable of handling the type of file that is attempted to be loaded.
    @return false if the file cannot be read.
    */
    template<typename ViewType>
    bool MDEWEventNexusLoadingPresenter<ViewType>::canReadFile() const
    {
      ::NeXus::File * file = NULL;

      file = new ::NeXus::File(m_filename);
      // MDEventWorkspace file has a different name for the entry
      try
      {
        file->openGroup("MDEventWorkspace", "NXentry");
        file->close();
        return 1;
      }
      catch(::NeXus::Exception &)
      {
        // If the entry name does not match, then it can't read the file.
        file->close();
        return 0;
      }
      return 0;
    }

    /*
    Executes the underlying algorithm to create the MVP model.
    @param factory : visualisation factory to use.
    @param eventHandler : object that encapuslates the direction of the gui change as the algorithm progresses.
    */
    template<typename ViewType>
    vtkDataSet* MDEWEventNexusLoadingPresenter<ViewType>::execute(vtkDataSetFactory* factory, ProgressAction& eventHandler)
    {
      using namespace Mantid::API;
      using namespace Mantid::Geometry;

      Poco::NObserver<ProgressAction, Mantid::API::Algorithm::ProgressNotification> observer(eventHandler, &ProgressAction::handler);
      AnalysisDataService::Instance().remove("MD_EVENT_WS_ID");

      Mantid::MDEvents::LoadMDEW alg;
      alg.initialize();
      alg.setPropertyValue("Filename", this->m_filename);
      alg.setPropertyValue("OutputWorkspace", "MD_EVENT_WS_ID");
      alg.setProperty("FileBackEnd", !m_view->getLoadInMemory()); //Load from file by default.
      alg.execute();

      Workspace_sptr result=AnalysisDataService::Instance().retrieve("MD_EVENT_WS_ID");
      Mantid::API::IMDEventWorkspace_sptr eventWs = boost::dynamic_pointer_cast<Mantid::API::IMDEventWorkspace>(result);

      factory->setRecursionDepth(m_view->getRecursionDepth());
      factory->initialize(eventWs);
      vtkDataSet* visualDataSet = factory->create();

      appendMetadata(visualDataSet, eventWs);
      
      return visualDataSet;
    }

    /*
    Append the geometry and function information onto the outgoing vtkDataSet.
    @param visualDataSet : outgoing dataset on which to append metadata.
    @param eventWs : Event workspace from which metadata is drawn.
    */
    template<typename ViewType>
    void MDEWEventNexusLoadingPresenter<ViewType>::appendMetadata(vtkDataSet* visualDataSet, Mantid::API::IMDEventWorkspace_sptr eventWs)
    {
      using namespace Mantid::Geometry;
      using namespace Mantid::API;

      vtkFieldData* outputFD = vtkFieldData::New();
      
      //Serialize metadata
      RebinningKnowledgeSerializer serializer(LocationNotRequired);
      serializer.setWorkspaceName(eventWs->getName());

      std::vector<MDDimensionExtents> ext = eventWs->getMinimumExtents(5);
      std::vector<IMDDimension_sptr> dimensions;
      size_t nDimensions = eventWs->getNumDims();
      for (size_t d=0; d<nDimensions; d++)
      {
        IMDDimension_sptr inDim = eventWs->getDimension(d);
        double min = (ext[d].min);
        double max = (ext[d].max);
        if (min > max)
        {
          min = 0.0;
          max = 1.0;
        }
        //std::cout << "dim " << d << min << " to " <<  max << std::endl;
        MDHistoDimension_sptr dim(new MDHistoDimension(inDim->getName(), inDim->getName(), inDim->getUnits(), min, max, size_t(10)));
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

      serializer.setGeometryXML(xmlBuilder.create());
      serializer.setImplicitFunction( ImplicitFunction_sptr(new Mantid::MDAlgorithms::CompositeImplicitFunction()));
      std::string xmlString = serializer.createXMLString();

      //Add metadata to dataset.
      MetadataToFieldData convert;
      convert(outputFD, xmlString, XMLDefinitions::metaDataId().c_str());
      visualDataSet->SetFieldData(outputFD);
      outputFD->Delete();

      m_isSetup = true;
    }

    /**
    Gets the geometry in a string format.
    @return geometry string.
    @throw runtime_error if execute has not been run first.
    */
    template<typename ViewType>
    std::string MDEWEventNexusLoadingPresenter<ViewType>::getGeometryXML() const
    {
      if(!m_isSetup)
      {
        throw std::runtime_error("Have not yet run ::execute!");
      }
      return xmlBuilder.create();
    }

    /**
    @return boolean indicating whether the T dimension is available.
    @throw runtime_error if execute has not been run first.
    */
    template<typename ViewType>
    bool MDEWEventNexusLoadingPresenter<ViewType>::hasTDimensionAvailable() const
    {
      if(!m_isSetup)
      {
        throw std::runtime_error("Have not yet run ::execute!");
      }
      return xmlBuilder.hasTDimension();
    }

    /*
    @return timestep values.
    @throw runtime_error if execute has not been run first.
    */
    template<typename ViewType>
    std::vector<double> MDEWEventNexusLoadingPresenter<ViewType>::getTimeStepValues() const
    {
      if(!m_isSetup)
      {
        throw std::runtime_error("Have not yet run ::execute!");
      }
      std::vector<double> result;
      for(size_t i = 0; i < tDimension->getNBins(); i++)
      {
        result.push_back(tDimension->getX(i));
      }
      return result;
    }

    ///Destructor
    template<typename ViewType>
    MDEWEventNexusLoadingPresenter<ViewType>::~MDEWEventNexusLoadingPresenter()
    {
    }

  }
}

#endif
