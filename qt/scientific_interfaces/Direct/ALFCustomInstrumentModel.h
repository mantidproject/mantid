// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2014 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include "MantidAPI/CompositeFunction.h"
#include "MantidAPI/FunctionFactory.h"
#include "MantidQtWidgets/InstrumentView/BaseCustomInstrumentModel.h"

#include <map>
#include <string>

namespace MantidQt {
namespace CustomInterfaces {

class DLLExport IALFCustomInstrumentModel
    : public virtual MantidWidgets::IBaseCustomInstrumentModel {

public:
  // virtual so we can patch them later
  virtual void loadAlg(const std::string &name)=0;
  virtual void transformData()=0;
  virtual std::map<std::string, bool> isDataValid()=0;
  virtual void storeSingleTube(const std::string &name)=0;
  virtual void averageTube()=0;
  virtual bool hasTubeBeenExtracted(const std::string &name)=0;
  virtual bool extractTubeConditon(std::map<std::string, bool> tabBools) = 0;
  virtual bool averageTubeConditon(std::map<std::string, bool> tabBools) = 0;
  virtual void extractSingleTube() = 0;
  virtual std::string WSName() = 0;
  virtual Mantid::API::CompositeFunction_sptr getDefaultFunction() = 0;
};


class DLLExport ALFCustomInstrumentModel
    : public virtual IALFCustomInstrumentModel, public
          MantidWidgets::BaseCustomInstrumentModel {

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
  bool extractTubeConditon(std::map<std::string, bool> tabBools) override;
  bool averageTubeConditon(std::map<std::string, bool> tabBools) override;
  void extractSingleTube() override;
  std::string WSName() override;
  Mantid::API::CompositeFunction_sptr getDefaultFunction() override;

private:
  int m_numberOfTubesInAverage;
};

} // namespace CustomInterfaces
} // namespace MantidQt
