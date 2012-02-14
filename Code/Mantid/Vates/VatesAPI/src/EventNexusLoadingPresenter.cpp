#include "MantidVatesAPI/EventNexusLoadingPresenter.h"
#include "MantidVatesAPI/MDLoadingView.h"
#include "MantidGeometry/MDGeometry/MDGeometryXMLBuilder.h"

#include "MantidVatesAPI/ProgressAction.h"
#include "MantidVatesAPI/vtkDataSetFactory.h"

#include "MantidNexusCPP/NeXusFile.hpp"
#include "MantidNexusCPP/NeXusException.hpp"
#include "MantidMDEvents/OneStepMDEW.h"

#include <vtkUnstructuredGrid.h>


namespace Mantid
{
  namespace VATES
  {
    /*
    Constructor
    @param view : MVP view
    @param filename : name of file to load
    @throw invalid_argument if file name is empty
    @throw invalid_arument if view is null
    @throw logic_error if cannot use the reader-presenter for this filetype.
    */
    EventNexusLoadingPresenter::EventNexusLoadingPresenter(MDLoadingView* view, const std::string filename) : MDEWLoadingPresenter(view), m_filename(filename), m_wsTypeName("")
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
    bool EventNexusLoadingPresenter::canReadFile() const
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
        catch(::NeXus::Exception &)
        {
          file->close();
          return 0;
        }
        // But only eventNexus files have bank123_events as a group name
        std::map<std::string, std::string> entries = file->getEntries();
        bool hasEvents = false;
        std::map<std::string, std::string>::iterator it;
        for (it = entries.begin(); it != entries.end(); ++it)
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
    vtkDataSet* EventNexusLoadingPresenter::execute(vtkDataSetFactory* factory, ProgressAction& eventHandler)
    {
      using namespace Mantid::API;
      using namespace Mantid::Geometry;

      this->m_view->getLoadInMemory(); //TODO, nexus reader algorithm currently has no use of this.

      if(this->shouldLoad())
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
      m_wsTypeName = eventWs->id();

      factory->setRecursionDepth(this->m_view->getRecursionDepth());
      vtkDataSet* visualDataSet = factory->oneStepCreate(eventWs);

      this->extractMetadata(eventWs);
      this->appendMetadata(visualDataSet, eventWs->getName());

      return visualDataSet;
    }

    /**
    @return boolean indicating whether the T dimension is available.
    @throw runtime_error if execute has not been run first.
    */
    bool EventNexusLoadingPresenter::hasTDimensionAvailable() const
    {
      return false; //OneStepMDEW uses ConvertToDiffractionMDWorkspace, which always generates 3 dimensional MDEW
    }

    /*
    @return timestep values.
    @throw runtime_error if execute has not been run first.
    */
    std::vector<double> EventNexusLoadingPresenter::getTimeStepValues() const
    {
      throw std::runtime_error("Does not have a 4th Dimension, so can be no T-axis");
    }

    ///Destructor
    EventNexusLoadingPresenter::~EventNexusLoadingPresenter()
    {
      delete m_view;
    }

    /**
    Executes any meta-data loading required.
    */
    void EventNexusLoadingPresenter::executeLoadMetadata()
    {
      /*Effectively a do-nothing implementation. 

      Do not have a metadataonly switch for the underlying algorithm, therfore would be costly to load metadata.
      For these file types we know that we get 3 dimensions anyway so do not need anyfurther geometry information until the point
      at which it must be added to the outgoing vtkdataset.
      */
      this->m_isSetup = true;
    }

    /*
    Getter for the workspace type name.
    @return Workspace Type Name
    */
    std::string EventNexusLoadingPresenter::getWorkspaceTypeName()
    {
      return m_wsTypeName;
    }
  }
}