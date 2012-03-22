#include "MantidQtCustomInterfaces/WorkspaceInADS.h"
#include "MantidAPI/AnalysisDataService.h"
#include "MantidAPI/MatrixWorkspace.h"
#include "MantidGeometry/Crystal/OrientedLattice.h"

using namespace Mantid::API;

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
      
      if(!checkStillThere())
      {
        throw std::runtime_error("WorkspaceInADS:: Workspace does not exist in the ADS: " + wsName);
      }
      
      MatrixWorkspace_sptr ws = AnalysisDataService::Instance().retrieveWS<MatrixWorkspace>(m_wsName);
      if(!ws)
      {
        throw std::invalid_argument("WorkspaceInADS:: Workspace is not a matrix workspace : " + wsName );
      }

      if(ws->mutableSample().hasOrientedLattice())
      {
        std::vector<double> ub = ws->mutableSample().getOrientedLattice().getUB().getVector();
        this->setUB(ub[0], ub[1], ub[2], ub[3], ub[4], ub[5], ub[6], ub[7], ub[8]);
      }
      cleanUp();
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
      return locType();
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
    @param protocol : fetch protocol
    @throw if workspace has been moved since instantiation.
    */
    Workspace_sptr WorkspaceInADS::fetchIt(FetchProtocol) const
    {
      if(!checkStillThere())
      {
        std::string msg("WorkspaceInADS is attempting to fetch a workspace out of the ADS that has been removed or renamed. Original name: " + m_wsName);
        throw std::runtime_error(msg);
      }
      return AnalysisDataService::Instance().retrieve(m_wsName);
    }

    /// Destructor
    WorkspaceInADS::~WorkspaceInADS()
    {
    }

    /*
    Apply actions. Load workspace and apply all actions to it.
    */
    Workspace_sptr WorkspaceInADS::applyActions()
    {
      //Do nothing because everything should already have been applied to the workspace in the ADS. Just return it for consistency.
      return AnalysisDataService::Instance().retrieve(m_wsName);
    }
  }
}