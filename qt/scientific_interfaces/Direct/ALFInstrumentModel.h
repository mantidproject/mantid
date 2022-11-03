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
  virtual std::pair<int, std::string> loadData(const std::string &name) = 0;
  virtual void setCurrentRun(int &run) = 0;
  virtual int getCurrentRun() const = 0;
  virtual void rename() = 0;
  virtual void remove() = 0;
  virtual std::string dataFileName() = 0;
  virtual int currentRun() = 0;
  virtual bool isErrorCode(const int run) const = 0;
  virtual std::map<std::string, bool> isDataValid() = 0;
  virtual const std::string getInstrument() const = 0;
  virtual const std::string getTmpName() const = 0;
  virtual const std::string getWSName() const = 0;
  virtual std::string WSName() = 0;
  virtual void transformData() = 0;
  virtual void storeSingleTube(const std::string &name) = 0;
  virtual void extractSingleTube() = 0;
  virtual void averageTube() = 0;
  virtual bool hasTubeBeenExtracted() const = 0;
  virtual int numberOfTubesInAverage() const = 0;
};

class MANTIDQT_DIRECT_DLL ALFInstrumentModel : public IALFInstrumentModel {

public:
  ALFInstrumentModel();
  std::pair<int, std::string> loadData(const std::string &name) override;
  void setCurrentRun(int &run) override { m_currentRun = run; };
  int getCurrentRun() const override { return m_currentRun; };
  void rename() override;
  void remove() override;
  std::string dataFileName() override;
  int currentRun() override;
  bool isErrorCode(const int run) const override;
  const std::string getInstrument() const override { return m_instrumentName; };
  const std::string getTmpName() const override { return m_tmpName; };
  const std::string getWSName() const override { return m_wsName; };
  void transformData() override;
  void extractSingleTube() override;
  void averageTube() override;
  void storeSingleTube(const std::string &name) override;
  std::optional<double> xConversionFactor(Mantid::API::MatrixWorkspace_const_sptr workspace) const;
  std::string WSName() override;
  std::map<std::string, bool> isDataValid() override;
  bool hasTubeBeenExtracted() const override;
  int numberOfTubesInAverage() const override;

protected:
  int m_numberOfTubesInAverage;
  int m_currentRun;
  std::string m_tmpName;
  std::string m_instrumentName;
  std::string m_wsName;
};

} // namespace CustomInterfaces
} // namespace MantidQt
