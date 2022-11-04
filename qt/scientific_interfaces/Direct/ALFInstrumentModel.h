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
  virtual std::optional<std::string> loadAndTransform(std::string const &filename) = 0;

  virtual std::string instrumentName() const = 0;
  virtual std::string loadedWsName() const = 0;
  virtual std::string extractedWsName() const = 0;
  virtual std::size_t runNumber() const = 0;

  virtual void extractSingleTube() = 0;
  virtual void averageTube() = 0;

  virtual bool showAverageTubeOption() const = 0;
};

class MANTIDQT_DIRECT_DLL ALFInstrumentModel final : public IALFInstrumentModel {

public:
  ALFInstrumentModel();

  std::optional<std::string> loadAndTransform(const std::string &filename) override;

  inline std::string instrumentName() const noexcept override { return "ALF"; }
  inline std::string loadedWsName() const noexcept override { return "ALFData"; };
  std::string extractedWsName() const override;
  std::size_t runNumber() const override;

  void extractSingleTube() override;
  void averageTube() override;

  bool showAverageTubeOption() const override;

public: // Methods for testing purposes
  inline std::size_t numberOfTubesInAverage() const noexcept { return m_numberOfTubesInAverage; }

private:
  Mantid::API::MatrixWorkspace_sptr retrieveSingleTube();

  std::size_t m_numberOfTubesInAverage;
};

} // namespace CustomInterfaces
} // namespace MantidQt
