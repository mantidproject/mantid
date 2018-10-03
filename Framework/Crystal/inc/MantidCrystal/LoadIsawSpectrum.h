// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2010 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#ifndef MANTID_CRYSTAL_LoadIsawSpectrum_H_
#define MANTID_CRYSTAL_LoadIsawSpectrum_H_

#include "MantidAPI/Algorithm.h"
#include "MantidGeometry/Instrument.h"

namespace Mantid {
namespace Crystal {

/**
Load incident spectrum and detector efficiency correction file.
 */

class DLLExport LoadIsawSpectrum : public API::Algorithm {
public:
  /// Algorithm's name for identification
  const std::string name() const override { return "LoadIsawSpectrum"; };
  /// Summary of algorithms purpose
  const std::string summary() const override {
    return "Load incident spectrum and detector efficiency correction file.";
  }

  /// Algorithm's version for identification
  int version() const override { return 1; };
  const std::vector<std::string> seeAlso() const override {
    return {"SaveIsawPeaks", "SaveIsawUB"};
  }
  /// Algorithm's category for identification
  const std::string category() const override {
    return "Crystal\\DataHandling;DataHandling\\Text";
  }

private:
  /// Initialise the properties
  void init() override;
  /// Run the algorithm
  void exec() override;

  double spectrumCalc(double TOF, int iSpec,
                      std::vector<std::vector<double>> time,
                      std::vector<std::vector<double>> spectra, size_t id);
  void getInstrument3WaysInit(Algorithm *alg);
  Geometry::Instrument_const_sptr getInstrument3Ways(Algorithm *alg);
};

} // namespace Crystal
} // namespace Mantid

#endif /* MANTID_CRYSTAL_LoadIsawSpectrum_H_ */
