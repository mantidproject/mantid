#ifndef MANTID_DATAHANDLING_LOADRAW2_H_
#define MANTID_DATAHANDLING_LOADRAW2_H_

//----------------------------------------------------------------------
// Includes
//----------------------------------------------------------------------
#include "MantidAPI/Algorithm.h"
#include "MantidAPI/DeprecatedAlgorithm.h"

namespace Mantid {
namespace DataHandling {
/** THIS VERSION OF THIS ALGORITHM HAS BEEN REMOVED FROM MANTID.
    Superseded by LoadRaw3.
    What's left in this class is just a stub.

    Copyright &copy; 2007-2013 ISIS Rutherford Appleton Laboratory, NScD Oak
   Ridge National Laboratory & European Spallation Source

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
class DLLExport LoadRaw2 : public API::Algorithm,
                           public API::DeprecatedAlgorithm {
public:
  /// Default constructor
  LoadRaw2();
  /// Destructor
  ~LoadRaw2() {}
  /// Algorithm's name for identification overriding a virtual method
  virtual const std::string name() const { return "LoadRaw"; }
  /// Summary of algorithms purpose
  virtual const std::string summary() const {
    return "*** This version of LoadRaw has been removed from Mantid. You "
           "should use the current version of this algorithm or try an earlier "
           "release of Mantid. ***";
  }

  /// Algorithm's version for identification overriding a virtual method
  virtual int version() const { return 2; }
  /// Algorithm's category for identification overriding a virtual method
  virtual const std::string category() const { return "DataHandling\\Raw"; }

private:
  void init();
  void exec();
};

} // namespace DataHandling
} // namespace Mantid

#endif /*MANTID_DATAHANDLING_LOADRAW2_H_*/
