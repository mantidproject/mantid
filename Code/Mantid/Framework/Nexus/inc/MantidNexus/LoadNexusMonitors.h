#ifndef MANTID_NEXUS_LOADNEXUSMONITORS_H_
#define MANTID_NEXUS_LOADNEXUSMONITORS_H_

#include "MantidAPI/Algorithm.h"
#include "MantidAPI/MatrixWorkspace.h"

//Copy of the NexusCpp API was placed in MantidNexus
#include "MantidNexus/NeXusFile.hpp"
#include "MantidNexus/NeXusException.hpp"

namespace Mantid
{

  namespace NeXus
  {
  /** @class LoadNexusMonitors LoadNexusMonitors.h Nexus/LoadNexusMonitors.h

  Load Monitors from NeXus files.

  Required Properties:
  <UL>
  <LI> Filename - The name of and path to the input NEXUS file </LI>
  <LI> Workspace - The name of the workspace to output</LI>
  </UL>

  @author Michael Reuter, SNS
  @date October 25, 2010

  Copyright &copy; 2010 ISIS Rutherford Appleton Laboratory & NScD Oak Ridge National Laboratory

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

  File change history is stored at: <https://svn.mantidproject.org/mantid/trunk/Code/Mantid>
  */
  class DLLExport LoadNexusMonitors : public API::Algorithm
  {
  public:
    /// Default constructor
    LoadNexusMonitors();
    /// Destructor
    virtual ~LoadNexusMonitors();
    /// Algorithm's name for identification overriding a virtual method
    virtual const std::string name() const { return "LoadNexusMonitors"; };
    /// Algorithm's version for identification overriding a virtual method
    virtual int version() const { return 1; };
    /// Algorithm's category for identification overriding a virtual method
    virtual const std::string category() const { return "Nexus"; }

  private:
    /// Sets documentation strings for this algorithm
    virtual void initDocs();
    /// Intialisation code
    void init();
    /// Execution code
    void exec();
    /// Load the appropriate instrument
    void runLoadInstrument(const std::string& instrument,
        API::MatrixWorkspace_sptr localWorkspace);

    /// The name and path of the input file
    std::string filename;
    /// The workspace being filled out
    API::MatrixWorkspace_sptr WS;
    /// Number of monitors
    size_t nMonitors;
    /// Set to true when instrument geometry was loaded.
    bool instrument_loaded_correctly;
  };

  }
}
#endif /* MANTID_NEXUS_LOADNEXUSMONITORS_H_ */
