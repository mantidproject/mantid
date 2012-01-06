#ifndef MANTID_CUSTOMINTERFACES_MEMENTO_H_
#define MANTID_CUSTOMINTERFACES_MEMENTO_H_

#include "MantidKernel/System.h"
#include "MantidKernel/Matrix.h"
#include "MantidAPI/Workspace.h"
#include <string>
#include <vector>
#include <boost/shared_ptr.hpp>

namespace MantidQt
{
  namespace CustomInterfaces
  {
    /** @class WorkspaceMemento

    A memento carrying basic information about an existing workspace. 
    Mementos introduce transaction like behaviour because changes to mementos are not automatically persisted to workspaces and can occur independently.

    @author Owen Arnold
    @date 30/08/2011

    Copyright &copy; 2007-8 ISIS Rutherford Appleton Laboratory & NScD Oak Ridge National Laboratory

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

    File change history is stored at: <https://svn.mantidproject.org/mantid/trunk/Code/Mantid>.
    Code Documentation is available at: <http://doxygen.mantidproject.org>
*/

    class DLLExport WorkspaceMemento
    {
    public:
      //Status enumeration type.
      enum Status{NoOrientedLattice=0, Ready};

      /// Constructor for the workspace memento.
      WorkspaceMemento();
      /**
      Getter for the id of the workspace
      @return the id of the workspace
      */
      virtual std::string getId() const = 0;
      /**
      Getter for the type of location where the workspace is stored
      @ return the location type
      */
      virtual std::string locationType() const = 0;
      /**
      Check that the workspace has not been deleted since instantiating this memento
      @return true if still in specified location
      */
      virtual bool checkStillThere() const = 0;
      /**
      Getter for the workspace itself
      @returns the matrix workspace
      @throw if workspace has been moved since instantiation.
      */
      virtual Mantid::API::Workspace_sptr  fetchIt() const = 0;
      /// Generates a status report based on the workspace state.
      std::string statusReport() const;
      /// Perform any clean up operations of the underlying workspace
      virtual void cleanUp() = 0;
      /// Sets a ub matrix element by element.
      void setUB(const double& ub00, const double&  ub01, const double&  ub02, const double&  ub10, const double&  ub11, const double&  ub12, const double&  ub20, const double&  ub21, const double&  ub22);
      /// Getter for a ub matrix.
      std::vector<double> getUB() const;
      /// Sets the goniometer matrix
      void setGoniometer(const Mantid::Kernel::DblMatrix& matrix); 
      /// Getter for the goniometer matrix
      Mantid::Kernel::DblMatrix getGoniometer() const;
      /// Destructor
      virtual ~WorkspaceMemento(){};
      /// Common implementation for generating status
      Status generateStatus() const;
      /// Apply actions wrapped up in the memento back to the original workspace
      virtual Mantid::API::Workspace_sptr applyActions() = 0;
      
    protected:

      // Vector of elements describing a UB matrix.
      std::vector<double> m_ub;

    private:

      /// Extract a friendly status.
      std::string interpretStatus(const Status arg) const;

      

      // Goniometer matrix
      Mantid::Kernel::DblMatrix m_goniometer;

    };

    /// WorkspaceMemento shared_ptr
    typedef boost::shared_ptr<WorkspaceMemento> WorkspaceMemento_sptr;
    /// Collection of WorkspaceMementos.
    typedef std::vector<WorkspaceMemento_sptr> WorkspaceMementoCollection;
  }
}

#endif