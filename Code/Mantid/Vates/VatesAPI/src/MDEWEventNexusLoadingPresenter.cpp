#include "MantidVatesAPI/MDEWEventNexusLoadingPresenter.h"
#include "MantidVatesAPI/MDLoadingView.h"
#include "MantidVatesAPI/ProgressAction.h"
#include "MantidVatesAPI/vtkDataSetFactory.h"

#include "MantidGeometry/MDGeometry/MDGeometryXMLBuilder.h"

#include <nexus/NeXusFile.hpp>
#include <nexus/NeXusException.hpp>
#include "MantidAPI/AlgorithmManager.h"
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
    MDEWEventNexusLoadingPresenter::MDEWEventNexusLoadingPresenter(MDLoadingView* view, const std::string filename) : MDEWLoadingPresenter(view), m_filename(filename), m_wsTypeName("")
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
    bool MDEWEventNexusLoadingPresenter::canReadFile() const
    {
      // Quick check based on extension.
      if(!canLoadFileBasedOnExtension(m_filename, ".nxs"))
      {
        return 0;
      }

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
    @param loadingProgressUpdate : Handler for GUI updates while algorithm progresses.
    @param drawingProgressUpdate : Handler for GUI updates while vtkDataSetFactory::create occurs.
    */
    vtkDataSet* MDEWEventNexusLoadingPresenter::execute(vtkDataSetFactory* factory, ProgressAction& loadingProgressUpdate, ProgressAction& drawingProgressUpdate)
    {
      using namespace Mantid::API;
      using namespace Mantid::Geometry;

      if(this->shouldLoad())
      {
        Poco::NObserver<ProgressAction, Mantid::API::Algorithm::ProgressNotification> observer(loadingProgressUpdate, &ProgressAction::handler);
        AnalysisDataService::Instance().remove("MD_EVENT_WS_ID");

        IAlgorithm_sptr alg = AlgorithmManager::Instance().create("LoadMD");
        alg->initialize();
        alg->setPropertyValue("Filename", this->m_filename);
        alg->setPropertyValue("OutputWorkspace", "MD_EVENT_WS_ID");
        alg->setProperty("FileBackEnd", !this->m_view->getLoadInMemory()); //Load from file by default.
        alg->addObserver(observer);
        alg->execute();
        alg->removeObserver(observer);
      }

      Workspace_sptr result=AnalysisDataService::Instance().retrieve("MD_EVENT_WS_ID");
      Mantid::API::IMDEventWorkspace_sptr eventWs = boost::dynamic_pointer_cast<Mantid::API::IMDEventWorkspace>(result);

      factory->setRecursionDepth(this->m_view->getRecursionDepth());
      //Create visualisation in one-shot.
      vtkDataSet* visualDataSet = factory->oneStepCreate(eventWs, drawingProgressUpdate);
      
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
    void MDEWEventNexusLoadingPresenter::executeLoadMetadata()
    {
      using namespace Mantid::API;
      AnalysisDataService::Instance().remove("MD_EVENT_WS_ID");

      IAlgorithm_sptr alg = AlgorithmManager::Instance().create("LoadMD");
       
      alg->initialize();
      alg->setPropertyValue("Filename", this->m_filename);
      alg->setPropertyValue("OutputWorkspace", "MD_EVENT_WS_ID");
      alg->setProperty("MetadataOnly", true); //Don't load the events.
      alg->setProperty("FileBackEnd", false); //Only require metadata, so do it in memory.
      alg->execute();

      Workspace_sptr result=AnalysisDataService::Instance().retrieve("MD_EVENT_WS_ID");
      IMDEventWorkspace_sptr eventWs = boost::dynamic_pointer_cast<Mantid::API::IMDEventWorkspace>(result);
      m_wsTypeName = eventWs->id();
      //Call base-class extraction method.
      this->extractMetadata(eventWs);

      AnalysisDataService::Instance().remove("MD_EVENT_WS_ID");
    }

    ///Destructor
    MDEWEventNexusLoadingPresenter::~MDEWEventNexusLoadingPresenter()
    {
      delete m_view;
    }

        /*
    Getter for the workspace type name.
    @return Workspace Type Name
    */
    std::string MDEWEventNexusLoadingPresenter::getWorkspaceTypeName()
    {
      return m_wsTypeName;
    }
  }
}
