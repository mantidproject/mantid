#ifndef MANTID_PYTHONINTERFACE_WEAKPTR_H_
#define MANTID_PYTHONINTERFACE_WEAKPTR_H_
/*

  This file declares the get_pointer template function to allow
  boost python to understand weak_pointers. It must be add to the
  boost namespace

  Copyright &copy; 2012 ISIS Rutherford Appleton Laboratory, NScD Oak Ridge National Laboratory & European Spallation Source

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
#include <boost/weak_ptr.hpp>
#include <stdexcept>

namespace boost
{
  /**
   * Boost.Python doesn't understand weak_ptrs out of the box. This acts an intermediary
   * so that a bare pointer can be retrieved from the wrapper. The important
   * bit here is that the weak pointer won't allow the bare pointer to be retrieved
   * unless the object it points to still exists
   * The name and arguments are dictated by boost
   * @param dataItem :: A reference to the weak_ptr
   * @return A bare pointer to the HeldType
   */
  template <typename HeldType>
  inline HeldType * get_pointer(const boost::weak_ptr<HeldType> & dataItem )
  {
    if( boost::shared_ptr<HeldType>  lockedItem = dataItem.lock() )
    {
      return lockedItem.get(); // Safe as we can guarantee that another reference exists
    }
    else
    {
      throw std::runtime_error("Variable invalidated, data has been deleted.");
    }
  }

}



#endif /* MANTID_PYTHONINTERFACE_WEAKPTR_H_ */
