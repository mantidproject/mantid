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
  int currentRun();
  bool isErrorCode(const int run);
  std::string getInstrument();
};

} // namespace CustomInterfaces
} // namespace MantidQt

#endif /* MANTIDQT_CUSTOMINTERFACES_ALFVIEWMODEL_H_ */
