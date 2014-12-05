/*
 * StripVanadiumPeaks.h
 *
 *  Created on: Sep 10, 2010
 *      Author: janik
 */

#ifndef MANTID_ALGORITHMS_STRIPVANADIUMPEAKS_H_
#define MANTID_ALGORITHMS_STRIPVANADIUMPEAKS_H_

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

    Copyright &copy; 2008-9 ISIS Rutherford Appleton Laboratory, NScD Oak Ridge National Laboratory & European Spallation Source

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

    File change history is stored at: <https://github.com/mantidproject/mantid>
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
    ///Summary of algorithms purpose
    virtual const std::string summary() const {return "This algorithm removes peaks (at vanadium d-spacing positions by default) out of a background by linearly interpolating over the expected peak positions.";}

  /// Algorithm's version
  virtual int version() const { return (1); }
  /// Algorithm's category for identification
  virtual const std::string category() const { return "CorrectionFunctions;Optimization\\PeakFinding;Diffraction"; }

private:
  
  /// Initialisation code
  void init();
  ///Execution code
  void exec();

};

} // namespace Algorithms
} // namespace Mantid



#endif /* STRIPVANADIUMPEAKS_H_ */


