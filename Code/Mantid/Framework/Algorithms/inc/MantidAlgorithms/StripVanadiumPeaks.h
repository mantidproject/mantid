/*
 * StripVanadiumPeaks.h
 *
 *  Created on: Sep 10, 2010
 *      Author: janik
 */

#ifndef MANTID_ALGORITHMS_STRIPVANADIUMPEAKS_H_
#define MANTID_ALGORITHMS_STRIPVANADIUMPEAKS_H_
/*WIKI* 


* If AlternativePeakPositions is specified, it is used for the central peak positions; otherwise, this list is used: 0.5044,0.5191,0.5350,0.5526,0.5936,0.6178,0.6453,0.6768,0.7134,0.7566,0.8089,0.8737,0.9571,1.0701,1.2356,1.5133,2.1401

* Using the peak center position C as the starting point, the width <math>W = C * PeakWidthPercent/100</math> is calculated.

* A linear background fit is performed by averaging the histogram from <math>x=-0.75W</math> to <math>-0.25W</math> on the left and  <math>x=0.25W</math> to <math>0.75W</math> on the right. The averaging reduces the error in the fit.

* The histogram y values from <math>x=-0.25W</math> to <math>+0.25W</math> are set to the interpolated values found from the linear fit found previously.


*WIKI*/

//----------------------------------------------------------------------
// Includes
//----------------------------------------------------------------------
#include "MantidAPI/Algorithm.h"
#include "MantidAPI/ITableWorkspace.h"

namespace Mantid
{
namespace Algorithms
{
/** StripVanadiumPeaks algorithm

    This algorithm takes a list of peak centers (or uses a default one for Vanadium peaks)
    and cuts them out by performing a linear fit of the Y values to the left and right of the peak:

    - The center of the peak C is specified in d-spacing.
    - A peak width W is specified as a percentage of the d-spacing at the peak center.
    - A width of W/2 is averaged on the left and right sides, centered at C +- W/2.
    - A linear fit is made from those two value.
    - The Y values between C-W/2 and C+W/2 are filled in with the result of the fit.

    @author Janik Zikovsky, SNS
    @date 2010-09-10

    Copyright &copy; 2008-9 ISIS Rutherford Appleton Laboratory & NScD Oak Ridge National Laboratory

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
class DLLExport StripVanadiumPeaks : public API::Algorithm
{
public:
  /// (Empty) Constructor
  StripVanadiumPeaks();
  /// Virtual destructor
  virtual ~StripVanadiumPeaks() {}
  /// Algorithm's name
  virtual const std::string name() const { return "StripVanadiumPeaks"; }
  /// Algorithm's version
  virtual int version() const { return (1); }
  /// Algorithm's category for identification
  virtual const std::string category() const { return "General"; }

private:
  /// Sets documentation strings for this algorithm
  virtual void initDocs();
  /// Initialisation code
  void init();
  ///Execution code
  void exec();

};

} // namespace Algorithms
} // namespace Mantid



#endif /* STRIPVANADIUMPEAKS_H_ */


