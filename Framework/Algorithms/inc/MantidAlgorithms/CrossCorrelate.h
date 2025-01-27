// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2008 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

//----------------------------------------------------------------------
// Includes
//----------------------------------------------------------------------
#include "MantidAPI/Algorithm.h"
#include "MantidAPI/SpectraDetectorTypes.h"
#include "MantidAlgorithms/DllConfig.h"

namespace Mantid {
namespace Algorithms {
/** Compute the cross correlation function for a range of spectra with respect
 to a reference spectrum.
 * This is use in powder diffraction experiments when trying to estimate the
 offset of one spectra
 * with respect to another one. The spectra are converted in d-spacing and then
 interpolate
 * on the X-axis of the reference. The cross correlation function is computed in
 the range [-N/2,N/2]
 * where N is the number of points.

    Required Properties:
    <UL>
    <LI> InputWorkspace - The name of the workspace to take as input. </LI>
    <LI> OutputWorkspace - The name under which to store the output workspace.
 </LI>
    <LI> ReferenceSpectra - The spectra number against which cross-correlation
 function is computed.</LI>
    <LI> Spectra_min  - Lower bound of the spectra range for which
 cross-correlation is computed.</LI>
    <LI> Spectra_max - Upper bound of the spectra range for which
 cross-correlation is computed.</LI>
    </UL>

    @author Laurent C Chapon, ISIS Facility Rutherford Appleton Laboratory
    @date 15/12/2008
*/
class MANTID_ALGORITHMS_DLL CrossCorrelate final : public API::Algorithm {
public:
  /// (Empty) Constructor
  CrossCorrelate() : API::Algorithm() {}
  /// Virtual destructor
  ~CrossCorrelate() override = default;
  /// Algorithm's name
  const std::string name() const override { return "CrossCorrelate"; }
  /// Summary of algorithms purpose
  const std::string summary() const override {
    return "Cross-correlates a range of spectra against one reference spectra "
           "in the same workspace.";
  }

  /// Algorithm's version
  int version() const override { return (1); }
  const std::vector<std::string> seeAlso() const override { return {"GetDetectorOffsets"}; }
  /// Algorithm's category for identification
  const std::string category() const override { return "Arithmetic"; }
  /// Input validation
  std::map<std::string, std::string> validateInputs() override;

private:
  /// Initialisation code
  void init() override;
  /// Execution code
  void exec() override;
  /// Spectra to index map
  spec2index_map index_map;
  /// Iterator for the spectra to index map
  spec2index_map::iterator index_map_it;

  /// Progress reporting
  std::unique_ptr<API::Progress> m_progress = nullptr;
};

// Functor for vector sum

} // namespace Algorithms
} // namespace Mantid
