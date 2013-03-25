#ifndef SORT_H_
#define SORT_H_

//----------------------------------------------------------------------
// Includes
//----------------------------------------------------------------------
#include "MantidAPI/Algorithm.h"
#include "MantidAPI/DeprecatedAlgorithm.h"
#include "MantidAlgorithms/SortEvents.h"

namespace Mantid
{
namespace Algorithms
{
/** Obsolete algorithm, renamed SortEvents

    @author Janik Zikovsky, SNS
    @date Friday, August 13, 2010.

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

    File change history is stored at: <https://github.com/mantidproject/mantid>
    Code Documentation is available at: <http://doxygen.mantidproject.org>
 */
class DLLExport Sort : public SortEvents, public API::DeprecatedAlgorithm
{
public:
  /// Default constructor
  Sort();
  /// Destructor
  virtual ~Sort() {};
  /// Algorithm's name for identification overriding a virtual method
  virtual const std::string name() const { return "Deprecated";}

};

} // namespace Algorithms
} // namespace Mantid


#endif /* SORT_H_ */
