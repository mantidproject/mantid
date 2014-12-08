#ifndef MANTID_DATAHANDLING_LoadCanSAS1D_H
#define MANTID_DATAHANDLING_LoadCanSAS1D_H

//----------------------------------------------------------------------
// Includes
//----------------------------------------------------------------------
#include "MantidAPI/IFileLoader.h"
#include "MantidDataObjects/Workspace2D.h"
#include <Poco/DOM/Element.h>
#include <Poco/DOM/Node.h>
//----------------------------------------------------------------------

namespace Poco {
  namespace XML {
    class Element;
  }
}

namespace Mantid
{
  namespace DataHandling
  {
    /** @class LoadCanSAS1D  DataHandling/LoadCanSAS1D.h

    This algorithm loads 1 CanSAS1d xml file into a workspace.

    Required properties:
    <UL>
    <LI> OutputWorkspace - The name of workspace to be created.</LI>
    <LI> Filename - Name of the file to load</LI>
    </UL>
       
    @author Sofia Antony, Rutherford Appleton Laboratory
    @date 26/01/2010

    Copyright &copy; 2007-10 ISIS Rutherford Appleton Laboratory, NScD Oak Ridge National Laboratory & European Spallation Source

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

    File change history is stored at: <https://github.com/mantidproject/mantid>.
    Code Documentation is available at: <http://doxygen.mantidproject.org>
*/
    class DLLExport LoadCanSAS1D : public API::IFileLoader<Kernel::FileDescriptor>
    {
    public:
      ///default constructor
      LoadCanSAS1D();
      /// destructor
      virtual ~LoadCanSAS1D();
      /// Algorithm's name for identification overriding a virtual method
      virtual const std::string name() const { return "LoadCanSAS1D"; }
    ///Summary of algorithms purpose
    virtual const std::string summary() const {return "Load a file written in the canSAS 1-D data format";}

      /// Algorithm's version for identification overriding a virtual method
      virtual int version() const { return 1; }
      /// Algorithm's category for identification overriding a virtual method
      virtual const std::string category() const { return "DataHandling\\XML"; }

      /// Returns a confidence value that this algorithm can load a file
      virtual int confidence(Kernel::FileDescriptor & descriptor) const;

    protected:
      
      /// If a workspace group is created this is set from empty to the root name of the members, the name of the workspace group members up to and including the _
      std::string m_groupMembersBase;
      /// When a workspace group is being written this is the number of the last member that was written
      int m_groupNumber;

      /// Overwrites Algorithm method.
      virtual void init();
      /// Overwrites Algorithm method
      void exec();

      /// Loads an individual SASentry element into a new workspace
      virtual API::MatrixWorkspace_sptr loadEntry(Poco::XML::Node * const workspaceData, std::string & runName);
      /// Checks if the pointer to the loaded data is not null or throws if it is
      void check(const Poco::XML::Element* const toCheck, const std::string & name) const;
      /// Appends the new data workspace creating a workspace group if there was existing data
      void appendDataToOutput(API::MatrixWorkspace_sptr newWork, const std::string & newWorkName, API::WorkspaceGroup_sptr container);
      /// Run LoadInstrument Child Algorithm
      void runLoadInstrument(const std::string & inst_name, API::MatrixWorkspace_sptr localWorkspace);
      /// Loads data into the run log
      void createLogs(const Poco::XML::Element * const sasEntry, API::MatrixWorkspace_sptr wSpace) const;
    };
    
  }
}

#endif
