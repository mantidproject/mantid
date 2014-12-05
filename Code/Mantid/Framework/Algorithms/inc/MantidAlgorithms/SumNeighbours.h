#ifndef MANTID_ALGORITHMS_SumNeighbours_H_
#define MANTID_ALGORITHMS_SumNeighbours_H_

//----------------------------------------------------------------------
// Includes
//----------------------------------------------------------------------
#include "MantidAPI/Algorithm.h"

namespace Mantid
{
namespace Algorithms
{
/** Sums neighboring pixels on rectangular detectors.
 * Each spectrum in the output workspace is a sum of a block of SumX*SumY pixels.
 * Only works on EventWorkspaces and for instruments with RectangularDetector's.
 *
 * This only works for instruments that have RectangularDetector's defined;
 * at the time of writing: TOPAZ, SNAP, PG3.
 *
    @author Janik Zikovsky, SNS
    @date Oct 2010

    Copyright &copy; 2007-2010 ISIS Rutherford Appleton Laboratory, NScD Oak Ridge National Laboratory & European Spallation Source

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
class DLLExport SumNeighbours : public API::Algorithm
{
public:
  /// Default constructor
  SumNeighbours() : API::Algorithm() {};
  /// Destructor
  virtual ~SumNeighbours() {};
  /// Algorithm's name for identification overriding a virtual method
  virtual const std::string name() const { return "SumNeighbours";}
    ///Summary of algorithms purpose
    virtual const std::string summary() const {return "Sum event lists from neighboring pixels in rectangular area detectors - e.g. to reduce the signal-to-noise of individual spectra. Each spectrum in the output workspace is a sum of a block of SumX*SumY pixels. Only works on EventWorkspaces and for instruments with RectangularDetector's.";}

  /// Algorithm's version for identification overriding a virtual method
  virtual int version() const { return (1);}
  /// Algorithm's category for identification overriding a virtual method
  virtual const std::string category() const { return "Transforms\\Grouping";}

private:
  
  // Overridden Algorithm methods
  void init();
  void exec();
  /// Number to sum
  int SumX;
  /// Number to sum
  int SumY;
};

} // namespace Algorithm
} // namespace Mantid

#endif /*MANTID_ALGORITHMS_SumNeighbours_H_*/
