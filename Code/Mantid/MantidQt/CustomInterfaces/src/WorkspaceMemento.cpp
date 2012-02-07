#include "MantidQtCustomInterfaces/WorkspaceMemento.h"

namespace MantidQt
{
  namespace CustomInterfaces
  {
    typedef WorkspaceMemento::Status Status;

    /// Constructor
    WorkspaceMemento::WorkspaceMemento() : m_ub(0)
    {
    }

    /**
    Getter for the status report.
    @return friendly report status.
    */
    std::string WorkspaceMemento::statusReport() const
    {
      const Status status = generateStatus();
      return interpretStatus(status);
    }

    /**
    Common implementation of status generation.
    @return status.
    */
    Status WorkspaceMemento::generateStatus() const
    {
      Status status;
      if(m_ub.empty())
      {
        status = NoOrientedLattice;
      }
      else
      {
        status = Ready;
      }
      return status;
    }

    /*
    Helper method for extracting friendly messages from status enum. 
    Single point for providing messages.
    */
    std::string WorkspaceMemento::interpretStatus(const Status status) const
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
      return msg;
    }

    /*
    Setter for the ub matrix. 9 elements required. ubXY where X is row index and Y is column index.
    @param ub00 : ub element
    @param ub01 : ub element
    @param ub02 : ub element
    @param ub10 : ub element
    @param ub11 : ub element
    @param ub12 : ub element
    @param ub20 : ub element
    @param ub21 : ub element
    @param ub22 : ub element
    */
    void WorkspaceMemento::setUB(const double& ub00, const double&  ub01, const double&  ub02, const double&  ub10, const double&  ub11, const double&  ub12, const double&  ub20, const double&  ub21, const double&  ub22)
    {
      std::vector<double> temp(9);
      m_ub.swap(temp);
      m_ub[0] = ub00;
      m_ub[1] = ub01;
      m_ub[2] = ub02;
      m_ub[3] = ub10;
      m_ub[4] = ub11;
      m_ub[5] = ub12;
      m_ub[6] = ub20;
      m_ub[7] = ub21;
      m_ub[8] = ub22;
    }

    /**
    Setter for goniometer axis.
    @param axis0 : axis 0 settings
    @param axis1 : axis 1 settings
    @param axis2 : axis 2 settings 
    @param axis3 : axis 3 settings 
    @param axis4 : axis 4 settings 
    @param axis5 : axis 5 settings 
    */
    void WorkspaceMemento::setGoniometer(const std::string axis0, const std::string axis1, const std::string axis2, const std::string axis3, const std::string axis4, const std::string axis5)
    {
      std::vector<std::string> temp(6);
      m_axes.swap(temp);
      m_axes[0] = axis0;
      m_axes[1] = axis1;
      m_axes[2] = axis2;
      m_axes[3] = axis3;
      m_axes[4] = axis4;
      m_axes[5] = axis5;
    }

    /**
    Setter for log values
    @param name : Name of the log
    @param value : Value of  the log
    @param logType : Log type to use
    */
    void WorkspaceMemento::setLogValue(const std::string name, const std::string value, const std::string logType)
    {
      LogEntry entry;
      entry.name = name;
      entry.value = value;
      entry.type = logType;
      this->m_logEntries.push_back(entry);
    }

    /*
    Getter for the ubmatrix
    @return vector of elements. indexed by row then by column. i.e. First 3 entries are from the first row and span all columns.
    */
    std::vector<double> WorkspaceMemento::getUB() const
    {
      return m_ub;
    }

  }
}