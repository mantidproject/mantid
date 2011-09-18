#ifndef MANTID_ALGORITHMS_IDENTIFYNOISYDETECTORS
#define MANTID_ALGORITHMS_IDENTIFYNOISYDETECTORS
/*WIKI* 

The process for this algorithm is:
* The standard deviation for each pixel within the pre-set range is calculated.
* The mean value and standard deviation of these standard deviation values is calculated.
* Any detector/pixel for which it's standard deviation value satisfied the following conditions:
** sdev(pixel) < mean(sdevs) - 3 * sdev(sdevs)
** sdev(pixel) > mean(sdevs) + 3 * sdev(sdevs)
** sdev(pixel) < mean(sdevs) * 0.0001
* (cont) is considered to be "noisy".

This is repeated three times from the second step.

This uses the [[Integration]], [[Power]] and [[Divide]] algorithms for the first step.

The lower bound for the integration is currently fixed to 2000.

The upper bound for the integration is currently fixed to 19000.


*WIKI*/

#include "MantidAPI/Algorithm.h"

namespace Mantid
{
namespace Algorithms
{
/**
  Identifies "bad" detectors based on their standard deviation, and how this differs from the
  standard deviation of other detectors. Runs through the process three times to get a narrower
  view.
  @author Michael Whitty
  @date 24/01/2011

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
class DLLExport IdentifyNoisyDetectors : public API::Algorithm
{
  public:
    IdentifyNoisyDetectors() : API::Algorithm() {} ///< Empty constructor
    virtual ~IdentifyNoisyDetectors() {} ///< Empty destructor

    virtual const std::string name() const { return "IdentifyNoisyDetectors"; } ///< @return the algorithms name
    virtual const std::string category() const { return "General"; } ///< @return the algorithms category
    virtual int version() const { return (1); } ///< @return version number of algorithm

  private:
    /// Sets documentation strings for this algorithm
    virtual void initDocs();
    void init(); ///< Initialise the algorithm. Declare properties, etc.
    void exec(); ///< Executes the algorithm.

    void getStdDev(Mantid::API::MatrixWorkspace_sptr valid, 
      Mantid::API::MatrixWorkspace_sptr values);
};
}
}
#endif
