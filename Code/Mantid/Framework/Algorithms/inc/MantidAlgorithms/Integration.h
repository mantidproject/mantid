#ifndef MANTID_ALGORITHMS_INTEGRATION_H_
#define MANTID_ALGORITHMS_INTEGRATION_H_
/*WIKI* 

Integration sums up spectra in a [[Workspace]] and outputs a [[Workspace]] that contains only 1 value per spectrum (i.e. the sum). The associated errors are added in quadrature.
The two X values per spectrum are set to the limits of the range over which the spectrum has been integrated. By default, the entire range is integrated and all spectra are included.

=== Optional properties ===
If only a portion of the workspace should be integrated then the optional parameters may be used to restrict the range. StartWorkspaceIndex & EndWorkspaceIndex may be used to select a contiguous range of spectra in the workspace (note that these parameters refer to the workspace index value rather than spectrum numbers as taken from the raw file).
If only a certain range of each spectrum should be summed (which must be the same for all spectra being integrated) then the Range_lower and Range_upper properties should be used. No rebinning takes place as part of this algorithm: if the values given do not coincide with a bin boundary then the first bin boundary within the range is used. If a value is given that is beyond the limit covered by the spectrum then it will be integrated up to its limit.
The data that falls outside any values set will not contribute to the output workspace.

=== EventWorkspaces ===
If an [[EventWorkspace]] is used as the input, the output will be a [[MatrixWorkspace]]. [[Rebin]] is recommended if you want to keep the workspace as an EventWorkspace.


*WIKI*/

//----------------------------------------------------------------------
// Includes
//----------------------------------------------------------------------
#include "MantidAPI/Algorithm.h"
#include "MantidDataObjects/EventWorkspace.h"

namespace Mantid
{
namespace Algorithms
{
/** Takes a workspace as input and sums each spectrum contained within
    it, storing the result as a workspace of spectra with one Y & E value
    and two X values indicating the range which the integration covers.

    Required Properties:
    <UL>
    <LI> InputWorkspace - The name of the workspace to take as input. Must be a histogram. </LI>
    <LI> OutputWorkspace - The name of the workspace in which to store the result </LI>
    </UL>

    Optional Properties (assume that you count from zero):
    <UL>
    <LI> Range_lower - The X value to integrate from (default 0)</LI>
    <LI> Range_upper - The X value to integrate to (default max)</LI>
    <LI> StartWorkspaceIndex - Workspace index number to integrate from (default 0)</LI>
    <LI> EndWorkspaceIndex - Workspace index number to integrate to (default max)</LI>
    <LI> IncludePartialBins - If true then partial bins from the beginning and end of the input range are also included in the integration (default false)</LI>
    </UL>

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
class DLLExport Integration : public API::Algorithm
{
public:
  /// Default constructor
  Integration() : API::Algorithm() {};
  /// Destructor
  virtual ~Integration() {};
  /// Algorithm's name for identification overriding a virtual method
  virtual const std::string name() const { return "Integration";}
  /// Algorithm's version for identification overriding a virtual method
  virtual int version() const { return (1);}
  /// Algorithm's category for identification overriding a virtual method
  virtual const std::string category() const { return "General";}

private:
  /// Sets documentation strings for this algorithm
  virtual void initDocs();
  // Overridden Algorithm methods
  void init();
  void exec();

  /// Input event workspace
  DataObjects::EventWorkspace_const_sptr inputEventWS;

  /// The value in X to start the integration from
  double m_MinRange;
  /// The value in X to finish the integration at
  double m_MaxRange;
  /// The spectrum to start the integration from
  int m_MinSpec;
  /// The spectrum to finish the integration at
  int m_MaxSpec;

};

} // namespace Algorithm
} // namespace Mantid

#endif /*MANTID_ALGORITHMS_INTEGRATION_H_*/
