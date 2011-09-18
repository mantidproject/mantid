#ifndef MANTID_ALGORITHMS_FLATBACKGROUND_H_
#define MANTID_ALGORITHMS_FLATBACKGROUND_H_
/*WIKI* 

This algorithm takes a list of spectra and for each spectrum calculates an average count rate in the given region, usually a region when there are only background neutrons. This count rate is then subtracted from the counts in all the spectrum's bins. However, no bin will take a negative value as bins with count rates less than the background are set to zero (and their error is set to the backgound value).

The average background count rate is estimated in one of two ways. When Mode is set to 'Mean' it is the sum of the values in the bins in the background region divided by the width of the X range. Selecting 'Linear Fit' sets the background value to the height in the centre of the background region of a line of best fit through that region.

The error on the background value is only calculated when 'Mean' is used. It is the errors in all the bins in the background region summed in quadrature divided by the number of bins. This background error value is added in quadrature to the errors in each bin.

====Subalgorithms used====
The [[Linear]] algorithm is used when the Mode = Linear Fit. From the resulting line of best fit a constant value taken as the value of the line at the centre of the fitted range.


*WIKI*/

//----------------------------------------------------------------------
// Includes
//----------------------------------------------------------------------
#include "MantidAPI/Algorithm.h"

namespace Mantid
{
namespace Algorithms
{
/** Finds a constant value fit to an appropriate range of each desired spectrum
    and subtracts that value from the entire spectrum.

    Required Properties:
    <UL>
    <LI> InputWorkspace     - The name of the input workspace. </LI>
    <LI> OutputWorkspace    - The name to give the output workspace. </LI>
    <LI> SpectrumIndexList  - The workspace indices of the spectra to fit background to. </LI>
    <LI> StartX             - The start of the flat region to fit to. </LI>
    <LI> EndX               - The end of the flat region to fit to. </LI>
    <LI> Mode               - How to estimate the background number of counts: a linear fit or the mean. </LI>
    <LI> OutputMode         - What to return in the Outputworkspace: the corrected signal or just the background. </LI>
    </UL>

    @author Russell Taylor, Tessella plc
    @date 5/02/2009

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
class DLLExport FlatBackground : public API::Algorithm
{
public:
  /// (Empty) Constructor
  FlatBackground() : API::Algorithm(), m_convertedFromRawCounts(false), m_progress(NULL) {}
  /// Virtual destructor
  virtual ~FlatBackground() {if(m_progress) delete m_progress;m_progress=NULL;}
  /// Algorithm's name
  virtual const std::string name() const { return "FlatBackground"; }
  /// Algorithm's version
  virtual int version() const { return (1); }
  /// Algorithm's category for identification
  virtual const std::string category() const { return "SANS"; }

private:
  /// Sets documentation strings for this algorithm
  virtual void initDocs();
  /// Initialisation code
  void init();
  ///Execution code
  void exec();

  void convertToDistribution(API::MatrixWorkspace_sptr workspace);
  void restoreDistributionState(API::MatrixWorkspace_sptr workspace);
  void checkRange(double& startX, double& endX);
  void getSpecInds(std::vector<int> &output, const int workspaceTotal);
  double Mean(const API::MatrixWorkspace_const_sptr WS, const int specInd, const double startX, const double endX, double &variance) const;
  double LinearFit(API::MatrixWorkspace_sptr WS, int spectrum, double startX, double endX);

  /// variable bin width raw count data must be converted to distributions first and then converted back, keep track of this
  bool m_convertedFromRawCounts;
  /// Progress reporting
  API::Progress* m_progress;

};

} // namespace Algorithms
} // namespace Mantid

#endif /*MANTID_ALGORITHMS_FLATBACKGROUND_H_*/
