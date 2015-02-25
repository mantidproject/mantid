#ifndef MANTID_DATAHANDLING_LOADMUONNEXUS1_H_
#define MANTID_DATAHANDLING_LOADMUONNEXUS1_H_

//----------------------------------------------------------------------
// Includes
//----------------------------------------------------------------------
#include "MantidDataHandling/LoadMuonNexus.h"
#include "MantidDataObjects/TableWorkspace.h"
#include "MantidDataObjects/Workspace2D.h"
#include "MantidKernel/System.h"

// Forward declaration (here, because is not in Mantid namespace)
class MuonNexusReader;

namespace Mantid {
//----------------------------------------------------------------------
// Forward declaration
//----------------------------------------------------------------------
namespace NeXus {
class NXRoot;
}

namespace DataHandling {
/** @class LoadMuonNexus LoadMuonNexus.h DataHandling/LoadMuonNexus.h

Loads an file in Nexus Muon format version 1 and stores it in a 2D workspace
(Workspace2D class). LoadMuonNexus is an algorithm and as such inherits
from the Algorithm class, via DataHandlingCommand, and overrides
the init() & exec() methods.

Required Properties:
<UL>
<LI> Filename - The name of and path to the input NeXus file </LI>
<LI> OutputWorkspace - The name of the workspace in which to store the imported
data
     (a multiperiod file will store higher periods in workspaces called
OutputWorkspace_PeriodNo)
     [ not yet implemented for Nexus ]</LI>
</UL>

Optional Properties: (note that these options are not available if reading a
multiperiod file)
<UL>
<LI> spectrum_min  - The spectrum to start loading from</LI>
<LI> spectrum_max  - The spectrum to load to</LI>
<LI> spectrum_list - An ArrayProperty of spectra to load</LI>
<LI> auto_group - Determines whether the spectra are automatically grouped
together based on the groupings in the NeXus file. </LI>
</UL>

Copyright &copy; 2007-2010 ISIS Rutherford Appleton Laboratory, NScD Oak Ridge
National Laboratory & European Spallation Source

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
class DLLExport LoadMuonNexus1 : public LoadMuonNexus {
public:
  /// Default constructor
  LoadMuonNexus1();
  /// Destructor
  virtual ~LoadMuonNexus1() {}
  /// Algorithm's name for identification overriding a virtual method
  virtual const std::string name() const { return "LoadMuonNexus"; }
  /// Summary of algorithms purpose
  virtual const std::string summary() const {
    return "The LoadMuonNexus algorithm will read the given NeXus Muon data "
           "file Version 1 and use the results to populate the named "
           "workspace. LoadMuonNexus may be invoked by LoadNexus if it is "
           "given a NeXus file of this type.";
  }

  /// Algorithm's version for identification overriding a virtual method
  virtual int version() const { return 1; }
  /// Algorithm's category for identification overriding a virtual method
  virtual const std::string category() const {
    return "DataHandling\\Nexus;Muon";
  }

  /// Returns a confidence value that this algorithm can load a file
  virtual int confidence(Kernel::NexusDescriptor &descriptor) const;

protected:
  /// Overwrites Algorithm method
  void exec();

private:
  void loadData(size_t hist, specid_t &i, specid_t specNo, 
                MuonNexusReader &nxload, const int64_t lengthIn,
                DataObjects::Workspace2D_sptr localWorkspace);
  void runLoadMappingTable(DataObjects::Workspace2D_sptr);
  void runLoadLog(DataObjects::Workspace2D_sptr);
  void loadRunDetails(DataObjects::Workspace2D_sptr localWorkspace);

  /// Loads dead time table for the detector
  void loadDeadTimes(Mantid::NeXus::NXRoot &root);

  /// Creates Dead Time Table using all the data between begin and end
  DataObjects::TableWorkspace_sptr
  createDeadTimeTable(std::vector<double>::const_iterator begin,
                      std::vector<double>::const_iterator end);

  /// Loads detector grouping information
  API::Workspace_sptr loadDetectorGrouping(Mantid::NeXus::NXRoot &root);

  /// Creates Detector Grouping Table using all the data from the range
  DataObjects::TableWorkspace_sptr
  createDetectorGroupingTable(std::vector<int>::const_iterator begin,
                              std::vector<int>::const_iterator end);
};

} // namespace DataHandling
} // namespace Mantid

#endif /*MANTID_DATAHANDLING_LOADMUONNEXUS1_H_*/
