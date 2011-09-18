#ifndef MANTID_ALGORITHMS_SMOOTHDATA_H_
#define MANTID_ALGORITHMS_SMOOTHDATA_H_
/*WIKI* 


Smooths out statistical jitter in a workspace's data by making each point the mean average of itself and
one or more points lying symmetrically either side of it. The statistical error on each point will be reduced by sqrt(npts) because more data is now contributing to it. For points at the end of each spectrum, a reduced number of smoothing points will be used. For example, if NPoints is 5 the first value in the spectrum will be smoothed by making it the average of the first 3 values, the next will use the first 4 and then the third and onwards will use the full 5 points in the averaging.


*WIKI*/

//----------------------------------------------------------------------
// Includes
//----------------------------------------------------------------------
#include "MantidAPI/Algorithm.h"

namespace Mantid
{
namespace Algorithms
{
/** Smooths the data of the input workspace by making each point the mean average of itself and
    one or more points lying symmetrically either side of it.

    Required Properties:
    <UL>
    <LI> InputWorkspace - The name of the workspace to take as input. </LI>
    <LI> OutputWorkspace - The name under which to store the output workspace. </LI>
    </UL>

    Optional Properties:
    <UL>
    <LI> NPoints - The number of points to average over. Must be at least 3 (the default). If
         an even number is given, it will be incremented by 1 to make it odd.</LI>
    </UL>

    @author Russell Taylor, Tessella Support Services plc
    @date 23/10/2008

    Copyright &copy; 2008 ISIS Rutherford Appleton Laboratory & NScD Oak Ridge National Laboratory

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
class DLLExport SmoothData : public API::Algorithm
{
public:
  /// (Empty) Constructor
  SmoothData() : API::Algorithm() {}
  /// Virtual destructor
  virtual ~SmoothData() {}
  /// Algorithm's name
  virtual const std::string name() const { return "SmoothData"; }
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

#endif /*MANTID_ALGORITHMS_SMOOTHDATA_H_*/
