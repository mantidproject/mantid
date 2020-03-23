// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2014 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include "DllOption.h"

#include <map>
#include <string>

namespace MantidQt {
namespace MantidWidgets {

class EXPORT_OPT_MANTIDQT_INSTRUMENTVIEW IBaseCustomInstrumentModel {

public:
  virtual void loadEmptyInstrument()=0;
  virtual std::pair<int, std::string> loadData(const std::string &name)=0;
  virtual void setCurrentRun(int &run)=0;
  virtual int getCurrentRun()=0;
  virtual void rename()=0;
  virtual void remove()=0;
  virtual std::string dataFileName()=0;
  virtual int currentRun()=0;
  virtual bool isErrorCode(const int run)=0;
  virtual const std::string getInstrument()=0;
  virtual const std::string getTmpName()=0;
  virtual const std::string getWSName()=0;
};


class EXPORT_OPT_MANTIDQT_INSTRUMENTVIEW BaseCustomInstrumentModel
    : public virtual IBaseCustomInstrumentModel{

public:
  BaseCustomInstrumentModel();
  BaseCustomInstrumentModel(const std::string &tmpName, const std::string instrumentName, const std::string wsName);
  ~BaseCustomInstrumentModel(){};
  void loadEmptyInstrument() override;
  std::pair<int, std::string> loadData(const std::string &name) override;
  void setCurrentRun(int &run) override{ m_currentRun = run; };
  int getCurrentRun() override{ return m_currentRun; };
  void rename() override;
  void remove() override;
  std::string dataFileName() override;
  int currentRun() override;
  bool isErrorCode(const int run) override;
  const std::string getInstrument() override { return m_instrumentName; };
  const std::string getTmpName() override { return m_tmpName; };
  const std::string getWSName() override { return m_wsName; };

protected:
  int m_currentRun;
  std::string m_tmpName;
  std::string m_instrumentName;
  std::string m_wsName;
};

} // namespace MantidWidgets
} // namespace MantidQt
