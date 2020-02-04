// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2014 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#ifndef MANTIDQT_CUSTOMINTERFACES_ALFCUSTOMINSTRUMENTMODEL_H_
#define MANTIDQT_CUSTOMINTERFACES_ALFCUSTOMINSTRUMENTMODEL_H_

#include "MantidAPI/CompositeFunction.h"
#include "MantidAPI/FunctionFactory.h"
#include "MantidQtWidgets/InstrumentView/BaseCustomInstrumentModel.h"

#include <map>
#include <string>

namespace MantidQt {
namespace CustomInterfaces {

class DLLExport ALFCustomInstrumentModel
    : public MantidWidgets::BaseCustomInstrumentModel {

public:
  ALFCustomInstrumentModel();
  virtual ~ALFCustomInstrumentModel(){};
  // virtual so we can patch them later
  virtual void loadAlg(const std::string &name);
  virtual void transformData();
  std::pair<int, std::string> loadData(const std::string &name) override;
  std::map<std::string, bool> isDataValid();
  void storeSingleTube(const std::string &name);
  void averageTube();
  bool hasTubeBeenExtracted(const std::string &name);
  bool extractTubeConditon(std::map<std::string, bool> tabBools);
  bool averageTubeConditon(std::map<std::string, bool> tabBools);
  void extractSingleTube();
  std::string WSName();
  Mantid::API::CompositeFunction_sptr getDefaultFunction();

private:
  int m_numberOfTubesInAverage;
};

} // namespace CustomInterfaces
} // namespace MantidQt

#endif /* MANTIDQT_CUSTOMINTERFACES_ALFCUSTOMINSTRUMENTMODEL_H_ */
