// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2014 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#ifndef MANTIDQT_CUSTOMINTERFACES_ALFVIEWMODEL_H_
#define MANTIDQT_CUSTOMINTERFACES_ALFVIEWMODEL_H_
#include <map>
#include <string>

namespace MantidQt {
namespace CustomInterfaces {

class ALFView_model {
public:
  void loadEmptyInstrument();
  std::pair<int, std::string> loadData(const std::string &name);
  std::map<std::string, bool> isDataValid();
  void transformData();
  void rename();
  void remove();
  std::string dataFileName();
  int currentRun();
  bool isErrorCode(const int run);
  std::string getInstrument();
  void storeSingleTube(const std::string &name);
  void averageTube(const int &oldTotalNumber, const std::string &name);
  bool hasTubeBeenExtracted(const std::string &name);
};

} // namespace CustomInterfaces
} // namespace MantidQt

#endif /* MANTIDQT_CUSTOMINTERFACES_ALFVIEWMODEL_H_ */
