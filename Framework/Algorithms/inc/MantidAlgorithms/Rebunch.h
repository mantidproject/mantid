#ifndef MANTID_ALGORITHM_REBUNCH_H_
#define MANTID_ALGORITHM_REBUNCH_H_

//----------------------------------------------------------------------
// Includes
//----------------------------------------------------------------------
#include "MantidAPI/Algorithm.h"

namespace Mantid {
namespace HistogramData {
class HistogramX;
class HistogramY;
class HistogramE;
} // namespace HistogramData
namespace Algorithms {
/**

Takes a workspace as input and rebunches the data according to the input rebunch
parameters.
    The process is exactly that defined by the mgenie code of T. G. Perring for
both point and histogram data,
    in that the the distribution flag is ignored for point data, and the
contributing point data points are averaged.

    Required Properties:
<UL>
<LI> InputWorkspace - The name of the Workspace to take as input </LI>
<LI> n_bunch - number of data points to bunch together for each new point, must
be >= 1 </LI>
<LI> OutputWorkspace - The name of the workspace in which to store the result
</LI>
</UL>


@author Dickon Champion, STFC
@date 24/06/2008

Copyright &copy; 2007 ISIS Rutherford Appleton Laboratory, NScD Oak Ridge
National Laboratory & European Spallation Source

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
class DLLExport Rebunch : public API::Algorithm {
public:
  /// Algorithm's name for identification overriding a virtual method
  const std::string name() const override { return "Rebunch"; }
  /// Summary of algorithms purpose
  const std::string summary() const override {
    return "Rebins data by adding together 'n_bunch' successive bins.";
  }

  /// Algorithm's version for identification overriding a virtual method
  int version() const override { return 1; }
  const std::vector<std::string> seeAlso() const override { return {"Rebin"}; }
  /// Algorithm's category for identification overriding a virtual method
  const std::string category() const override { return "Transforms\\Rebin"; }

private:
  // Overridden Algorithm methods
  void init() override;
  void exec() override;
  void rebunch_hist_counts(const HistogramData::HistogramX &xold,
                           const HistogramData::HistogramY &yold,
                           const HistogramData::HistogramE &eold,
                           HistogramData::HistogramX &xnew,
                           HistogramData::HistogramY &ynew,
                           HistogramData::HistogramE &enew,
                           const size_t n_bunch);
  void rebunch_hist_frequencies(const HistogramData::HistogramX &xold,
                                const HistogramData::HistogramY &yold,
                                const HistogramData::HistogramE &eold,
                                HistogramData::HistogramX &xnew,
                                HistogramData::HistogramY &ynew,
                                HistogramData::HistogramE &enew,
                                const size_t n_bunch);
  void rebunch_point(const HistogramData::HistogramX &xold,
                     const HistogramData::HistogramY &yold,
                     const HistogramData::HistogramE &eold,
                     HistogramData::HistogramX &xnew,
                     HistogramData::HistogramY &ynew,
                     HistogramData::HistogramE &enew, const size_t n_bunch);
};

} // namespace Algorithms
} // namespace Mantid

#endif /*MANTID_ALGORITHM_REBUNCH_H_*/
