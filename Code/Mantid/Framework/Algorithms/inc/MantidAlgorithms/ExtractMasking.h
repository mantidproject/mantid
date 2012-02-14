#ifndef MANTID_ALGORITHMS_EXTRACTMASKING_H_
#define MANTID_ALGORITHMS_EXTRACTMASKING_H_

#include "MantidAlgorithms/ExtractMask.h"
#include "MantidAPI/DeprecatedAlgorithm.h"


namespace Mantid
{
namespace Algorithms
{

  /** ExtractMasking : TODO: DESCRIPTION
    
    @date 2012-02-13

    Copyright &copy; 2012 ISIS Rutherford Appleton Laboratory & NScD Oak Ridge National Laboratory

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
  class DLLExport ExtractMasking : public Algorithms::ExtractMask, public API::DeprecatedAlgorithm
  {
  public:
    ExtractMasking();
    virtual ~ExtractMasking();
    
    /// Algorithm's name for identification overriding a virtual method
    virtual const std::string name() const {return "ExtractMasking";};

    /// Algorithm's version for identification overriding a virtual method
    virtual int version() const {return 1;};

  };


} // namespace Algorithms
} // namespace Mantid

#endif  /* MANTID_ALGORITHMS_EXTRACTMASKING_H_ */
