#ifndef MANTID_ALGORITHMS_POINTBYPOINTVCORRECTION_H_
#define MANTID_ALGORITHMS_POINTBYPOINTVCORRECTION_H_
/*WIKI* 

Divides the data spectra by the matching vanadium spectra according to the following formula:

<math>(y_i)_{Norm}=(\frac{y_i}{v_i})*\Delta w_i*\frac{\sum_i{y_i}}{\sum_i((\frac{y_i}{v_i})\Delta w_i)}</math>

where <math>y_i</math> is the signal in the sample workspace, <math>v_i</math> the count in the corresponding vanadium bin, <math>\Delta w_i</math> the bin width, <math>\sum_i{y_i}</math> the integrated data count and <math>\sum_i((\frac{y_i}{v_i})\Delta w_i)</math> the sum of the sample counts divided by the vanadium counts multiplied by the bin width.

This leads to a normalised histogram which has unit of counts, as before.

In order to minimise sudden jumps in the data caused by 0 counts in the corresponding vanadium spectrum it is worth considering smoothing the Vanadium spectrum using [[SmoothData]] prior to using it in this algorithm.

=== Valid input workspaces ===
The input workspaces have to have the following in order to be valid inputs for this algorithm.
* The same number of spectra
* Matching spectra numbers
This is normally not a problem unless the setup of the instrument has been changed between recording the Vanadium and the sample datasets.


*WIKI*/

//----------------------------------------------------------------------
// Includes
//----------------------------------------------------------------------
#include "MantidAPI/Algorithm.h"

namespace Mantid
{
namespace Algorithms
{
/** First attempt at spectrum by spectrum division for vanadium normalisation correction.

    @author Laurent Chapon, ISIS Facility, Rutherford Appleton Laboratory
    @date 04/03/2009

    Copyright &copy; 2009 ISIS Rutherford Appleton Laboratory & NScD Oak Ridge National Laboratory

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
class DLLExport PointByPointVCorrection : public API::Algorithm
{
public:
  PointByPointVCorrection();
  virtual ~PointByPointVCorrection();
  /// Algorithm's name for identification overriding a virtual method
  virtual const std::string name() const { return "PointByPointVCorrection"; }
  /// Algorithm's version for identification overriding a virtual method
  virtual int version() const { return 1; }
  /// Algorithm's category for identification overriding a virtual method
  virtual const std::string category() const { return "Diffraction";}

private:
  /// Sets documentation strings for this algorithm
  virtual void initDocs();
  // Overridden Algorithm methods
  void init();
  void exec();
  void check_validity(API::MatrixWorkspace_const_sptr& w1,
		  API::MatrixWorkspace_const_sptr& w2,API::MatrixWorkspace_sptr& out);
  void check_masks(const API::MatrixWorkspace_const_sptr& w1,
		  const API::MatrixWorkspace_const_sptr& w2, const int& index) const;
};

} // namespace Algorithm
} // namespace Mantid

#endif /* MANTID_ALGORITHMS_POINTBYPOINTVCORRECTION_H_ */
