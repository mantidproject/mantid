#ifndef MANTID_LIVEDATA_EXCEPTION_H_
#define MANTID_LIVEDATA_EXCEPTION_H_

//----------------------------------------------------------------------
// Includes
//----------------------------------------------------------------------
#include <stdexcept>

namespace Mantid
{
namespace LiveData
{
namespace Exception
{

/** An exception that can be thrown by an ILiveListener implementation to notify LoadLiveData
    that it is not yet ready to return data. This could be, for example, because it has not
    yet completed its initialisation step or if the instrument from which data is being read
    is not in a run. LoadLiveData will ask for data again after a short delay.
    Other exceptions thrown by the listener will have the effect of stopping the algorithm.

    Copyright &copy; 2013 ISIS Rutherford Appleton Laboratory, NScD Oak Ridge National Laboratory & European Spallation Source

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

    File change history is stored at: <https://github.com/mantidproject/mantid>.
    Code Documentation is available at: <http://doxygen.mantidproject.org>
*/
class NotYet : public std::runtime_error
{
public:
  /** Constructor
   *  @param message A description of the exceptional condition
   */
  explicit NotYet(const std::string& message) : std::runtime_error(message)
  {}

  /// Destructor
  ~NotYet() throw() {}
};


} // namespace Exception
} // namespace LiveData
} // namespace Mantid

#endif // MANTID_LIVEDATA_EXCEPTION_H_
