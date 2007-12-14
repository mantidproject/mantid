#ifndef MANTID_ALGORITHM_BINARYOPHELPER_H_
#define MANTID_ALGORITHM_BINARYOPHELPER_H_

//----------------------------------------------------------------------
// Includes
//----------------------------------------------------------------------
#include <algorithm>
#include <functional>
#include <iterator>

#include "MantidAPI/Workspace.h" 
#include "MantidDataObjects/Workspace1D.h" 
#include "MantidDataObjects/Workspace2D.h" 

namespace Mantid
{
namespace Algorithms
{
  /** @class BinaryOpHelper BinaryOpHelper.h Algorithms/BinaryOpHelper.h

	  BinaryOpHelper is an helper class that contains common functionality used for binary operation algorithms such as plus and minus.
 
    @author Nick Draper
    @date 14/12/2007
    
    Copyright &copy; 2007 STFC Rutherford Appleton Laboratories

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
*/

class DLLExport BinaryOpHelper
{
public:
  /// Default constructor
  BinaryOpHelper() {};
	/// Destructor
	~BinaryOpHelper() {};
	
  const bool checkSizeCompatability(const API::Workspace* ws1,const API::Workspace* ws2) const;

  API::Workspace* createOutputWorkspace(const API::Workspace* ws1, const API::Workspace* ws2) const;

private:
  /// Static reference to the logger class
  static Mantid::Kernel::Logger& g_log;
  

};

} // namespace Algorithm
} // namespace Mantid

#endif /*MANTID_ALGORITHM_BINARYOPHELPER_H_*/
