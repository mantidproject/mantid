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
#include "MantidLiveData/DllConfig.h"

namespace Mantid {
namespace LiveData {
/**
    Simulates ISIS histogram DAE. It runs continuously until canceled and
    listens to port 6789 for ISIS DAE commands.

    Data is generated starting at 10000 microseconds Time of flight, and each
    bin requested covers 100 microseconds.
    The algorithm silently defines three additional spectra with numbers
    NSpectra+1, NSpectra+2 and NSpectra+3 in a
    different time regime (they have different binning to the rest of the
    spectra).
*/
class MANTID_LIVEDATA_DLL FakeISISHistoDAE final : public API::Algorithm {
public:
  /// Algorithm's name for identification overriding a virtual method
  const std::string name() const override { return "FakeISISHistoDAE"; }
  /// Algorithm's version for identification overriding a virtual method
  int version() const override { return 1; }
  /// Algorithm's category for identification overriding a virtual method
  const std::string category() const override { return "DataHandling\\DataAcquisition"; }
  const std::vector<std::string> seeAlso() const override { return {"FakeISISEventDAE"}; }
  /// Summary of algorithms purpose
  const std::string summary() const override { return "Simulates ISIS histogram DAE."; }

private:
  // Implement abstract Algorithm methods
  void init() override;
  void exec() override;
};

} // namespace LiveData
} // namespace Mantid
