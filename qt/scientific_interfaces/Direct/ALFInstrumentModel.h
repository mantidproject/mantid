// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2014 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include "DllConfig.h"
#include "MantidAPI/MatrixWorkspace_fwd.h"

#include <map>
#include <optional>
#include <string>

namespace MantidQt {
namespace CustomInterfaces {

class MANTIDQT_DIRECT_DLL IALFInstrumentModel {

public:
  virtual std::optional<std::string> loadData(std::string const &filename) = 0;
  virtual void extractSingleTube() = 0;
  virtual void averageTube() = 0;
  virtual void setCurrentRun(int &run) = 0;
  virtual int runNumber() const = 0;
  virtual std::string instrumentName() const = 0;
  virtual const std::string getWSName() const = 0;
  virtual std::string extractedWsName() const = 0;
  virtual bool hasTubeBeenExtracted() const = 0;
  virtual int numberOfTubesInAverage() const = 0;
};

class MANTIDQT_DIRECT_DLL ALFInstrumentModel : public IALFInstrumentModel {

public:
  ALFInstrumentModel();
  std::optional<std::string> loadData(const std::string &filename) override final;
  void extractSingleTube() override final;
  void averageTube() override final;
  void setCurrentRun(int &run) override final { m_currentRun = run; };
  int runNumber() const override final { return m_currentRun; }
  std::string instrumentName() const override final { return m_instrumentName; }
  const std::string getWSName() const override final { return m_wsName; };
  std::string extractedWsName() const override final;
  bool hasTubeBeenExtracted() const override final;
  int numberOfTubesInAverage() const override final;

private:
  Mantid::API::MatrixWorkspace_sptr retrieveSingleTube();

  int m_numberOfTubesInAverage;
  int m_currentRun;
  std::string m_instrumentName;
  std::string m_wsName;
};

} // namespace CustomInterfaces
} // namespace MantidQt
