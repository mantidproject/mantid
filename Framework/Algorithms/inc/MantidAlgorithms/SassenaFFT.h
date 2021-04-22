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
#include "MantidDataObjects/Workspace2D.h"
#include "MantidKernel/PhysicalConstants.h"

namespace Mantid {
namespace Algorithms {

/** Perform Fourier Transform of the Sassena Intermediate Scattering Function

  @author Jose Borreguero
  @date 2012-05-29
  */

class MANTID_ALGORITHMS_DLL SassenaFFT : public API::Algorithm {
public:
  /// Algorithm's name for identification overriding a virtual method
  const std::string name() const override { return "SassenaFFT"; }
  /// Summary of algorithms purpose
  const std::string summary() const override {
    return "Performs complex Fast Fourier Transform of intermediate scattering "
           "function";
  }

  /// Algorithm's version for identification overriding a virtual method
  int version() const override { return 1; }
  const std::vector<std::string> seeAlso() const override {
    return {"ExtractFFTSpectrum", "FFT", "FFTDerivative", "MaxEnt", "RealFFT", "FFTSmooth"};
  }
  /// Algorithm's category for identification overriding a virtual method
  const std::string category() const override { return "Arithmetic\\FFT"; }

protected:
  // Overridden Algorithm methods
  bool processGroups() override;

private:
  // Overridden Algorithm methods
  void init() override;
  void exec() override;
  bool checkGroups() override;
  static constexpr double m_T2ueV = 1000.0 / Mantid::PhysicalConstants::meVtoKelvin; // conversion factor from
                                                                                     // Kelvin to ueV
  const double m_ps2meV = 4.136; // conversion factor from picosecond to mili-eV

}; // class SassenaFFT

} // namespace Algorithms
} // namespace Mantid
