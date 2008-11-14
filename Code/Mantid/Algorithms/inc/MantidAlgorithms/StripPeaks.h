#ifndef MANTID_ALGORITHMS_STRIPPEAKS_H_
#define MANTID_ALGORITHMS_STRIPPEAKS_H_

#include "MantidAPI/Algorithm.h"
#include "MantidDataObjects/TableWorkspace.h"

namespace Mantid
{
namespace Algorithms
{
/** This algorithm attempts to find all the peaks in all spectra of a workspace
    and subtract them from the data, leaving just the 'background'.

    *** AT PRESENT, AUTOMATIC PEAK FINDING IS MISSING SO A FEW VANADIUM PEAK
    POSITIONS ARE HARD-CODED IN. THIS MEANS, OF COURSE, THAT THE ALGORITHM,
    WHICH IS INTENDED TO BE GENERAL, CAN ONLY BE USED FOR VANADIUM DATA FILES
    (WHICH ARE IN UNITS OF D-SPACING). INDEED, IT MAY BE OVER-TUNED TO THE
    PARTICULAR DATA FILE USED WHEN WRITING IT!
    THERE ARE ALSO IMPROVEMENTS PENDING TO THE FITTING SUBROUTINE USED,
    WHICH SHOULD IMPROVE ACCURACY OF THE PEAK FITTING. ***

    Required Properties:
    <UL>
    <LI> InputWorkspace - The name of the Workspace to take as input. </LI>
    <LI> OutputWorkspace - The name under which to store the output workspace. </LI>
    </UL>

    @author Russell Taylor, Tessella Support Services plc
    @date 30/10/2008

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

    File change history is stored at: <https://svn.mantidproject.org/mantid/trunk/Code/Mantid>
    Code Documentation is available at: <http://doxygen.mantidproject.org>
*/
class DLLExport StripPeaks : public API::Algorithm
{
public:
  /// (Empty) Constructor
  StripPeaks();
  /// Virtual destructor
  virtual ~StripPeaks() {}
  /// Algorithm's name
  virtual const std::string name() const { return "StripPeaks"; }
  /// Algorithm's version
  virtual const int version() const { return (1); }
  /// Algorithm's category for identification
  virtual const std::string category() const { return "General"; }

private:
  /// Initialisation code
  void init();
  ///Execution code
  void exec();

  API::Workspace_sptr smoothInput(API::Workspace_sptr input);
  void findPeaks(API::Workspace_sptr WS);
  void fitPeaks(API::Workspace_sptr WS);
  API::Workspace_sptr removePeaks(API::Workspace_sptr input);

  /// Storage of the peak data
  DataObjects::TableWorkspace m_peaks;

  /// Static reference to the logger class
  static Kernel::Logger& g_log;
};

} // namespace Algorithms
} // namespace Mantid

#endif /*MANTID_ALGORITHMS_STRIPPEAKS_H_*/
