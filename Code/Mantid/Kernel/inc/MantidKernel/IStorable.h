#ifndef ISTORABLE_H_
#define ISTORABLE_H_

namespace Mantid
{
namespace Kernel
{
/** @class IStorable IStorable.h Kernel/IStorable.h

    An interface that is implemented by WorkspaceProperty.
    Used for storing workspaces into the AnalysisDataService.

    @author Russell Taylor, Tessella Support Services plc
    @date 11/12/2007
    
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
    
    File change history is stored at: <https://svn.mantidproject.org/mantid/trunk/Code/Mantid>.
    Code Documentation is available at: <http://doxygen.mantidproject.org>
*/
class IStorable
{
public:
  /// Store a workspace into the AnalysisDataService
  virtual bool store() = 0;
  /// Clear the stored pointer
  virtual void clear() = 0;
  /// Virtual destructor
  virtual ~IStorable() {}
};

} // namespace Kernel
} // namespace Mantid

#endif /*ISTORABLE_H_*/
