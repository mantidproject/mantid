#ifndef MANTID_DATAHANDLING_MARKDEADDETECTORS_H_
#define MANTID_DATAHANDLING_MARKDEADDETECTORS_H_

//----------------------------------------------------------------------
// Includes
//----------------------------------------------------------------------
#include "DataHandlingCommand.h"
#include "MantidDataObjects/Workspace2D.h"

namespace Mantid
{
namespace DataHandling
{
/** An algorithm to mark a detector, or set or range of detectors, as dead.
    The workspace spectra associated with those detectors are zeroed.

    Required Properties:
    <UL>
    <LI> Workspace - The name of the (input & output) Workspace2D on which to perform the algorithm </LI>
    </UL>

    Optional Properties (one or the other should be set or nothing will happen!):
    <UL>
    <LI> WorkspaceIndexList - An ArrayProperty containing a list of workspace indices whose detectors should be marked as dead </LI>
    <LI> WorkspaceIndexMin  - The lower bound of a range of workspace indices whose detectors should be marked as dead </LI>
    <LI> WorkspaceIndexMax  - The upper bound of a range of workspace indices whose detectors should be marked as dead </LI>
    </UL>

    @author Russell Taylor, Tessella Support Services plc
    @date 15/04/2008

    Copyright &copy; 2008 STFC Rutherford Appleton Laboratory

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
class DLLExport MarkDeadDetectors : public DataHandlingCommand
{
public:
  MarkDeadDetectors();
  virtual ~MarkDeadDetectors();
	
  /// Algorithm's name for identification overriding a virtual method
  virtual const std::string name() const { return "MarkDeadDetectors";};
  /// Algorithm's version for identification overriding a virtual method
  virtual const int version() const { return 1;};
  /// Algorithm's category for identification overriding a virtual method
  virtual const std::string category() const { return "DataHandling\\Dectectors";}

private:
  // Implement abstract Algorithm methods
  void init();
  void exec();
  
  /// Clears the spectrum data
  void clearSpectrum(const int& index, const int& vectorSize);
  
  /// Pointer to the local workspace
  DataObjects::Workspace2D_sptr m_localWorkspace;  
  
  /// Static reference to the logger class
  static Kernel::Logger& g_log;
};

} // namespace DataHandling
} // namespace Mantid

#endif /*MANTID_DATAHANDLING_MARKDEADDETECTORS_H_*/
