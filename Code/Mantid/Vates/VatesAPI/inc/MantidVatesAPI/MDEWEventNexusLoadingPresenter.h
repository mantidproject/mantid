#ifndef MANTID_VATES_MDEW_EVENT_NEXUS_LOADING_PRESENTER
#define MANTID_VATES_MDEW_EVENT_NEXUS_LOADING_PRESENTER

#include <vtkUnstructuredGrid.h>

#include "MantidGeometry/MDGeometry/MDGeometryXMLBuilder.h"
#include "MantidVatesAPI/MDEWLoadingPresenter.h"
#include "MantidVatesAPI/ProgressAction.h"
#include "MantidVatesAPI/vtkDataSetFactory.h"

#include "MantidNexusCPP/NeXusFile.hpp"
#include "MantidNexusCPP/NeXusException.hpp"
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
    class DLLExport MDEWEventNexusLoadingPresenter : public MDEWLoadingPresenter<ViewType>
    {
    public:
      MDEWEventNexusLoadingPresenter(ViewType* view, const std::string fileName);
      virtual vtkDataSet* execute(vtkDataSetFactory* factory, ProgressAction& eventHandler);
      virtual void executeLoadMetadata();
      virtual ~MDEWEventNexusLoadingPresenter();
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
    MDEWEventNexusLoadingPresenter<ViewType>::MDEWEventNexusLoadingPresenter(ViewType* view, const std::string filename) : MDEWLoadingPresenter<ViewType>(filename, view)
    {
      if(this->m_filename.empty())
      {
        throw std::invalid_argument("File name is an empty string.");
      }
      if(NULL == this->m_view)
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

      file = new ::NeXus::File(this->m_filename);
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

      if(this->shouldLoad())
      {
        Poco::NObserver<ProgressAction, Mantid::API::Algorithm::ProgressNotification> observer(eventHandler, &ProgressAction::handler);
        AnalysisDataService::Instance().remove("MD_EVENT_WS_ID");

        Mantid::MDEvents::LoadMDEW alg;
        alg.initialize();
        alg.setPropertyValue("Filename", this->m_filename);
        alg.setPropertyValue("OutputWorkspace", "MD_EVENT_WS_ID");
        alg.setProperty("FileBackEnd", !this->m_view->getLoadInMemory()); //Load from file by default.
        alg.execute();
      }

      Workspace_sptr result=AnalysisDataService::Instance().retrieve("MD_EVENT_WS_ID");
      Mantid::API::IMDEventWorkspace_sptr eventWs = boost::dynamic_pointer_cast<Mantid::API::IMDEventWorkspace>(result);

      factory->setRecursionDepth(this->m_view->getRecursionDepth());
      factory->initialize(eventWs);
      vtkDataSet* visualDataSet = factory->create();
      
      /*extractMetaData needs to be re-run here because the first execution of this from ::executeLoadMetadata will not have ensured that all dimensions
        have proper range extents set.
      */
      this->extractMetadata(eventWs);

      this->appendMetadata(visualDataSet, eventWs->getName());
      return visualDataSet;
    }

    /**
     Executes any meta-data loading required.
    */
    template<typename ViewType>
    void MDEWEventNexusLoadingPresenter<ViewType>::executeLoadMetadata()
    {
      using namespace Mantid::API;
      AnalysisDataService::Instance().remove("MD_EVENT_WS_ID");

      Mantid::MDEvents::LoadMDEW alg;
      alg.initialize();
      alg.setPropertyValue("Filename", this->m_filename);
      alg.setPropertyValue("OutputWorkspace", "MD_EVENT_WS_ID");
      alg.setProperty("MetadataOnly", true); //Don't load the events.
      alg.setProperty("FileBackEnd", false); //Only require metadata, so do it in memory.
      alg.execute();

      Workspace_sptr result=AnalysisDataService::Instance().retrieve("MD_EVENT_WS_ID");
      IMDEventWorkspace_sptr eventWs = boost::dynamic_pointer_cast<Mantid::API::IMDEventWorkspace>(result);

      //Call base-class extraction method.
      this->extractMetadata(eventWs);

      AnalysisDataService::Instance().remove("MD_EVENT_WS_ID");
    }

    ///Destructor
    template<typename ViewType>
    MDEWEventNexusLoadingPresenter<ViewType>::~MDEWEventNexusLoadingPresenter()
    {
    }

  }
}

#endif
