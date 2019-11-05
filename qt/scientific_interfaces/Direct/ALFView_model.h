// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2014 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#ifndef MANTIDQT_CUSTOMINTERFACES_ALFVIEWMODEL_H_
#define MANTIDQT_CUSTOMINTERFACES_ALFVIEWMODEL_H_

#include "BaseInstrumentModel.h"
#include "MantidAPI/CompositeFunction.h"
#include "MantidAPI/FunctionFactory.h"

#include <map>
#include <string>

namespace MantidQt {
namespace CustomInterfaces {

class ALFView_model : public BaseInstrumentModel {

public:
  ALFView_model();
  virtual ~ALFView_model(){};
  std::pair<int, std::string> loadData(const std::string &name) override;
  std::map<std::string, bool> isDataValid();
  void transformData();
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

#endif /* MANTIDQT_CUSTOMINTERFACES_ALFVIEWMODEL_H_ */
