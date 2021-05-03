// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include <map>
#include <string>

class IFQFitObserver {
public:
  virtual ~IFQFitObserver() = default;
  virtual void updateAvailableFunctions(const std::map<std::string, std::string> &functionInitialisationStrings) = 0;
};
