// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include "MantidAPI/Algorithm.h"
#include "MantidAlgorithms/DllConfig.h"
#include "MantidGeometry/Instrument.h"

namespace Mantid {
namespace Algorithms {

/** Creates a new GroupingWorkspace using an instrument from one of:
 *  an input workspace,
 *  an instrument name,
 *  or an instrument IDF file.
 *
 *  Optionally uses bank names to create the groups.
 */
class MANTID_ALGORITHMS_DLL CreateGroupingWorkspace final : public API::Algorithm {
public:
  /// Algorithm's name for identification
  const std::string name() const override;
  /// Summary of algorithms purpose
  const std::string summary() const override {
    return "Creates a new GroupingWorkspace using an instrument from one of: "
           "an input workspace, an instrument name, or an instrument IDF "
           "file.\nOptionally uses bank names to create the groups.";
  }

  /// Algorithm's version for identification
  int version() const override;
  const std::vector<std::string> seeAlso() const override {
    return {"DiffractionFocussing", "LoadCalFile", "CreateGroupingByComponent"};
  }
  /// Algorithm's category for identification
  const std::string category() const override;

private:
  /// Initialise the properties
  void init() override;
  /// Cross-check properties with each other @see IAlgorithm::validateInputs
  std::map<std::string, std::string> validateInputs() override;
  /// Run the algorithm
  void exec() override;

  Mantid::Geometry::Instrument_const_sptr getInstrument();
};

} // namespace Algorithms
} // namespace Mantid
