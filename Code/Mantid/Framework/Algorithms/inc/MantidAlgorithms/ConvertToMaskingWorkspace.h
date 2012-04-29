#ifndef MANTID_ALGORITHMS_CONVERTTOMASKINGWORKSPACE_H_
#define MANTID_ALGORITHMS_CONVERTTOMASKINGWORKSPACE_H_

#include "MantidAPI/DeprecatedAlgorithm.h"
#include "MantidAlgorithms/ConvertToMaskWorkspace.h"


namespace Mantid
{
namespace Algorithms
{

  /** ConvertToMaskingWorkspace : TODO: DESCRIPTION
    
    @date 2011-12-29

    Copyright &copy; 2011 ISIS Rutherford Appleton Laboratory & NScD Oak Ridge National Laboratory

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
class DLLExport ConvertToMaskingWorkspace : public Algorithms::ConvertToMaskWorkspace, public API::DeprecatedAlgorithm
  {
  public:
    ConvertToMaskingWorkspace();
    virtual ~ConvertToMaskingWorkspace();
    
    virtual const std::string name() const {return "ConvertToMaskingWorkspace"; };
    virtual int version() const {return 1; };

  };

} // namespace Algorithms
} // namespace Mantid

#endif  /* MANTID_ALGORITHMS_CONVERTTOMASKINGWORKSPACE_H_ */
