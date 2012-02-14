#include "MantidVatesAPI/SQWLoadingPresenter.h"
#include "MantidVatesAPI/MDLoadingView.h"
#include "MantidVatesAPI/ProgressAction.h"
#include "MantidVatesAPI/vtkDataSetFactory.h"

#include "MantidMDEvents/LoadSQW.h"
#include <boost/regex.hpp>

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
    SQWLoadingPresenter::SQWLoadingPresenter(MDLoadingView* view, const std::string filename) : MDEWLoadingPresenter(view), m_filename(filename), m_wsTypeName("")
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
    bool SQWLoadingPresenter::canReadFile() const
    {
      boost::regex expression(".*sqw$", boost::regex_constants::icase); //check that the file ends with sqw.
      return boost::regex_match(this->m_filename, expression);
    }


    /*
    Executes the underlying algorithm to create the MVP model.
    @param factory : visualisation factory to use.
    @param eventHandler : object that encapuslates the direction of the gui change as the algorithm progresses.
    */
    vtkDataSet* SQWLoadingPresenter::execute(vtkDataSetFactory* factory, ProgressAction& eventHandler)
    {
      using namespace Mantid::API;
      using namespace Mantid::Geometry;

      if(this->shouldLoad())
      {
        Poco::NObserver<ProgressAction, Mantid::API::Algorithm::ProgressNotification> observer(eventHandler, &ProgressAction::handler);
        AnalysisDataService::Instance().remove("MD_EVENT_WS_ID");
        
        

        Mantid::MDEvents::LoadSQW alg;
        alg.initialize();
        alg.setPropertyValue("Filename", this->m_filename);
        alg.setPropertyValue("OutputWorkspace", "MD_EVENT_WS_ID");
        //Default is not to load into memory and when this is the case, generate a nxs backend for output.
        if(!this->m_view->getLoadInMemory())
        {
          size_t pos = this->m_filename.find(".");
          std::string backEndFile = this->m_filename.substr(0, pos) + ".nxs";
          alg.setPropertyValue("OutputFilename", backEndFile);
        }
        alg.addObserver(observer);
        alg.execute();
        alg.removeObserver(observer);
      }

      Workspace_sptr result=AnalysisDataService::Instance().retrieve("MD_EVENT_WS_ID");
      Mantid::API::IMDEventWorkspace_sptr eventWs = boost::dynamic_pointer_cast<Mantid::API::IMDEventWorkspace>(result);

      factory->setRecursionDepth(this->m_view->getRecursionDepth());
      
      vtkDataSet* visualDataSet = factory->oneStepCreate(eventWs);

      this->appendMetadata(visualDataSet, eventWs->getName());
      
      return visualDataSet;
    }

     /*
    Extract the geometry and function information 

    This implementation is an override of the base-class method, which deals with the more common event based route. However the SQW files will provide complete
    dimensions with ranges already set. Less work needs to be done here than for event workspaces where the extents of each dimension need to be individually
    extracted.

    @param eventWs : event workspace to get the information from.
    */
    void SQWLoadingPresenter::extractMetadata(Mantid::API::IMDEventWorkspace_sptr eventWs)
    {
      using namespace Mantid::Geometry;
      MDGeometryBuilderXML<NoDimensionPolicy> refresh;
      this->xmlBuilder= refresh; //Reassign.
      std::vector<MDDimensionExtents> ext = eventWs->getMinimumExtents(5);
      std::vector<IMDDimension_sptr> dimensions;
      size_t nDimensions = eventWs->getNumDims();
      for (size_t d=0; d<nDimensions; d++)
      {
        IMDDimension_const_sptr inDim = eventWs->getDimension(d);
        //Copy the dimension, but set the ID and name to be the same. This is an assumption in bintohistoworkspace.
        MDHistoDimension_sptr dim(new MDHistoDimension(inDim->getName(), inDim->getName(), inDim->getUnits(), inDim->getMinimum(), inDim->getMaximum(), size_t(10)));
        dimensions.push_back(dim);
      }

      //Configuring the geometry xml builder allows the object panel associated with this reader to later
      //determine how to display all geometry related properties.
      if(nDimensions > 0)
      {
        this->xmlBuilder.addXDimension( dimensions[0] );
      }
      if(nDimensions > 1)
      {
        this->xmlBuilder.addYDimension( dimensions[1] );
      }
      if(nDimensions > 2)
      {
        this->xmlBuilder.addZDimension( dimensions[2]  );
      }
      if(nDimensions > 3)
      {
        this->tDimension = dimensions[3];
        this->xmlBuilder.addTDimension(this->tDimension);
      }
      this->m_isSetup = true;
    }
    

    /**
     Executes any meta-data loading required.
    */
    void SQWLoadingPresenter::executeLoadMetadata()
    {
      using namespace Mantid::API;
      using namespace Mantid::Geometry;

      AnalysisDataService::Instance().remove("MD_EVENT_WS_ID");

      Mantid::MDEvents::LoadSQW alg;
      alg.initialize();
      alg.setPropertyValue("Filename", this->m_filename);
      alg.setProperty("MetadataOnly", true); //Don't load the events.
      alg.setPropertyValue("OutputWorkspace", "MD_EVENT_WS_ID");
      alg.execute();

      Workspace_sptr result=AnalysisDataService::Instance().retrieve("MD_EVENT_WS_ID");
      Mantid::API::IMDEventWorkspace_sptr eventWs = boost::dynamic_pointer_cast<Mantid::API::IMDEventWorkspace>(result);
      m_wsTypeName = eventWs->id();
      //Call base-class extraction method.
      extractMetadata(eventWs);
    }

    ///Destructor
    SQWLoadingPresenter::~SQWLoadingPresenter()
    {
      delete m_view;
    }

    /*
    Getter for the workspace type name.
    @return Workspace Type Name
    */
    std::string SQWLoadingPresenter::getWorkspaceTypeName()
    {
      return m_wsTypeName;
    }

    }
    }
