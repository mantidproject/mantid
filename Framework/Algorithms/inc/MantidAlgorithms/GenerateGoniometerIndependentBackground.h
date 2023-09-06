// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2023 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include "MantidAPI/Algorithm.h"
#include "MantidAlgorithms/DllConfig.h"
#include "MantidDataObjects/EventList.h"

namespace Mantid {
namespace Algorithms {

/** GenerateGoniometerIndependentBackground : TODO: DESCRIPTION
 */
class MANTID_ALGORITHMS_DLL GenerateGoniometerIndependentBackground : public API::Algorithm {
public:
  const std::string name() const override;
  int version() const override;
  const std::string category() const override;
  const std::string summary() const override;
  std::map<std::string, std::string> validateInputs() override;

private:
  void init() override;
  void exec() override;
  template <class T>
  void filterAndAddEvents(const std::vector<T> &events, Mantid::DataObjects::EventList &outSpec,
                          const Mantid::API::EventType eventType, const double start, const double end);
};
} // namespace Algorithms
} // namespace Mantid
