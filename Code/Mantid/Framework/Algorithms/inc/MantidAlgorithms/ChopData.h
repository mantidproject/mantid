#ifndef MANTID_ALGORITHMS_CHOPDATA
#define MANTID_ALGORITHMS_CHOPDATA
/*WIKI* 

This algorithm will chop the input workspace into equally sized workspaces, and adjust the X-values given so that they all begin from the same point. This is useful if your raw files contain multiple frames.

=== Identifying Extended Frames ===
[[File:ChopDataIntegrationExplanation.png|frame|Figure 1: Example Monitor Spectrum with Extended Frames]]

If the parameters ''IntegrationRangeLower'', ''IntegrationRangeUpper'' and ''MonitorWorkspaceIndex'' are provided to the algorithm, then it will attempt to identify where in the workspace the frames have been extended.

For example: looking at Figure 1 which shows an input workspace covering 100000 microseconds, we can see that the first frame covers forty thousand, and the other three cover twenty thousand each.

In order for Mantid to determine this programatically, it integrates over a range (defined by IntegrationRangeLower and IntegrationRangeUpper) for each "chop" of the data. If the relative values for this integration fall within certain bounds, then the chop is deemed to be a continuation of the previous one rather than a separate frame. If this happens, then they will be placed in the same workspace within the result group.

The algorithm will only look at the workspace given in ''MonitorWorkspaceIndex'' property to determine this. Though it is expected and recommended that you use a monitor spectrum for this purpose, it is not enforced so you may use a regular detector if you have cause to do so.


*WIKI*/

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
