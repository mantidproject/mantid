// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2010 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

//----------------------------------------------------------------------
// Includes
//----------------------------------------------------------------------
#include "MantidAPI/Algorithm.h"
#include "MantidAlgorithms/DllConfig.h"

namespace Mantid {
namespace Algorithms {
/**
    This algorithm performs a Fast Fourier Transform on each spectra of the
   input workspace.
    It then takes a specified part of the FFT result (parameter "FFTPart") and
   places it in
    a new workspace, which will share the Y axis of the old one (ie,
   spectra-detector map) and
    have the unit label set to "Time / ns" (a non-functional unit).

    @author Michael Whitty, STFC ISIS
    @date 21/09/2010
*/
class MANTID_ALGORITHMS_DLL ExtractFFTSpectrum : public API::Algorithm {
public:
  /// Algorithm's name
  const std::string name() const override { return "ExtractFFTSpectrum"; }
  /// Summary of algorithms purpose
  const std::string summary() const override {
    return "This algorithm performs a Fast Fourier Transform on each spectrum "
           "in a workspace, and from the result takes the indicated spectrum "
           "and places it into the OutputWorkspace, so that you end up with "
           "one result spectrum for each input spectrum in the same workspace.";
  }

  /// Algorithm's version
  int version() const override { return (1); }
  const std::vector<std::string> seeAlso() const override {
    return {"FFT", "FFTDerivative", "MaxEnt", "RealFFT", "SassenaFFT", "FFTSmooth"};
  }
  /// Algorithm's category for identification
  const std::string category() const override { return "Arithmetic\\FFT"; }

private:
  /// Initialisation code
  void init() override;
  /// Execution code
  void exec() override;
};

} // namespace Algorithms
} // namespace Mantid
