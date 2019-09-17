// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2014 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#ifndef MANTIDQT_CUSTOMINTERFACES_ALFVIEWMODEL_H_
#define MANTIDQT_CUSTOMINTERFACES_ALFVIEWMODEL_H_
#include <string>

namespace MantidQt {
namespace CustomInterfaces {
namespace Direct {

void loadEmptyInstrument();
int loadData(const std::string &name);
std::pair<bool,bool> isDataValid();
void transformData();
void rename();
void remove();
int currentRun();
} // namespace Direct
} // namespace CustomInterfaces
} // namespace MantidQt

#endif /* MANTIDQT_CUSTOMINTERFACES_ALFVIEWMODEL_H_ */
