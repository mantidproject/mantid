#ifndef MANTID_ALGORITHMS_FILTERBYTIME2_H_
#define MANTID_ALGORITHMS_FILTERBYTIME2_H_

#include "MantidKernel/System.h"
#include "MantidAPI/Algorithm.h"
#include "MantidKernel/DateAndTime.h"

namespace Mantid
{
namespace Algorithms
{

  /** FilterByTime2 : TODO: DESCRIPTION
    
    @date 2012-04-25

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

    File change history is stored at: <https://github.com/mantidproject/mantid>
    Code Documentation is available at: <http://doxygen.mantidproject.org>
  */
  class DLLExport FilterByTime2 : public API::Algorithm
  {
  public:
    FilterByTime2();
    virtual ~FilterByTime2();

    /// Algorithm's name for identification overriding a virtual method
    virtual const std::string name() const { return "FilterByTime";};
    /// Algorithm's version for identification overriding a virtual method
    virtual int version() const { return 2;};
    /// Algorithm's category for identification overriding a virtual method
    virtual const std::string category() const { return "Events\\EventFiltering";}
    /// Algorithm's summary for identification overriding a virtual method
    virtual const std::string summary() const { return "Filter event data by time."; }

  private:
    /// Sets documentation strings for this algorithm

    // Implement abstract Algorithm methods
    void init();
    void exec();

  };


} // namespace Algorithms
} // namespace Mantid

#endif  /* MANTID_ALGORITHMS_FILTERBYTIME2_H_ */
