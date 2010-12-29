#ifndef MANTID_ALGORITHMS_SUMSPECTRA_H_
#define MANTID_ALGORITHMS_SUMSPECTRA_H_

//----------------------------------------------------------------------
// Includes
//----------------------------------------------------------------------
#include "MantidAPI/Algorithm.h"
#include "MantidDataObjects/EventWorkspace.h"

namespace Mantid
{
namespace Algorithms
{
/** Takes a workspace as input and sums all of the spectra within it maintaining the existing bin structure and units.
    The result is stored as a new workspace containing a single spectra.

    Required Properties:
    <UL>
    <LI> InputWorkspace - The name of the Workspace2D to take as input </LI>
    <LI> OutputWorkspace - The name of the workspace in which to store the result </LI>
    </UL>

    Optional Properties (assume that you count from zero):
    <UL>
    <LI> StartSpectrum - Workspace index number to integrate from (default 0)</LI>
    <LI> EndSpectrum - Workspace index number to integrate to (default max)</LI>
    <LI> IncludeMonitors - Whether to include monitor spectra in the sum (default yes)
    </UL>

    @author Nick Draper, Tessella Support Services plc
    @date 22/01/2009

    Copyright &copy; 2007-2010 ISIS Rutherford Appleton Laboratory & NScD Oak Ridge National Laboratory

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
    Code Documentation is available at: <http://doxygen.mantidproject.org>
 */
class DLLExport SumSpectra : public API::Algorithm
{
public:
  /// Default constructor
  SumSpectra() : API::Algorithm() {};
  /// Destructor
  virtual ~SumSpectra() {};
  /// Algorithm's name for identification overriding a virtual method
  virtual const std::string name() const { return "SumSpectra";}
  /// Algorithm's version for identification overriding a virtual method
  virtual int version() const { return (1);}
  /// Algorithm's category for identification overriding a virtual method
  virtual const std::string category() const { return "General";}

private:
  // Overridden Algorithm methods
  void init();
  void exec();
  void execEvent(DataObjects::EventWorkspace_const_sptr localworkspace, std::set<int> &indices);

  /// The spectrum to start the integration from
  int m_MinSpec;
  /// The spectrum to finish the integration at
  int m_MaxSpec;
  /// Set true to keep monitors
  bool keepMonitors;
  /// numberOfSpectra in the input
  int numberOfSpectra;
};

} // namespace Algorithm
} // namespace Mantid

#endif /*MANTID_ALGORITHMS_SUMSPECTRA_H_*/
