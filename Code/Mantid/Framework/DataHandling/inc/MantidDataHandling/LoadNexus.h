#ifndef MANTID_DATAHANDLING_LOADNEXUS_H_
#define MANTID_DATAHANDLING_LOADNEXUS_H_

//----------------------------------------------------------------------
// Includes
//----------------------------------------------------------------------
#include "MantidAPI/Algorithm.h"
#include "MantidAPI/Workspace.h"
#include "MantidKernel/Property.h"
#include <climits>
#include "MantidAPI/WorkspaceGroup.h"

namespace Mantid {
namespace DataHandling {
/** @class LoadNexus

Loads a file in NeXus format and stores it in a 2D workspace
(Workspace2D class). LoadNexus is an algorithm and as such inherits
from the Algorithm class, via DataHandlingCommand, and overrides
the init() & exec() methods.

Required Properties:
<UL>
<LI> Filename - The name of and path to the input NeXus file </LI>
<LI> OutputWorkspace - The name of the workspace in which to store the imported
data </LI>
    </UL>
    Optional Properties:
    <UL>
    <LI> spectrum_list - integer list of spectra numbers to load</LI>
    <LI> spectrum_min, spectrum_max - range of spectra to load</LI>
</UL>

@author Ronald Fowler, based on version by Freddie Akeroyd
@date 29/08/2008

Copyright &copy; 2007-8 ISIS Rutherford Appleton Laboratory, NScD Oak Ridge
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
class DLLExport LoadNexus : public API::Algorithm {
public:
  /// Default constructor
  LoadNexus();

  /// Destructor
  ~LoadNexus() {}
  /// Algorithm's name for identification overriding a virtual method
  virtual const std::string name() const { return "LoadNexus"; };
  /// Summary of algorithms purpose
  virtual const std::string summary() const {
    return "The LoadNexus algorithm will try to identify the type of Nexus "
           "file given to it and invoke the appropriate algorithm to read the "
           "data and populate the named workspace.";
  }

  /// Algorithm's version for identification overriding a virtual method
  virtual int version() const { return 1; };
  /// Algorithm's category for identification overriding a virtual method
  virtual const std::string category() const { return "DataHandling\\Nexus"; }

private:
  /// Overwrites Algorithm method.
  void init();

  /// Overwrites Algorithm method
  void exec();

  /// The name and path of the input file
  std::string m_filename;

  /// The name of the output workspace
  std::string m_workspace;

  /// run LoadMuonNexus
  void runLoadMuonNexus();

  /// run LoadIsisNexus
  void runLoadIsisNexus();

  /// run LoadNexusProcessed
  void runLoadNexusProcessed();

  /// run LoadTOFRawNexus
  void runLoadTOFRawNexus();

  /// set the output workspaces from the child algorithms
  void setOutputWorkspace(const API::IAlgorithm_sptr &loader);
};
} // namespace DataHandling
} // namespace Mantid

#endif /*MANTID_DATAHANDLING_LOADNEXUS_H_*/
