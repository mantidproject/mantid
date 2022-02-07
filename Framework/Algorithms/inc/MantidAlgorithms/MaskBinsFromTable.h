// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2012 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include "MantidAPI/Algorithm.h"
#include "MantidAlgorithms/DllConfig.h"
#include "MantidDataObjects/TableWorkspace.h"

namespace Mantid {
namespace Algorithms {

/** MaskBinsFromTable : TODO: DESCRIPTION

  @date 2012-06-04
*/
class MANTID_ALGORITHMS_DLL MaskBinsFromTable : public API::Algorithm {
public:
  MaskBinsFromTable();

  /// Algorithm's name for identification overriding a virtual method
  const std::string name() const override { return "MaskBinsFromTable"; };
  /// Summary of algorithms purpose
  const std::string summary() const override { return "Mask bins from a table workspace. "; }

  /// Algorithm's version for identification overriding a virtual method
  int version() const override { return 1; };
  const std::vector<std::string> seeAlso() const override { return {"MaskBins"}; }
  /// Algorithm's category for identification overriding a virtual method
  const std::string category() const override { return "Transforms\\Masking"; }

private:
  // Implement abstract Algorithm methods
  void init() override;
  // Implement abstract Algorithm methods
  void exec() override;

  /// Process input Mask bin TableWorkspace.
  void processMaskBinWorkspace(const DataObjects::TableWorkspace_sptr &masktblws,
                               const API::MatrixWorkspace_sptr &dataws);
  /// Call MaskBins
  void maskBins(const API::MatrixWorkspace_sptr &dataws);
  /// Convert a list of detector IDs list (string) to a list of
  /// spectra/workspace indexes list
  std::string convertToSpectraList(const API::MatrixWorkspace_sptr &dataws, const std::string &detidliststr);

  /// Column indexes of XMin, XMax, SpectraList, DetectorIDsList
  int id_xmin, id_xmax, id_spec, id_dets;
  bool m_useDetectorID;
  bool m_useSpectrumID;

  /// Vector to store XMin, XMax and SpectraList
  std::vector<double> m_xminVec, m_xmaxVec;
  std::vector<std::string> m_spectraVec;
};

} // namespace Algorithms
} // namespace Mantid
