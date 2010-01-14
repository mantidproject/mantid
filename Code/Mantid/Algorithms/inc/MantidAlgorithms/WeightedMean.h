#ifndef MANTID_ALGORITHMS_WEIGHTEDMEAN_H_
#define MANTID_ALGORITHMS_WEIGHTEDMEAN_H_

//----------------------------------------------------------------------
// Includes
//----------------------------------------------------------------------
#include "MantidAPI/Algorithm.h"

namespace Mantid
{
namespace Algorithms
{
/** An algorithm to calculate the weighted mean of two workspaces.

    @author Robert Dalgliesh, ISIS, RAL
    @date 12/1/2010

    Copyright &copy; 2010 STFC Rutherford Appleton Laboratory

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

    File change history is stored at: <https://svn.mantidproject.org/mantid/trunk/Code/Mantid>.
    Code Documentation is available at: <http://doxygen.mantidproject.org>
 */
class DLLExport WeightedMean : public API::Algorithm
{
public:
  /// Empty Constructor
  WeightedMean() : API::Algorithm() {}
  /// Empty Destructor
  virtual ~WeightedMean() {}

  virtual const std::string name() const { return "WeightedMean"; }
  virtual const int version() const { return (1); }
  virtual const std::string category() const { return "Arithmetic"; }

private:
  void init();
  void exec();
};

} // namespace Algorithms
} // namespace Mantid

#endif /*MANTID_ALGORITHMS_WEIGHTEDMEAN_H_*/

