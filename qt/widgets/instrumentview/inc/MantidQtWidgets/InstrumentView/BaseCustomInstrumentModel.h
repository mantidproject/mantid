// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2014 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#ifndef MANTIDQT_INSTRUMENTVIEW_BASECUSTOMINSTRUMENTMODEL_H_
#define MANTIDQT_INSTRUMENTVIEW_BASECUSTOMINSTRUMENTMODEL_H_

#include "DllOption.h"

#include <map>
#include <string>

namespace MantidQt {
namespace MantidWidgets {

class EXPORT_OPT_MANTIDQT_INSTRUMENTVIEW BaseCustomInstrumentModel {

public:
  BaseCustomInstrumentModel();
  ~BaseCustomInstrumentModel(){};
  virtual void loadEmptyInstrument();
  virtual std::pair<int, std::string> loadData(const std::string &name);
  void setCurrentRun(int &run) { m_currentRun = run; };
  int getCurrentRun() { return m_currentRun; };
  void rename();
  void remove();
  std::string dataFileName();
  int currentRun();
  bool isErrorCode(const int run);
  const std::string getInstrument() { return m_instrumentName; };

protected:
  int m_currentRun;
  std::string m_tmpName;
  std::string m_instrumentName;
  std::string m_wsName;
};

} // namespace MantidWidgets
} // namespace MantidQt

#endif /* MANTIDQT_INSTRUMENTVIEW_BASEINSTRUMENTMODEL_H_ */
