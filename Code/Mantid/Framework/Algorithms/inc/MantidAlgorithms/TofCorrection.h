#ifndef MANTID_ALGORITHMS_TOFCORRECTION
#define MANTID_ALGORITHMS_TOFCORRECTION

#include "MantidAPI/Algorithm.h"

namespace Mantid
{
namespace Algorithms
{
/**
  @author Michael Whitty
  @date 13/09/2010

  Copyright &copy; 2010 ISIS Rutherford Appleton Laboratory & NScD Oak Ridge National Laboratory

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
  class DLLExport TofCorrection : public API::Algorithm
  {
  public:
    TofCorrection() : API::Algorithm() {} ///< Empty constructor
    virtual ~TofCorrection() {} ///< Empty destructor

    virtual const std::string name() const { return "TofCorrection"; } ///< @return the algorithms name
    virtual const std::string category() const { return "General"; } ///< @return the algorithms category
    virtual int version() const { return (1); } ///< @return version number of algorithm

  private:
    void init(); ///< Initialise the algorithm. Declare properties, etc.
    void exec(); ///< Executes the algorithm.
  };
}
}
#endif
