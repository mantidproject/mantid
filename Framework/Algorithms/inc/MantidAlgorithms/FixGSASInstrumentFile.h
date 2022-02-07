// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2013 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include "MantidAPI/Algorithm.h"
#include "MantidAlgorithms/DllConfig.h"

namespace Mantid {
namespace Algorithms {

/** FixGSASInstrumentFile : TODO: DESCRIPTION
 */
class MANTID_ALGORITHMS_DLL FixGSASInstrumentFile : public API::Algorithm {
public:
  /// Algorithm's name for identification overriding a virtual method
  const std::string name() const override { return "FixGSASInstrumentFile"; }
  /// Summary of algorithms purpose
  const std::string summary() const override { return "Fix format error in an GSAS instrument file."; }

  /// Algorithm's version for identification overriding a virtual method
  int version() const override { return 1; }
  const std::vector<std::string> seeAlso() const override { return {"LoadGSASInstrumentFile"}; }

  /// Algorithm's category for identification overriding a virtual method
  const std::string category() const override { return "Diffraction\\DataHandling"; }

private:
  /// Implement abstract Algorithm methods
  void init() override;
  /// Implement abstract Algorithm methods
  void exec() override;
};

} // namespace Algorithms
} // namespace Mantid
