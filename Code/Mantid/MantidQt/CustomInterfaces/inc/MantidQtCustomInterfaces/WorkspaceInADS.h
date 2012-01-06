#ifndef MANTID_CUSTOMINTERFACES_WORKSPACE_IN_ADS_H_
#define MANTID_CUSTOMINTERFACES_WORKSPACE_IN_ADS_H_

#include "MantidQtCustomInterfaces/WorkspaceMemento.h"

namespace MantidQt
{
  namespace CustomInterfaces
  {
    /** @class WorkspaceInADS
    A workspace memento refering to a workspace in the Analysis Data Service.

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
    class DLLExport WorkspaceInADS : public WorkspaceMemento
    {
    public:
      /// Constructor
      WorkspaceInADS(std::string wsName);
      /**
      Getter for the id of the workspace
      @return the id of the workspace
      */
      virtual std::string getId() const;
      /**
      Getter for the type of location where the workspace is stored
      @ return the location type
      */
      virtual std::string locationType() const;
      /**
      Check that the workspace has not been deleted since instantiating this memento
      @return true if still in specified location
      */
      virtual bool checkStillThere() const;
      /**
      Getter for the workspace itself
      @returns the workspace
      @param protocol : fetch protocol
      @throw if workspace has been moved since instantiation.
      */
      virtual Mantid::API::Workspace_sptr fetchIt(FetchProtocol protocol) const;

      /*
      Do nothing clean-up method.
      */
      virtual void cleanUp()
      {
      }

      /*
      Location type associated with this type.
      @return string describing location
      */
      static std::string locType()
      {
        return "In Memory";
      }

      /// Destructor
      virtual ~WorkspaceInADS();

      //Apply actions wrapped up in this memento.
      virtual Mantid::API::Workspace_sptr applyActions();

    private:
      /// Id/name of the workspace in the ADS
      std::string m_wsName;
    };

  }
}
#endif
