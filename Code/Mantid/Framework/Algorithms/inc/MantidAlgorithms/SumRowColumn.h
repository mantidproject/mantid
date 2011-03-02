#ifndef MANTID_ALGORITHMS_SUMROWCOLUMN_H_
#define MANTID_ALGORITHMS_SUMROWCOLUMN_H_

//----------------------------------------------------------------------
// Includes
//----------------------------------------------------------------------
#include "MantidAPI/Algorithm.h"

namespace Mantid
{
namespace Algorithms
{
/** This algorithm is the equivalent of the COLETTE "DISPLAY H/V" command.
    It firsts integrates the input workspace, which must contain all the spectra from
    the detector of interest - no more and no less (so 128x128 or 192x192), 
    between the X values given. Then each row or column is summed between the H/V_Min/Max
    values, if given, and the result is a single spectrum of row or column number against
    total counts.

    Required Properties:
    <UL>
    <LI> InputWorkspace  - The name of the input workspace. </LI>
    <LI> OutputWorkspace - The name of the output workspace. </LI>
    <LI> Orientation     - Whether to sum rows (D_H) or columns (D_V) </LI>
    </UL>

    Optional properties:
    <UL>
    <LI> XMin - The starting X value for each spectrum to include in the summation (default min). </LI>
    <LI> XMax - The ending X value for each spectrum to include in the summation (default max). </LI>
    <LI> H/V_Min - The first row to include when summing by columns, or vice versa (default all). </LI>
    <LI> H/V_max - The last row to include when summing by columns, or vice versa (default all). </LI>
    </UL>

    @author Russell Taylor, Tessella plc
    @date 22/06/2009

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
class DLLExport SumRowColumn : public API::Algorithm
{
public:
  /// (Empty) Constructor
  SumRowColumn() : API::Algorithm() {}
  /// Virtual destructor
  virtual ~SumRowColumn() {}
  /// Algorithm's name
  virtual const std::string name() const { return "SumRowColumn"; }
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

  API::MatrixWorkspace_sptr integrateWorkspace();
};

} // namespace Algorithms
} // namespace Mantid

#endif /*MANTID_ALGORITHMS_SUMROWCOLUMN_H_*/
