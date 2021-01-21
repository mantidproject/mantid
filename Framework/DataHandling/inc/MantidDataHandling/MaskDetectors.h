// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2008 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

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
    return {"MaskDetectorsInShape", "MaskDetectorsIf", "MaskSpectra", "MaskBTP", "MaskAngle", "InvertMask"};
  }
  /// Algorithm's category for identification overriding a virtual method
  const std::string category() const override { return "Transforms\\Masking"; }

private:
  // create type for range information
  using RangeInfo = std::tuple<size_t, size_t, bool>;

  const std::string workspaceMethodName() const override { return "maskDetectors"; }
  const std::string workspaceMethodInputProperty() const override { return "Workspace"; }

  // Implement abstract Algorithm methods
  void init() override;
  void exec() override;

  /// Choose how to mask given that we have a mask workspace
  void handleMaskByMaskWorkspace(const DataObjects::MaskWorkspace_const_sptr &maskWs,
                                 const API::MatrixWorkspace_const_sptr &WS, std::vector<detid_t> &detectorList,
                                 std::vector<size_t> &indexList, const RangeInfo &rangeInfo);

  /// Choose how to mask given that we have a matrix workspace
  void handleMaskByMatrixWorkspace(const API::MatrixWorkspace_const_sptr &maskWs,
                                   const API::MatrixWorkspace_const_sptr &WS, std::vector<detid_t> &detectorList,
                                   std::vector<size_t> &indexList, const RangeInfo &rangeInfo);

  void execPeaks(const DataObjects::PeaksWorkspace_sptr &WS);
  void fillIndexListFromSpectra(std::vector<size_t> &indexList, std::vector<Indexing::SpectrumNumber> spectraList,
                                const API::MatrixWorkspace_sptr &WS, const RangeInfo &range_info);
  void appendToDetectorListFromComponentList(std::vector<detid_t> &detectorList,
                                             const std::vector<std::string> &componentList,
                                             const API::MatrixWorkspace_const_sptr &WS);
  void appendToIndexListFromWS(std::vector<size_t> &indexList, const API::MatrixWorkspace_const_sptr &maskedWorkspace,
                               const RangeInfo &range_info);
  void appendToDetectorListFromWS(std::vector<detid_t> &detectorList, const API::MatrixWorkspace_const_sptr &inputWs,
                                  const API::MatrixWorkspace_const_sptr &maskWs,
                                  const std::tuple<size_t, size_t, bool> &range_info);
  void appendToIndexListFromMaskWS(std::vector<size_t> &indexList,
                                   const DataObjects::MaskWorkspace_const_sptr &maskedWorkspace,
                                   const std::tuple<size_t, size_t, bool> &range_info);
  void extractMaskedWSDetIDs(std::vector<detid_t> &detectorList, const DataObjects::MaskWorkspace_const_sptr &maskWS);
  void constrainMaskedIndexes(std::vector<size_t> &indexList, const RangeInfo &range_info);

  RangeInfo getRanges(const API::MatrixWorkspace_sptr &targWS);
};

} // namespace DataHandling
} // namespace Mantid
