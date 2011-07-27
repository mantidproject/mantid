#ifndef MANTID_VATES_MDEW_EVENT_NEXUS_REBINNING_PRESENTER 
#define MANTID_VATES_MDEW_EVENT_NEXUS_REBINNING_PRESENTER

#include "MantidVatesAPI/MDEWLoadingRebinningPresenter.h"
#include "MantidMDEvents/OneStepMDEW.h"
#include "MantidNexus/NeXusException.hpp"

namespace Mantid
{
  namespace VATES
  {
    /**
 @class  MDEWEventNexusPresenter
 @brief Presenter for loading and rebinning specific for Event Nexus files.
 @author Owen Arnold
 @date July 28th 2011
 @version 1.0

 Can identify Event Nexus files from other nexus file types and uses a specific data loading algorithm to extract EventWorkspaces from such files. Base class handles 
 other presenter responsibilities specific to MDEW type rebinning.

 Copyright &copy; 2007 ISIS Rutherford Appleton Laboratory & NScD Oak Ridge National Laboratory

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

 */
    template<typename ViewType>
    class DLLExport MDEWEventNexusPresenter : public MDEWLoadingRebinningPresenter<ViewType>
    {
    public:
      MDEWEventNexusPresenter(std::string fileName, RebinningActionManager* request, ViewType* view);
      virtual ~MDEWEventNexusPresenter();
      virtual bool canLoadFile() const;
      virtual void executeLoad(ProgressAction& eventHandler);
    };

    /** Constructor
    * @param filename : File to load.
    * @param request : Request managing object
    * @param view : MVP view
    */
    template<typename ViewType>
    MDEWEventNexusPresenter<ViewType>::MDEWEventNexusPresenter(std::string fileName, RebinningActionManager* request, ViewType* view) : 
    MDEWLoadingRebinningPresenter<ViewType>(fileName, request, view)
    {
    }

    /// Destructor
    template<typename ViewType>
    MDEWEventNexusPresenter<ViewType>::~MDEWEventNexusPresenter()
    {
    }

    /** Determine whether this presenter is suitable for the file provided.
    * @return true if suitable.
    */
    template<typename ViewType>
    bool MDEWEventNexusPresenter<ViewType>::canLoadFile() const
    {
      ::NeXus::File * file = NULL;
      try
      {
        file = new ::NeXus::File(this->m_filename);
        // All SNS (event or histo) nxs files have an entry named "entry"
        try
        {
          file->openGroup("entry", "NXentry");
        }
        catch(::NeXus::Exception&)
        {
          file->close();
          return false;
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
        return hasEvents;
      }
      catch (std::exception & e)
      {
        std::cerr << "Could not open " << this->m_filename << " as an EventNexus file because of exception: " << e.what() << std::endl;
        // Clean up, if possible
        if (file)
          file->close();
      }
      return false;
    }

    /** Determine whether this presenter is suitable for the file provided.
    * @param eventHandler : EventHandler for algorithmic progress updating.
    */
    template<typename ViewType>
    void MDEWEventNexusPresenter<ViewType>::executeLoad(ProgressAction& eventHandler)
    {
      using namespace Mantid::API;
      using namespace Mantid::Geometry;

      AnalysisDataService::Instance().remove(this->m_mdEventWsId);

      Mantid::MDEvents::OneStepMDEW alg;
      alg.initialize();
      alg.setPropertyValue("Filename", this->m_filename);
      alg.setPropertyValue("OutputWorkspace", this->m_mdEventWsId);

      Poco::NObserver<ProgressAction, Mantid::API::Algorithm::ProgressNotification> observer(eventHandler, &ProgressAction::handler);
      //Add observer.
      alg.addObserver(observer);
      //Run loading algorithm.
      alg.execute();
      //Remove observer.
      alg.removeObserver(observer);

    Workspace_sptr result=AnalysisDataService::Instance().retrieve(this->m_mdEventWsId);
    Mantid::API::IMDEventWorkspace_sptr eventWs = boost::dynamic_pointer_cast<Mantid::API::IMDEventWorkspace>(result);
    
    // Now, we get the minimum extents in order to get nice default sizes
    std::vector<Mantid::Geometry::MDDimensionExtents> ext = eventWs->getMinimumExtents(5);
    std::vector<IMDDimension_sptr> defaultDimensions;
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
      defaultDimensions.push_back(dim);
    }
    
    //Clear out any old values by reassigning a new instance.
    this->m_geometryXmlBuilder = MDGeometryBuilderXML<StrictDimensionPolicy>();

    //Configuring the geometry xml builder allows the object panel associated with this reader to later
    //determine how to display all geometry related properties.
    if(nDimensions > 0)
    {
      this->m_geometryXmlBuilder.addXDimension( defaultDimensions[0] );
    }
    if(nDimensions > 1)
    {
      this->m_geometryXmlBuilder.addYDimension( defaultDimensions[1] );
    }
    if(nDimensions > 2)
    {
      this->m_geometryXmlBuilder.addZDimension( defaultDimensions[2] );
    }
    if(nDimensions > 3)
    {
      this->m_geometryXmlBuilder.addTDimension( defaultDimensions[3] );
    }
    if(nDimensions > 4)
    {
      for(size_t i = 4; i < nDimensions; i++)
      {
        this->m_geometryXmlBuilder.addOrdinaryDimension( defaultDimensions[i] );
      }
    }
    //Now have a record of the input geometry.
    this->m_serializer.setGeometryXML(this->m_geometryXmlBuilder.create());

    }
  }
}

#endif