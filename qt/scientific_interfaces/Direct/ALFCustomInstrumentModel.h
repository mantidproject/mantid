// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2014 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include "DllConfig.h"
#include "MantidAPI/CompositeFunction.h"
#include "MantidAPI/FunctionFactory.h"
#include "MantidQtWidgets/InstrumentView/BaseCustomInstrumentModel.h"

#include <map>
#include <string>

namespace MantidQt {
namespace CustomInterfaces {

class MANTIDQT_DIRECT_DLL IALFCustomInstrumentModel : public virtual MantidWidgets::IBaseCustomInstrumentModel {

public:
  IALFCustomInstrumentModel() {}
  virtual ~IALFCustomInstrumentModel(){};
  // virtual so we can patch them later
  virtual void loadAlg(const std::string &name) = 0;
  virtual void transformData() = 0;
  virtual std::map<std::string, bool> isDataValid() = 0;
  virtual void storeSingleTube(const std::string &name) = 0;
  virtual void averageTube() = 0;
  virtual bool hasTubeBeenExtracted(const std::string &name) = 0;
  virtual bool extractTubeCondition(std::map<std::string, bool> tabBools) = 0;
  virtual bool averageTubeCondition(std::map<std::string, bool> tabBools) = 0;
  virtual void extractSingleTube() = 0;
  virtual std::string WSName() = 0;
  virtual Mantid::API::CompositeFunction_sptr getDefaultFunction() = 0;
};

class DLLExport ALFCustomInstrumentModel : public IALFCustomInstrumentModel {

public:
  ALFCustomInstrumentModel();
  virtual ~ALFCustomInstrumentModel(){};
  // virtual so we can patch them later
  virtual void loadAlg(const std::string &name) override;
  virtual void transformData() override;
  std::pair<int, std::string> loadData(const std::string &name) override final;
  std::map<std::string, bool> isDataValid() override;
  void storeSingleTube(const std::string &name) override;
  void averageTube() override;
  bool hasTubeBeenExtracted(const std::string &name) override;
  bool extractTubeCondition(std::map<std::string, bool> tabBools) override;
  bool averageTubeCondition(std::map<std::string, bool> tabBools) override;
  void extractSingleTube() override;
  std::string WSName() override;
  Mantid::API::CompositeFunction_sptr getDefaultFunction() override;

  void loadEmptyInstrument() override { m_base->loadEmptyInstrument(); };
  void setCurrentRun(int &run) override { m_base->setCurrentRun(run); };
  int getCurrentRun() override { return m_base->getCurrentRun(); };
  void rename() override { m_base->rename(); };
  void remove() override { m_base->remove(); };
  std::string dataFileName() override { return m_base->dataFileName(); };
  int currentRun() override { return m_base->currentRun(); };
  bool isErrorCode(const int run) override { return m_base->isErrorCode(run); };
  const std::string getInstrument() override { return m_base->getInstrument(); };
  const std::string getTmpName() override { return m_base->getTmpName(); };
  const std::string getWSName() override { return m_base->getWSName(); };

private:
  int m_numberOfTubesInAverage;
  MantidWidgets::BaseCustomInstrumentModel *m_base;
};

} // namespace CustomInterfaces
} // namespace MantidQt
