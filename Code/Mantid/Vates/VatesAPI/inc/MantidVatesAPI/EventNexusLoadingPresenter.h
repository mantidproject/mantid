#ifndef MANTID_VATES_EVENT_NEXUS_LOADING_PRESENTER
#define MANTID_VATES_EVENT_NEXUS_LOADING_PRESENTER

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
#include "MantidMDEvents/OneStepMDEW.h"

namespace Mantid
{
  namespace VATES
  {
    /** 
    @class EventNexusLoadingPresenter, Abstract presenters for loading conversion of MDEW workspaces into render-able vtk objects.
    @author Owen Arnold, Tessella plc
    @date 05/08/2011

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
    class DLLExport EventNexusLoadingPresenter : public MDLoadingPresenter
    {
    public:
      EventNexusLoadingPresenter(ViewType* view, const std::string fileName);
      virtual vtkDataSet* execute(vtkDataSetFactory* factory, ProgressAction& eventHandler);
      virtual bool hasTDimensionAvailable() const;
      virtual std::vector<double> getTimeStepValues() const;
      std::string getGeometryXML() const;
      virtual ~EventNexusLoadingPresenter();
    private:
      bool canReadFile() const;
      const std::string m_filename;
      ViewType* m_view;
      Mantid::Geometry::MDGeometryBuilderXML<Mantid::Geometry::StrictDimensionPolicy> xmlBuilder;
      Mantid::Geometry::IMDDimension_sptr tDimension;
      void appendMetadata(vtkDataSet* visualDataSet, Mantid::API::IMDEventWorkspace_sptr eventWs);
      bool m_isSetup;
    };

    template<typename ViewType>
    EventNexusLoadingPresenter<ViewType>::EventNexusLoadingPresenter(ViewType* view, const std::string filename) : MDLoadingPresenter(), m_filename(filename), m_view(view), m_isSetup(false)
    {
      if(m_filename.empty())
      {
        throw std::invalid_argument("File name is an empty string.");
      }
      if(NULL == m_view)
      {
        throw std::invalid_argument("View is NULL.");
      }
      if(!this->canReadFile())
      {
        throw std::logic_error("Cannot use this reader-presenter for this filetype");
      }
    }

    template<typename ViewType>
    bool EventNexusLoadingPresenter<ViewType>::canReadFile() const
    {
      ::NeXus::File * file = NULL;
      try
      {
        file = new ::NeXus::File(m_filename);
        // All SNS (event or histo) nxs files have an entry named "entry"
        try
        {
          file->openGroup("entry", "NXentry");
        }
        catch(::NeXus::Exception &)
        {
          file->close();
          return 0;
        }
        // But only eventNexus files have bank123_events as a group name
        std::map<std::string, std::string> entries = file->getEntries();
        bool hasEvents = false;
        std::map<std::string, std::string>::iterator it;
        for (it = entries.begin(); it != entries.end(); it++)
        {
          if (it->first.find("_events") != std::string::npos)
          {
            hasEvents=true;
            break;
          }
        }
        file->close();
        return hasEvents ? 1 : 0;
      }
      catch (std::exception & e)
      {
        std::cerr << "Could not open " << this->m_filename << " as an EventNexus file because of exception: " << e.what() << std::endl;
        // Clean up, if possible
        if (file)
          file->close();
      }
      return 0;
    }

    template<typename ViewType>
    vtkDataSet* EventNexusLoadingPresenter<ViewType>::execute(vtkDataSetFactory* factory, ProgressAction& eventHandler)
    {
      using namespace Mantid::API;
      using namespace Mantid::Geometry;

      m_view->getLoadInMemory(); //TODO, nexus reader algorithm currently has no use of this.
      
      Poco::NObserver<ProgressAction, Mantid::API::Algorithm::ProgressNotification> observer(eventHandler, &ProgressAction::handler);

      AnalysisDataService::Instance().remove("MD_EVENT_WS_ID");

      Mantid::MDEvents::OneStepMDEW alg;
      alg.initialize();
      alg.setPropertyValue("Filename", this->m_filename);
      alg.setPropertyValue("OutputWorkspace", "MD_EVENT_WS_ID");
      alg.addObserver(observer);
      alg.execute();
      alg.removeObserver(observer);

      Workspace_sptr result=AnalysisDataService::Instance().retrieve("MD_EVENT_WS_ID");
      Mantid::API::IMDEventWorkspace_sptr eventWs = boost::dynamic_pointer_cast<Mantid::API::IMDEventWorkspace>(result);

      factory->setRecursionDepth(m_view->getRecursionDepth());
      factory->initialize(eventWs);
      vtkDataSet* visualDataSet = factory->create();

      appendMetadata(visualDataSet, eventWs);
      
      return visualDataSet;
    }

    template<typename ViewType>
    void EventNexusLoadingPresenter<ViewType>::appendMetadata(vtkDataSet* visualDataSet, Mantid::API::IMDEventWorkspace_sptr eventWs)
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

    template<typename ViewType>
    std::string EventNexusLoadingPresenter<ViewType>::getGeometryXML() const
    {
      if(!m_isSetup)
      {
        throw std::runtime_error("Have not yet run ::execute!");
      }
      return xmlBuilder.create();
    }

    template<typename ViewType>
    bool EventNexusLoadingPresenter<ViewType>::hasTDimensionAvailable() const
    {
      if(!m_isSetup)
      {
        throw std::runtime_error("Have not yet run ::execute!");
      }
      return xmlBuilder.hasTDimension();
    }

    template<typename ViewType>
    std::vector<double> EventNexusLoadingPresenter<ViewType>::getTimeStepValues() const
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

    template<typename ViewType>
    EventNexusLoadingPresenter<ViewType>::~EventNexusLoadingPresenter()
    {
    }

  }
}

#endif
