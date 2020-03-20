// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2015 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include "MantidAPI/Algorithm.h"
#include "MantidAPI/DataProcessorAlgorithm.h"
#include "MantidAlgorithms/DllConfig.h"
#include <boost/optional.hpp>

namespace Mantid {
namespace Algorithms {

/** CalculateSlits
 */

class MANTID_ALGORITHMS_DLL CalculateSlits
    : public API::DataProcessorAlgorithm {
public:
  CalculateSlits();
  ~CalculateSlits() override;

  const std::string name() const override;
  int version() const override;
  const std::vector<std::string> seeAlso() const override {
    return {"NRCalculateSlitResolution"};
  }
  const std::string category() const override;
  const std::string summary() const override;

private:
  void init() override;
  void exec() override;
};

} // namespace Algorithms
} // namespace Mantid
