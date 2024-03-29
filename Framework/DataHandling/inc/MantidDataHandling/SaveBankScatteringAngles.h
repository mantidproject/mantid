// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include "MantidAPI/Algorithm.h"
#include "MantidDataHandling/DllConfig.h"

namespace Mantid {
namespace DataHandling {

class MANTID_DATAHANDLING_DLL SaveBankScatteringAngles final : public API::Algorithm {
public:
  const std::string name() const override;

  const std::string summary() const override;

  int version() const override;

  const std::vector<std::string> seeAlso() const override;

  const std::string category() const override;

private:
  const static std::string PROP_FILENAME;
  const static std::string PROP_INPUT_WS;

  void init() override;

  void exec() override;

  std::map<std::string, std::string> validateInputs() override;
};

} // namespace DataHandling
} // namespace Mantid
