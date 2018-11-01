// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2012 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#ifndef MANTID_ALGORITHMS_MASKBINSFROMTABLE_H_
#define MANTID_ALGORITHMS_MASKBINSFROMTABLE_H_

#include "MantidAPI/Algorithm.h"
#include "MantidDataObjects/TableWorkspace.h"
#include "MantidKernel/System.h"

namespace Mantid {
namespace Algorithms {

/** MaskBinsFromTable : TODO: DESCRIPTION

  @date 2012-06-04
*/
class DLLExport MaskBinsFromTable : public API::Algorithm {
public:
  MaskBinsFromTable();

  /// Algorithm's name for identification overriding a virtual method
  const std::string name() const override { return "MaskBinsFromTable"; };
  /// Summary of algorithms purpose
  const std::string summary() const override {
    return "Mask bins from a table workspace. ";
  }

  /// Algorithm's version for identification overriding a virtual method
  int version() const override { return 1; };
  const std::vector<std::string> seeAlso() const override {
    return {"MaskBins"};
  }
  /// Algorithm's category for identification overriding a virtual method
  const std::string category() const override { return "Transforms\\Masking"; }

private:
  // Implement abstract Algorithm methods
  void init() override;
  // Implement abstract Algorithm methods
  void exec() override;

  /// Process input Mask bin TableWorkspace.
  void processMaskBinWorkspace(DataObjects::TableWorkspace_sptr masktblws,
                               API::MatrixWorkspace_sptr dataws);
  /// Call MaskBins
  void maskBins(API::MatrixWorkspace_sptr dataws);
  /// Convert a list of detector IDs list (string) to a list of
  /// spectra/workspace indexes list
  std::string convertToSpectraList(API::MatrixWorkspace_sptr dataws,
                                   std::string detidliststr);

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

#endif /* MANTID_ALGORITHMS_MASKBINSFROMTABLE_H_ */
