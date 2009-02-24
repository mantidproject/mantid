#ifndef MANTID_ALGORITHMS_FINDPEAKS_H_
#define MANTID_ALGORITHMS_FINDPEAKS_H_

//----------------------------------------------------------------------
// Includes
//----------------------------------------------------------------------
#include "MantidAPI/Algorithm.h"
#include "MantidDataObjects/TableWorkspace.h"

namespace Mantid
{
namespace Algorithms
{
/** This algorithm searches for peaks in a dataset.
    The method used is detailed in: M.A.Mariscotti, NIM 50 (1967) 309.

    Required Properties:
    <UL>
    <LI> InputWorkspace - The name of the Workspace to search for peaks. </LI>
    <LI> PeaksList      - The name of the TableWorkspace in which to store the list of peaks found. </LI>
    </UL>

    Optional Properties:
    <UL>
    <LI> fwhm - The number of points covered on average by the fwhm of a peak (default 7) </LI>
    <LI> Tolerance - Sets the strictness desired in meeting the conditions on peak candidates (default 4, Mariscotti recommended 2) </LI>
    </UL>
    
    @author Russell Taylor, Tessella Support Services plc
    @date 25/11/2008

    Copyright &copy; 2008-9 STFC Rutherford Appleton Laboratory

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
class DLLExport FindPeaks : public API::Algorithm
{
public:
  /// Constructor
  FindPeaks();
  /// Virtual destructor
  virtual ~FindPeaks() {}
  /// Algorithm's name
  virtual const std::string name() const { return "FindPeaks"; }
  /// Algorithm's version
  virtual const int version() const { return (1); }
  /// Algorithm's category for identification
  virtual const std::string category() const { return "General"; }

private:
  /// Initialisation code
  void init();
  /// Execution code
  void exec();

  API::MatrixWorkspace_sptr calculateSecondDifference(const API::MatrixWorkspace_const_sptr &input);
  void smoothData(API::MatrixWorkspace_sptr &WS, const int &w);
  void calculateStandardDeviation(const API::MatrixWorkspace_const_sptr &input, const API::MatrixWorkspace_sptr &smoothed, const int &w);

  void fitPeak(const API::MatrixWorkspace_sptr &input, const int spectrum, const int i0, const int i4);

  /// Strip peaks method - living here until I can pass TableWorkspace as property
  API::MatrixWorkspace_sptr removePeaks(const API::MatrixWorkspace_const_sptr &input);

  /// The number of smoothing iterations
  static int g_z;
  
  /// Storage of the peak data
  boost::shared_ptr<DataObjects::TableWorkspace> m_peaks;

  /// Static reference to the logger class
  static Kernel::Logger& g_log;
};

} // namespace Algorithms
} // namespace Mantid

#endif /*MANTID_ALGORITHMS_FINDPEAKS_H_*/
