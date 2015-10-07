#ifndef MANTID_DATAHANDLING_LOADMUONNEXUS2_H_
#define MANTID_DATAHANDLING_LOADMUONNEXUS2_H_

//----------------------------------------------------------------------
// Includes
//----------------------------------------------------------------------
#include "MantidDataHandling/LoadMuonNexus.h"
#include "MantidNexus/NexusClasses.h"

namespace Mantid {

namespace DataHandling {

/**
Loads an file in NeXus Muon format version 1 and 2 and stores it in a 2D
workspace
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
[ not yet implemented for NeXus ]</LI>
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
class DLLExport LoadMuonNexus2 : public LoadMuonNexus {
public:
  /// Default constructor
  LoadMuonNexus2();
  /// Destructor
  ~LoadMuonNexus2() {}
  /// Algorithm's name for identification overriding a virtual method
  virtual const std::string name() const { return "LoadMuonNexus"; }
  /// Summary of algorithms purpose
  virtual const std::string summary() const {
    return "The LoadMuonNexus algorithm will read the given NeXus Muon data "
           "file Version 2 and use the results to populate the named "
           "workspace. LoadMuonNexus may be invoked by LoadNexus if it is "
           "given a NeXus file of this type.";
  }

  /// Algorithm's version for identification overriding a virtual method
  virtual int version() const { return 2; }
  /// Algorithm's category for identification overriding a virtual method
  virtual const std::string category() const {
    return "DataHandling\\Nexus;Muon";
  }

  /// Returns a confidence value that this algorithm can load a file
  virtual int confidence(Kernel::NexusDescriptor &descriptor) const;

private:
  /// Overwrites Algorithm method
  void exec();
  /// Execute this version of the algorithm
  void doExec();

  void loadData(const Mantid::NeXus::NXInt &counts,
                const std::vector<double> &timeBins, int wsIndex, int period,
                int spec, API::MatrixWorkspace_sptr localWorkspace);
  void loadLogs(API::MatrixWorkspace_sptr ws, Mantid::NeXus::NXEntry &entry,
                int period);
  void loadRunDetails(DataObjects::Workspace2D_sptr localWorkspace);
};

} // namespace DataHandling
} // namespace Mantid

#endif /*MANTID_DATAHANDLING_LOADMUONNEXUS2_H_*/
