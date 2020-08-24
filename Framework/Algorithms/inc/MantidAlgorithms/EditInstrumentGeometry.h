// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2011 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include "MantidAPI/Algorithm.h"
#include "MantidAlgorithms/DllConfig.h"

namespace Mantid {
namespace Algorithms {

/** EditInstrumentGeometry : TODO: DESCRIPTION

  @author
  @date 2011-08-22
*/
class MANTID_ALGORITHMS_DLL EditInstrumentGeometry : public API::Algorithm {
public:
  const std::string name() const override;
  /// Summary of algorithms purpose
  const std::string summary() const override {
    return "Adds a new instrument or edits an existing one; "
           "currently works in an overwrite mode only.";
  }

  /// Algorithm's category for identification overriding a virtual method
  const std::string category() const override;
  /// Algorithm's version for identification overriding a virtual method
  int version() const override;
  /// Validate the inputs that must be parallel
  std::map<std::string, std::string> validateInputs() override;

private:
  /// Initialise the properties
  void init() override;
  /// Run the algorithm
  void exec() override;
};

} // namespace Algorithms
} // namespace Mantid
