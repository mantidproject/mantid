// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2010 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include "MantidAPI/Algorithm.h"
#include "MantidCrystal/DllConfig.h"
#include "MantidGeometry/Instrument.h"

namespace Mantid {
namespace Crystal {

/**
Load incident spectrum and detector efficiency correction file.
 */

class MANTID_CRYSTAL_DLL LoadIsawSpectrum final : public API::Algorithm {
public:
  /// Algorithm's name for identification
  const std::string name() const override { return "LoadIsawSpectrum"; };
  /// Summary of algorithms purpose
  const std::string summary() const override {
    return "Load incident spectrum and detector efficiency correction file.";
  }

  /// Algorithm's version for identification
  int version() const override { return 1; };
  const std::vector<std::string> seeAlso() const override { return {"SaveIsawPeaks", "SaveIsawUB"}; }
  /// Algorithm's category for identification
  const std::string category() const override { return "Crystal\\DataHandling;DataHandling\\Text"; }

private:
  /// Initialise the properties
  void init() override;
  /// Run the algorithm
  void exec() override;

  double spectrumCalc(double TOF, int iSpec, const std::vector<std::vector<double>> &time,
                      const std::vector<std::vector<double>> &spectra, size_t id);
  void getInstrument3WaysInit(Algorithm *alg);
  Geometry::Instrument_const_sptr getInstrument3Ways(Algorithm *alg);
};

} // namespace Crystal
} // namespace Mantid
