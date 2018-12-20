#ifndef MANTID_DATAHANDLING_MASKDETECTORS_H_
#define MANTID_DATAHANDLING_MASKDETECTORS_H_

#include "MantidAPI/Algorithm.h"
#include "MantidDataObjects/EventWorkspace.h"
#include "MantidDataObjects/MaskWorkspace.h"
#include "MantidDataObjects/PeaksWorkspace.h"

namespace Mantid {
namespace Indexing {
class SpectrumNumber;
}
namespace DataHandling {
/** An algorithm to mask a detector, or set of detectors.
    The workspace spectra associated with those detectors are zeroed.

    Required Properties:
    <UL>
    <LI> Workspace - The name of the (input & output) Workspace on which to
   perform the algorithm </LI>
    </UL>

    Optional Properties (One should be set. The highest listed below will be
   used if more than one is.):
    <UL>
    <LI> SpectraList - An ArrayProperty containing a list of spectra to mask
   </LI>
    <LI> DetectorList - An ArrayProperty containing a list of detector IDs to
   mask </LI>
    <LI> WorkspaceIndexList - An ArrayProperty containing the workspace indices
   to mask </LI>
    </UL>

    Copyright &copy; 2008-2012 ISIS Rutherford Appleton Laboratory, NScD Oak
   Ridge National Laboratory & European Spallation Source

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

    File change history is stored at: <https://github.com/mantidproject/mantid>.
    Code Documentation is available at: <http://doxygen.mantidproject.org>
*/
class DLLExport MaskDetectors : public API::Algorithm {
public:
  /// Algorithm's name for identification overriding a virtual method
  const std::string name() const override { return "MaskDetectors"; };
  /// Summary of algorithms purpose
  const std::string summary() const override {
    return "An algorithm to mask a detector, or set of detectors, as not to be "
           "used. The workspace spectra associated with those detectors are "
           "zeroed.";
  }

  /// Algorithm's version for identification overriding a virtual method
  int version() const override { return 1; };
  const std::vector<std::string> seeAlso() const override {
    return {"MaskDetectorsInShape",
            "MaskDetectorsIf",
            "MaskInstrument",
            "MaskSpectra",
            "MaskBTP",
            "MaskAngle",
            "InvertMask"};
  }
  /// Algorithm's category for identification overriding a virtual method
  const std::string category() const override { return "Transforms\\Masking"; }

private:
  // create type for range information
  using RangeInfo = std::tuple<size_t, size_t, bool>;

  const std::string workspaceMethodName() const override {
    return "maskDetectors";
  }
  const std::string workspaceMethodInputProperty() const override {
    return "Workspace";
  }

  // Implement abstract Algorithm methods
  void init() override;
  void exec() override;

  /// Choose how to mask given that we have a mask workspace
  void
  handleMaskByMaskWorkspace(const DataObjects::MaskWorkspace_const_sptr maskWs,
                            const API::MatrixWorkspace_const_sptr WS,
                            std::vector<detid_t> &detectorList,
                            std::vector<size_t> &indexList,
                            const RangeInfo &rangeInfo);

  /// Choose how to mask given that we have a matrix workspace
  void handleMaskByMatrixWorkspace(const API::MatrixWorkspace_const_sptr maskWs,
                                   const API::MatrixWorkspace_const_sptr WS,
                                   std::vector<detid_t> &detectorList,
                                   std::vector<size_t> &indexList,
                                   const RangeInfo &rangeInfo);

  void execPeaks(DataObjects::PeaksWorkspace_sptr WS);
  void
  fillIndexListFromSpectra(std::vector<size_t> &indexList,
                           std::vector<Indexing::SpectrumNumber> spectraList,
                           const API::MatrixWorkspace_sptr WS,
                           const RangeInfo &range_info);
  void appendToDetectorListFromComponentList(
      std::vector<detid_t> &detectorList,
      const std::vector<std::string> &componentList,
      const API::MatrixWorkspace_const_sptr WS);
  void
  appendToIndexListFromWS(std::vector<size_t> &indexList,
                          const API::MatrixWorkspace_const_sptr maskedWorkspace,
                          const RangeInfo &range_info);
  void appendToDetectorListFromWS(
      std::vector<detid_t> &detectorList,
      const API::MatrixWorkspace_const_sptr inputWs,
      const API::MatrixWorkspace_const_sptr maskWs,
      const std::tuple<size_t, size_t, bool> &range_info);
  void appendToIndexListFromMaskWS(
      std::vector<size_t> &indexList,
      const DataObjects::MaskWorkspace_const_sptr maskedWorkspace,
      const std::tuple<size_t, size_t, bool> &range_info);
  void
  extractMaskedWSDetIDs(std::vector<detid_t> &detectorList,
                        const DataObjects::MaskWorkspace_const_sptr &maskWS);
  void constrainMaskedIndexes(std::vector<size_t> &indexList,
                              const RangeInfo &range_info);

  RangeInfo getRanges(const API::MatrixWorkspace_sptr &targWS);
};

} // namespace DataHandling
} // namespace Mantid

#endif /*MANTID_DATAHANDLING_MASKDETECTORS_H_*/
