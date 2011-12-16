#include "MantidQtCustomInterfaces/WorkspaceInADS.h"
#include "MantidAPI/AnalysisDataService.h"

namespace MantidQt
{
  namespace CustomInterfaces
  {
    /*
    Workspace in ADS
    @param wsName : name of the workspace in the ADS
    */
    WorkspaceInADS::WorkspaceInADS(std::string wsName) : m_wsName(wsName)
    {
      using namespace Mantid::API;
      if(!checkStillThere())
      {
        throw std::runtime_error("WorkspaceInADS:: Workspace does not exist in the ADS: " + wsName);
      }
      
      MatrixWorkspace_sptr ws = boost::dynamic_pointer_cast<MatrixWorkspace>(AnalysisDataService::Instance().retrieve(m_wsName));
      if(!ws)
      {
        throw std::invalid_argument("WorkspaceInADS:: Workspace is not a matrix workspace : " + wsName );
      }
    }

    /**
    Getter for the id of the workspace
    @return the id of the workspace
    */
    std::string WorkspaceInADS::getId() const
    {
      return m_wsName;
    }

    /**
    Getter for the type of location where the workspace is stored
    @ return the location type
    */
    std::string WorkspaceInADS::locationType() const
    {
      return "In Memory";
    }

    /**
    Check that the workspace has not been deleted since instantiating this memento
    @return true if still in specified location
    */
    bool WorkspaceInADS::checkStillThere() const
    {
      return Mantid::API::AnalysisDataService::Instance().doesExist(m_wsName);
    }

    /**
    Getter for the workspace itself
    @returns the matrix workspace
    @throw if workspace has been moved since instantiation.
    */
    Mantid::API::MatrixWorkspace_sptr WorkspaceInADS::fetchIt() const
    {
      using namespace Mantid::API;
      if(!checkStillThere())
      {
        std::string msg("WorkspaceInADS is attempting to fetch a workspace out of the ADS that has been removed or renamed. Original name: " + m_wsName);
        throw std::runtime_error(msg);
      }
      return boost::dynamic_pointer_cast<MatrixWorkspace>(AnalysisDataService::Instance().retrieve(m_wsName));
    }

    /// Destructor
    WorkspaceInADS::~WorkspaceInADS()
    {
    }
  }
}