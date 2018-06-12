#ifndef MANTID_ALGORITHMS_MASKBINS_H_
#define MANTID_ALGORITHMS_MASKBINS_H_

#include "MantidAPI/DistributedAlgorithm.h"
#include "MantidDataObjects/EventList.h"
#include "MantidDataObjects/EventWorkspace.h"
#include "MantidIndexing/SpectrumIndexSet.h"

namespace Mantid {

namespace Histogram {
class BinEdges;
}
namespace Algorithms {
/** Masks bins in a workspace. Bins falling within the range given (even
   partially) are
    masked, i.e. their data and error values are set to zero and the bin is
   added to the
    list of masked bins. This range is masked for all spectra in the workspace
   (though the
    workspace does not have to have common X values in all spectra).

    Required Properties:
    <UL>
    <LI> InputWorkspace - The name of the Workspace to take as input. </LI>
    <LI> OutputWorkspace - The name of the Workspace containing the masked bins.
   </LI>
    <LI> XMin - The value to start masking from.</LI>
    <LI> XMax - The value to end masking at.</LI>
    </UL>

    @author Russell Taylor, Tessella plc
    @date 29/04/2009

    Copyright &copy; 2009 ISIS Rutherford Appleton Laboratory, NScD Oak Ridge
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
class DLLExport MaskBins : public API::DistributedAlgorithm {
public:
  /// Algorithm's name
  const std::string name() const override { return "MaskBins"; }
  /// Summary of algorithms purpose
  const std::string summary() const override {
    return "Marks bins in a workspace as being masked.";
  }

  /// Algorithm's version
  int version() const override { return (1); }
  const std::vector<std::string> seeAlso() const override {
    return {"MaskBinsFromTable"};
  }
  /// Algorithm's category for identification
  const std::string category() const override { return "Transforms\\Masking"; }

private:
  /// Initialisation code
  void init() override;
  /// Execution code
  void exec() override;
  void execEvent();

  void findIndices(const HistogramData::BinEdges &X,
                   MantidVec::difference_type &startBin,
                   MantidVec::difference_type &endBin);

  double m_startX{0.0}; ///< The range start point
  double m_endX{0.0};   ///< The range end point
  Indexing::SpectrumIndexSet
      indexSet; ///<the list of Spectra (workspace index) to load
};

} // namespace Algorithms
} // namespace Mantid

#endif /*MANTID_ALGORITHMS_MASKBINS_H_*/
