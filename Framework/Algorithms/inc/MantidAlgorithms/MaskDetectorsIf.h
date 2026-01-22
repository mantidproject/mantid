// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2009 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

//----------------------------------------------------------------------
// Includes
//----------------------------------------------------------------------
#include "MantidAPI/Algorithm.h"
#include "MantidAlgorithms/DllConfig.h"
#include "MantidGeometry/IDTypes.h"
#include <boost/function.hpp>

// To be compatible with VSC Express edition that does not have tr1

#include <unordered_map>

namespace Mantid {
namespace Algorithms {
/**
 *
This algorithm is used to select/deselect detectors in a *.cal file.


 @author Laurent Chapon, Pascal Manuel ISIS Facility, Rutherford Appleton
Laboratory
 @date 06/07/2009
 */
class MANTID_ALGORITHMS_DLL MaskDetectorsIf final : public API::Algorithm {
public:
  /// Algorithm's name for identification overriding a virtual method
  const std::string name() const override { return "MaskDetectorsIf"; }
  /// Summary of algorithms purpose
  const std::string summary() const override {
    return "Masks detectors depending on the values in the input workspace.";
  }

  /// Algorithm's version for identification overriding a virtual method
  int version() const override { return 1; }
  const std::vector<std::string> seeAlso() const override { return {"MaskDetectors"}; }
  /// Algorithm's category for identification overriding a virtual method
  const std::string category() const override { return "Diffraction\\Masking;Transforms\\Masking"; }

private:
  /// Typedef for det to value map
  using udet2valuem = std::unordered_map<detid_t, bool>;
  /// A map from detid to selection
  udet2valuem m_umap;
  /// Whether select is on or off
  bool m_select_on = false;
  /// The start index parameter
  int m_start_ix = 0;
  /// The end index parameter
  int m_end_ix = -1;
  /// The Value parameter
  double m_value = 0.0;
  /// The input workspace
  API::MatrixWorkspace_const_sptr m_inputW;
  /// A comparator function
  boost::function<bool(double, double)> m_compar_f;
  void outputToWorkspace();
  void retrieveProperties();
  void createNewCalFile();
  void init() override;
  void exec() override;
  std::map<std::string, std::string> validateInputs() override;
};

} // namespace Algorithms
} // namespace Mantid
