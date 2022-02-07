// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2017 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include "MantidAPI/DeprecatedAlgorithm.h"
#include "MantidAPI/DistributedAlgorithm.h"
#include "MantidAlgorithms/DllConfig.h"

namespace Mantid {
namespace Algorithms {

/** Mask specified detectors in an instrument.

  @author Simon Heybrock
  @date 2017
*/
class MANTID_ALGORITHMS_DLL MaskInstrument : public API::DistributedAlgorithm, public API::DeprecatedAlgorithm {
public:
  MaskInstrument();

  const std::string name() const override;
  int version() const override;
  const std::vector<std::string> seeAlso() const override { return {"MaskDetectors"}; }
  const std::string category() const override;
  const std::string summary() const override;

private:
  void init() override;
  void exec() override;
};

} // namespace Algorithms
} // namespace Mantid
