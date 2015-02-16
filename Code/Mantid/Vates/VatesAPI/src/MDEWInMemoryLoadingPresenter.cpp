#include "MantidVatesAPI/MDEWInMemoryLoadingPresenter.h"
#include "MantidVatesAPI/MDLoadingView.h"
#include "MantidVatesAPI/ProgressAction.h"
#include "MantidVatesAPI/vtkDataSetFactory.h"
#include "MantidVatesAPI/WorkspaceProvider.h"
#include "MantidGeometry/MDGeometry/MDGeometryXMLBuilder.h"
#include <qwt_double_interval.h>
#include <vtkUnstructuredGrid.h>

namespace Mantid
{
  namespace VATES
  {

    /*
    Constructor
    @param view : MVP view
    @param repository : Object for accessing the workspaces
    @param wsName : Name of the workspace to use.
    @throw invalid_argument if the workspace name is empty
    @throw invalid_argument if the repository is null
    @throw invalid_arument if view is null
    */
  MDEWInMemoryLoadingPresenter::MDEWInMemoryLoadingPresenter(MDLoadingView* view, WorkspaceProvider* repository, std::string wsName) : MDEWLoadingPresenter(view), 
    m_repository(repository), m_wsName(wsName), m_wsTypeName(""), m_specialCoords(-1)
    {
      if(m_wsName.empty())
      {
        throw std::invalid_argument("The workspace name is empty.");
      }
      if(NULL == repository)
      {
        throw std::invalid_argument("The repository is NULL");
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
    bool MDEWInMemoryLoadingPresenter::canReadFile() const
    {
      bool bCanReadIt = true;
      if(!m_repository->canProvideWorkspace(m_wsName))
      {
        //The workspace does not exist.
        bCanReadIt = false;
      }
      else if(NULL == boost::dynamic_pointer_cast<Mantid::API::IMDEventWorkspace>(m_repository->fetchWorkspace(m_wsName)).get())
      {
        //The workspace can be found, but is not an IMDEventWorkspace.
        bCanReadIt = false;
      }
      else
      {
        //The workspace is present, and is of the correct type.
        bCanReadIt = true;
      }
      return bCanReadIt;
    }

    /*
    Executes the underlying algorithm to create the MVP model.
    @param factory : visualisation factory to use.
    @param  : Handler for GUI updates while algorithm progresses.
    @param drawingProgressUpdate : Handler for GUI updates while vtkDataSetFactory::create occurs.
    */
    vtkDataSet* MDEWInMemoryLoadingPresenter::execute(vtkDataSetFactory* factory, ProgressAction&, ProgressAction& drawingProgressUpdate)
    {
      using namespace Mantid::API;
      using namespace Mantid::Geometry;

      Workspace_sptr ws = m_repository->fetchWorkspace(m_wsName);
      IMDEventWorkspace_sptr eventWs = boost::dynamic_pointer_cast<Mantid::API::IMDEventWorkspace>(ws);

      factory->setRecursionDepth(this->m_view->getRecursionDepth());
      vtkDataSet* visualDataSet = factory->oneStepCreate(eventWs, drawingProgressUpdate);
      
      /*extractMetaData needs to be re-run here because the first execution of this from ::executeLoadMetadata will not have ensured that all dimensions
        have proper range extents set.
      */

      // Update the meta data min and max values with the values of the visual data set. This is necessary since we want the full data range of the visual 
      // data set and not of the actual underlying data set.
      double* range  = visualDataSet->GetScalarRange();
      if (range)
      {
        this->m_metadataJsonManager->setMinValue(range[0]);
        this->m_metadataJsonManager->setMaxValue(range[1]);
      }

      this->extractMetadata(eventWs);

      this->appendMetadata(visualDataSet, eventWs->getName());
      return visualDataSet;
    }

    /**
     Executes any meta-data loading required.
    */
    void MDEWInMemoryLoadingPresenter::executeLoadMetadata()
    {
      using namespace Mantid::API;

      Workspace_sptr ws = m_repository->fetchWorkspace(m_wsName);
      IMDEventWorkspace_sptr eventWs = boost::dynamic_pointer_cast<Mantid::API::IMDEventWorkspace>(ws);
      m_wsTypeName = eventWs->id();
      m_specialCoords = eventWs->getSpecialCoordinateSystem();

      // Set the minimum and maximum of the workspace data.
      QwtDoubleInterval minMaxContainer = m_metaDataExtractor->getMinAndMax(eventWs);
      m_metadataJsonManager->setMinValue(minMaxContainer.minValue());
      m_metadataJsonManager->setMaxValue(minMaxContainer.maxValue());

      // Set the instrument which is associated with the workspace.
      m_metadataJsonManager->setInstrument(m_metaDataExtractor->extractInstrument(eventWs));

      //Call base-class extraction method.
      this->extractMetadata(eventWs);
    }

    ///Destructor
    MDEWInMemoryLoadingPresenter::~MDEWInMemoryLoadingPresenter()
    {
      delete m_view;
    }

     /*
      Getter for the workspace type name.
      @return Workspace Type Name
    */
    std::string MDEWInMemoryLoadingPresenter::getWorkspaceTypeName()
    {
      return m_wsTypeName;
    }

    /**
     * Getter for the special coordinates.
     * @return the special coordinates value
     */
    int MDEWInMemoryLoadingPresenter::getSpecialCoordinates()
    {
      return m_specialCoords;
    }
  }
}
