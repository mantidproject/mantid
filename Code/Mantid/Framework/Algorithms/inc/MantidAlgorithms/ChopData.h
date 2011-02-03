#ifndef MANTID_ALGORITHMS_CHOPDATA
#define MANTID_ALGORITHMS_CHOPDATA

#include "MantidAPI/Algorithm.h"

namespace Mantid
{
namespace Algorithms
{
/**

  For use in TOSCA reduction. Splits a 0-100k microsecond workspace into either five 20k or
  three 20k and a 40k workspaces

  @author Michael Whitty
  @date 03/02/2011

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
  class DLLExport ChopData : public API::Algorithm
  {
  public:
    ChopData() : API::Algorithm() {} ///< Empty constructor
    virtual ~ChopData() {} ///< Empty destructor

    virtual const std::string name() const { return "ChopData"; } ///< @return the algorithms name
    virtual const std::string category() const { return "General"; } ///< @return the algorithms category
    virtual int version() const { return (1); } ///< @return version number of algorithm

  private:
    void init(); ///< Initialise the algorithm. Declare properties, etc.
    void exec(); ///< Executes the algorithm.
  };
}
}
#endif
