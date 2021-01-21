// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2016 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include "MantidAPI/Algorithm.h"
#include "MantidDataHandling/DllConfig.h"
namespace Mantid {
namespace DataHandling {

/**
  Set properties of the beam.
*/
class MANTID_DATAHANDLING_DLL SetBeam final : public API::Algorithm {
public:
  const std::string name() const override final;
  int version() const override final;
  const std::vector<std::string> seeAlso() const override { return {"SetSample"}; }
  const std::string category() const override final;
  const std::string summary() const override final;

private:
  std::map<std::string, std::string> validateInputs() override final;
  void init() override final;
  void exec() override final;
};

} // namespace DataHandling
} // namespace Mantid
