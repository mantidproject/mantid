#include "MantidQtCustomInterfaces/WorkspaceMemento.h"

namespace MantidQt
{
  namespace CustomInterfaces
  {
    /// Constructor
    WorkspaceMemento::WorkspaceMemento()
    {
    }

    /**
    Common implementation of report generation.
    @param ws : workspace to report on.
    */
    void WorkspaceMemento::generateReport(Mantid::API::MatrixWorkspace_sptr ws)
    {
      Status status;
      if(!ws->sample().hasOrientedLattice())
      {
        status = NoOrientedLattice;
      }
      else
      {
        status = Ready;
      }
      interpretStatus(status);
    }
    
    /*
    Setter for the status report.
    */
    void WorkspaceMemento::setReport(const Status status)
    {
      interpretStatus(status);
    }

    /*
    Helper method for extracting friendly messages from status enum. 
    Single point for providing messages.
    */
    void WorkspaceMemento::interpretStatus(const Status status)
    {
      std::string msg;
      if(status == NoOrientedLattice)
      {
        msg = "Has no Oriented Lattice";
      }
      else
      {
        msg = "Ready!";
      }
      m_statusReport = msg;
    }

  }
}