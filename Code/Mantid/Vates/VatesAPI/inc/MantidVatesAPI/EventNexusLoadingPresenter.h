#ifndef MANTID_VATES_EVENT_NEXUS_LOADING_PRESENTER
#define MANTID_VATES_EVENT_NEXUS_LOADING_PRESENTER

#include "MantidVatesAPI/MDEWLoadingPresenter.h"
#include "MantidVatesAPI/FilteringUpdateProgressAction.h"

#include "MantidNexus/NeXusFile.hpp"
#include "MantidNexus/NeXusException.hpp"
#include "MantidMDEvents/OneStepMDEW.h"

#include <vtkUnstructuredGrid.h>

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
    class DLLExport EventNexusLoadingPresenter : public MDEWLoadingPresenter<ViewType>
    {
    public:
      EventNexusLoadingPresenter(ViewType* view, const std::string fileName);
      virtual vtkDataSet* execute(vtkDataSetFactory* factory, ProgressAction& eventHandler);
      virtual void executeLoadMetadata();
      virtual bool hasTDimensionAvailable() const;
      virtual std::vector<double> getTimeStepValues() const;
      virtual ~EventNexusLoadingPresenter();
      virtual bool canReadFile() const;
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
    EventNexusLoadingPresenter<ViewType>::EventNexusLoadingPresenter(ViewType* view, const std::string filename) : MDEWLoadingPresenter<ViewType>(filename, view)
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

    /*
    Executes the underlying algorithm to create the MVP model.
    @param factory : visualisation factory to use.
    @param eventHandler : object that encapuslates the direction of the gui change as the algorithm progresses.
    */
    template<typename ViewType>
    vtkDataSet* EventNexusLoadingPresenter<ViewType>::execute(vtkDataSetFactory* factory, ProgressAction& eventHandler)
    {
      using namespace Mantid::API;
      using namespace Mantid::Geometry;

      m_view->getLoadInMemory(); //TODO, nexus reader algorithm currently has no use of this.
      
      if(shouldLoad())
      {
        Poco::NObserver<ProgressAction, Mantid::API::Algorithm::ProgressNotification> observer(eventHandler, &ProgressAction::handler);
        AnalysisDataService::Instance().remove("MD_EVENT_WS_ID");

        Mantid::MDEvents::OneStepMDEW alg;
        alg.initialize();
        alg.setRethrows(true);
        alg.setPropertyValue("Filename", this->m_filename);
        alg.setPropertyValue("OutputWorkspace", "MD_EVENT_WS_ID");
        alg.addObserver(observer);
        alg.execute();
        alg.removeObserver(observer);
      }

      Workspace_sptr result=AnalysisDataService::Instance().retrieve("MD_EVENT_WS_ID");
      Mantid::API::IMDEventWorkspace_sptr eventWs = boost::dynamic_pointer_cast<Mantid::API::IMDEventWorkspace>(result);

      factory->setRecursionDepth(m_view->getRecursionDepth());
      factory->initialize(eventWs);
      vtkDataSet* visualDataSet = factory->create();

      extractMetadata(eventWs);
      appendMetadata(visualDataSet, eventWs->getName());
      
      return visualDataSet;
    }

    /**
    @return boolean indicating whether the T dimension is available.
    @throw runtime_error if execute has not been run first.
    */
    template<typename ViewType>
    bool EventNexusLoadingPresenter<ViewType>::hasTDimensionAvailable() const
    {
      return false; //OneStepMDEW uses MakeDiffractionMDEventWorkspace, which always generates 3 dimensional MDEW
    }

       /*
    @return timestep values.
    @throw runtime_error if execute has not been run first.
    */
    template<typename ViewType>
    std::vector<double> EventNexusLoadingPresenter<ViewType>::getTimeStepValues() const
    {
      throw std::runtime_error("Does not have a 4th Dimension, so can be no T-axis");
    }

    ///Destructor
    template<typename ViewType>
    EventNexusLoadingPresenter<ViewType>::~EventNexusLoadingPresenter()
    {
    }

     /**
     Executes any meta-data loading required.
    */
    template<typename ViewType>
    void EventNexusLoadingPresenter<ViewType>::executeLoadMetadata()
    {
      /*Effectively a do-nothing implementation. 
      
      Do not have a metadataonly switch for the underlying algorithm, therfore would be costly to load metadata.
      For these file types we know that we get 3 dimensions anyway so do not need anyfurther geometry information until the point
      at which it must be added to the outgoing vtkdataset.
      */
      m_isSetup = true;
    }

  }
}

#endif
