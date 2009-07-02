#ifndef MANTID_ALGORITHMS_FINDPEAKS1D_H_
#define MANTID_ALGORITHMS_FINDPEAKS1D_H_

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
    The method used is based on the method detailed in: M.A.Mariscotti, NIM 50 (1967) 309.

    Required Properties:
    <UL>
    <LI> InputWorkspace - The name of the Workspace to search for peaks. </LI>
    <LI> PeaksList      - The name of the TableWorkspace in which to store the list of peaks found. </LI>
    <LI> spectrum       - The spectrum number in the InputWorkspace.</LI>
    <LI> smoothNpts     - The number of points for averaging, i.e. summing will be done in the range [y(i-m),y(i+m)] when
                          calculating the second difference </LI>
    <LI> smoothIter     - The number of iteration iterations in the averaging procedure </LI>
    <LI> threashold     - The threahsold value for peak detection, i.e only points with Intensity/sigma>threashold
	                        will be considered as peaks </LI>
    </UL>


    @author Laurent Chapon, ISIS, Rutherford Appleton Laboratory
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
class DLLExport FindPeaks1D : public API::Algorithm
{
public:
  /// Constructor
  FindPeaks1D();
  /// Virtual destructor
  virtual ~FindPeaks1D() {}
  /// Algorithm's name
  virtual const std::string name() const { return "FindPeaks1D"; }
  /// Algorithm's version
  virtual const int version() const { return (1); }
  /// Algorithm's category for identification
  virtual const std::string category() const { return "General"; }

private:
  /// Initialisation code
  void init();
  /// Execution code
  void exec();
  /// Sub-algorithm for calculating Smoothed second difference
  void generalisedSecondDifference();
  /// Reads in the values passed to this algorithm and set any default values
  void retrieveProperties();
  /// Performs the peak searching
  void analyseVector();
  /// Static reference to the logger class
  static Kernel::Logger& g_log;
  // Parameters
  API::MatrixWorkspace_sptr input;            ///< The input workspace
  API::MatrixWorkspace_sptr second_diff_spec; ///< A workspace holding the generalised second difference
  int spec_number;      ///< The spectrum index to search for peaks
  int smooth_npts;      ///< The number of points to use in the smoothing
  int smooth_iter;      ///< The number of smoothing iterations
  double threashold;    ///< The threshold (number of sigma) intensity for a candidate peak to pass
  API::ITableWorkspace_sptr peaks; ///< Table Workspace to store peaks

};

} // namespace Algorithms
} // namespace Mantid

#endif /*MANTID_ALGORITHMS_FINDPEAKS1D_H_*/
