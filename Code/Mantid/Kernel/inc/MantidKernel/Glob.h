#ifndef MANTID_KERNEL_GLOB_H_
#define MANTID_KERNEL_GLOB_H_

//----------------------------------------------------------------------
// Includes
//----------------------------------------------------------------------
#include "MantidKernel/System.h"
#include <Poco/Glob.h>
#include <Poco/Path.h>


namespace Mantid
{
namespace Kernel
{
/** This Glob class overrides the glob() method of Poco::Glob class
    to make it more reliable.

    @author Roman Tolchenov, Tessella plc
    @date 23/07/2009

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

    File change history is stored at: <https://svn.mantidproject.org/mantid/trunk/Code/Mantid>.
    Code Documentation is available at: <http://doxygen.mantidproject.org>
*/

class DLLExport Glob:public Poco::Glob
{
public:
    /// Creates a set of files that match the given pathPattern.
    static void glob(const Poco::Path& pathPattern, std::set<std::string>& files, int options = 0);
    /// Creates a set of files that match the given pathPattern.
    static void glob(const std::string& base, const std::string& pathPattern, std::set<std::string>& files, int options = 0);
    /// Creates a set of files that match the given pathPattern.
	static void glob(const Poco::Path& base, const Poco::Path& pathPattern, std::set<std::string>& files, int options = 0);
};


} // namespace Kernel
} // namespace Mantid

#endif /*MANTID_KERNEL_GLOB_H_*/
