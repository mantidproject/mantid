// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2025 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include "MantidAPI/Algorithm.h"
#include "MantidAPI/MultipleExperimentInfos.h"
#include "MantidDataHandling/DllConfig.h"

namespace Mantid {} // namespace Mantid

namespace Mantid {
namespace DataHandling {

class MANTID_DATAHANDLING_DLL TranslateSampleShape final : public API::Algorithm {
public:
  /// Algorithm's name for identification
  const std::string name() const override { return "TranslateSampleShape"; };
  /// Summary of algorithms purpose
  const std::string summary() const override { return "Redefine the initial position of the sample shape mesh"; }

  /// Algorithm's version for identification
  int version() const override { return 1; };
  /// Algorithm's category for identification
  const std::string category() const override { return "DataHandling\\Instrument"; }

private:
  /// Initialise the properties
  void init() override;
  /// Run the algorithm
  void exec() override;
  bool checkIsValidShape(const API::ExperimentInfo_sptr &ei, std::string &shapeXML, bool &isMeshShape);
};

} // namespace DataHandling
} // namespace Mantid
