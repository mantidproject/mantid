// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2014 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#ifndef MANTIDQT_INSTRUMENTVIEW_BASECUSTOMINSTRUMENTVIEW_H_
#define MANTIDQT_INSTRUMENTVIEW_BASECUSTOMINSTRUMENTVIEW_H_

#include "DllOption.h"
#include "MantidQtWidgets/Common/MWRunFiles.h"
#include "MantidQtWidgets/Common/ObserverPattern.h"
#include "MantidQtWidgets/InstrumentView/InstrumentWidget.h"

#include <QObject>
#include <QPushButton>
#include <QSplitter>
#include <QString>
#include <string>

namespace MantidQt {
namespace MantidWidgets {

class EXPORT_OPT_MANTIDQT_INSTRUMENTVIEW IBaseCustomInstrumentView
{
public:
  virtual std::string getFile()=0;
  virtual void setRunQuietly(const std::string &runNumber)=0;
  virtual void observeLoadRun(Observer *listener)=0;
  virtual void warningBox(const std::string &message)=0;
  virtual void setInstrumentWidget(MantidWidgets::InstrumentWidget *instrument)=0;
  virtual MantidWidgets::InstrumentWidget *getInstrumentView()=0;
  virtual void
  setUpInstrument(const std::string &fileName,
                  std::vector<std::function<bool(std::map<std::string, bool>)>>
                      &instrument)=0;
  virtual void addObserver(std::tuple<std::string, Observer *> &listener)=0;
  virtual void setupInstrumentAnalysisSplitters(QWidget *analysis)=0;
  virtual void setupHelp()=0;
};

class EXPORT_OPT_MANTIDQT_INSTRUMENTVIEW BaseCustomInstrumentView
    : public QSplitter, IBaseCustomInstrumentView {
  Q_OBJECT

public:
  explicit BaseCustomInstrumentView(const std::string &instrument,
                                    QWidget *parent = nullptr);
  std::string getFile() override;
  void setRunQuietly(const std::string &runNumber) override;
  void observeLoadRun(Observer *listener) override {
    m_loadRunObservable->attach(listener);
  };
  void warningBox(const std::string &message) override;
  void setInstrumentWidget(MantidWidgets::InstrumentWidget *instrument) override{
    m_instrumentWidget = instrument;
  };
  MantidWidgets::InstrumentWidget *getInstrumentView()override {
    return m_instrumentWidget;
  };
  virtual void
  setUpInstrument(const std::string &fileName,
                  std::vector<std::function<bool(std::map<std::string, bool>)>>
                      &instrument) override;
  virtual void addObserver(std::tuple<std::string, Observer *> &listener) override {
    (void)listener;
  };
  void setupInstrumentAnalysisSplitters(QWidget *analysis) override;
  void setupHelp() override;

public slots:
  void fileLoaded();
  void openHelp();

protected:
  std::string m_helpPage;

private:
  QWidget *generateLoadWidget();
  void warningBox(const QString &message);
  Observable *m_loadRunObservable;
  API::MWRunFiles *m_files;
  QString m_instrument;
  MantidWidgets::InstrumentWidget *m_instrumentWidget;
  QPushButton *m_help;
};
} // namespace MantidWidgets
} // namespace MantidQt

#endif /* MANTIDQT_INSTRUMENTVIEW_BASECUSTOMINSTRUMENTVIEW_H_ */
