// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2014 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#ifndef MANTIDQT_CUSTOMINTERFACES_BASEINSTRUMENTMODEL_H_
#define MANTIDQT_CUSTOMINTERFACES_BASEINSTRUMENTMODEL_H_
#include <map>
#include <string>

namespace MantidQt {
namespace CustomInterfaces {

class BaseInstrumentModel {


public:
  BaseInstrumentModel();
  ~BaseInstrumentModel(){};
  virtual void loadEmptyInstrument();
  virtual std::pair<int, std::string> loadData(const std::string &name);
  void setCurrentRun(int &run) { m_currentRun = run; };
  int getCurrentRun() { return m_currentRun; };
  void rename();
  void remove();
  std::string dataFileName();
  int currentRun();
  bool isErrorCode(const int run);
  std::string getInstrument();

  void setTmpName(const std::string &name) { m_tmpName = name; };
  void setInstrumentName(const std::string &name) { m_instrumentName = name; };
  void setWSName(const std::string &name) { m_wsName = name; };
  const std::string getTmpName() { return m_tmpName; };
  const std::string getInstrumentName() { return m_instrumentName; };
  const std::string getWSName() { return m_wsName; };


private:
  int m_currentRun;
  std::string m_tmpName;
  std::string m_instrumentName;
  std::string m_wsName;
};

} // namespace CustomInterfaces
} // namespace MantidQt

#endif /* MANTIDQT_CUSTOMINTERFACES_BASEINSTRUMENTMODEL_H_ */
