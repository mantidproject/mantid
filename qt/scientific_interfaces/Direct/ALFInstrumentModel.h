// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2022 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include "DllConfig.h"
#include "MantidAPI/MatrixWorkspace_fwd.h"

#include <optional>
#include <string>

namespace MantidQt {
namespace CustomInterfaces {

class MANTIDQT_DIRECT_DLL IALFInstrumentModel {

public:
  virtual ~IALFInstrumentModel() = default;

  virtual std::optional<std::string> loadAndTransform(std::string const &filename) = 0;

  virtual std::string instrumentName() const = 0;
  virtual std::string loadedWsName() const = 0;
  virtual std::string extractedWsName() const = 0;
  virtual std::size_t runNumber() const = 0;

  virtual std::optional<double> extractSingleTube() = 0;
  virtual std::optional<double> averageTube(std::size_t const numberOfTubes) = 0;

  virtual bool checkDataIsExtracted() const = 0;
};

class MANTIDQT_DIRECT_DLL ALFInstrumentModel final : public IALFInstrumentModel {

public:
  ALFInstrumentModel();

  std::optional<std::string> loadAndTransform(const std::string &filename) override;

  inline std::string instrumentName() const noexcept override { return "ALF"; }
  inline std::string loadedWsName() const noexcept override { return "ALFData"; };
  std::string extractedWsName() const override;
  std::size_t runNumber() const override;

  std::optional<double> extractSingleTube() override;
  std::optional<double> averageTube(std::size_t const numberOfTubes) override;

  bool checkDataIsExtracted() const override;

private:
  Mantid::API::MatrixWorkspace_sptr retrieveSingleTube();
};

} // namespace CustomInterfaces
} // namespace MantidQt
