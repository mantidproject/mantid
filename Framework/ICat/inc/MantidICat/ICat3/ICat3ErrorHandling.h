#ifndef MANTID_ICAT_ERRORHANDLING_H_
#define MANTID_ICAT_ERRORHANDLING_H_

//----------------------------------------------------------------------
// Includes
//----------------------------------------------------------------------
#include <stdexcept>
#include <string>

//----------------------------------------------------------------------
// Forward Declaration
//----------------------------------------------------------------------
namespace ICat3 {
class ICATPortBindingProxy;
}

namespace Mantid {
namespace ICat {

/** CErrorHandling class responsible for handling errors in Mantid-ICat
   Algorithms.
    This algorithm  gives the datsets for a given investigations record

    @author Sofia Antony, ISIS Rutherford Appleton Laboratory
    @date 07/07/2010
    Copyright &copy; 2010 ISIS Rutherford Appleton Laboratory, NScD Oak Ridge
   National Laboratory & European Spallation Source

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
class CErrorHandling {
public:
  /** This method throws the error string returned by gsoap to mantid upper
   * layer
   *  @param icat ICat proxy object
   */
  static void throwErrorMessages(ICat3::ICATPortBindingProxy &icat);
};

/** a class for Throwing Session exception in Catalog module
 */
class SessionException : public std::runtime_error {
private:
  /// error string
  std::string m_error;

public:
  /// constructor
  SessionException(const std::string &error);
  /// return the error message
  const char *what() const noexcept override;
};
} // namespace ICat
} // namespace Mantid
#endif
