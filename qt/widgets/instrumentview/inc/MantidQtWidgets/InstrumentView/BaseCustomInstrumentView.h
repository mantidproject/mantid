// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2014 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include "DllOption.h"
#include "MantidQtWidgets/Common/FileFinderWidget.h"
#include "MantidQtWidgets/Common/ObserverPattern.h"
#include "MantidQtWidgets/InstrumentView/InstrumentWidget.h"

#include <QObject>
#include <QPushButton>
#include <QSplitter>
#include <QString>
#include <string>

namespace MantidQt {
namespace MantidWidgets {

class BaseCustomInstrumentPresenter;

class EXPORT_OPT_MANTIDQT_INSTRUMENTVIEW IBaseCustomInstrumentView {
public:
  IBaseCustomInstrumentView(){};
  virtual ~IBaseCustomInstrumentView(){};
  virtual void subscribePresenter(MantidQt::MantidWidgets::BaseCustomInstrumentPresenter *presenter) = 0;
  virtual QWidget *generateLoadWidget() = 0;
  virtual std::string getFile() = 0;
  virtual void setRunQuietly(const std::string &runNumber) = 0;
  virtual void warningBox(const std::string &message) = 0;
  virtual void setInstrumentWidget(MantidWidgets::InstrumentWidget *instrument) = 0;
  virtual MantidWidgets::InstrumentWidget *getInstrumentView() = 0;
  virtual void setUpInstrument(const std::string &fileName,
                               std::vector<std::function<bool(std::map<std::string, bool>)>> &instrument) = 0;
  virtual void addObserver(std::tuple<std::string, Observer *> &listener) = 0;
  virtual void setupHelp() = 0;
};

class EXPORT_OPT_MANTIDQT_INSTRUMENTVIEW BaseCustomInstrumentView : public QSplitter,
                                                                    virtual IBaseCustomInstrumentView {
  Q_OBJECT

public:
  explicit BaseCustomInstrumentView(const std::string &instrument, QWidget *parent = nullptr);
  void subscribePresenter(MantidQt::MantidWidgets::BaseCustomInstrumentPresenter *presenter) override;
  QWidget *generateLoadWidget() override;
  std::string getFile() override;
  void setRunQuietly(const std::string &runNumber) override;
  void warningBox(const std::string &message) override;
  void setInstrumentWidget(MantidWidgets::InstrumentWidget *instrument) override { m_instrumentWidget = instrument; };
  MantidWidgets::InstrumentWidget *getInstrumentView() override { return m_instrumentWidget; };
  virtual void setUpInstrument(const std::string &fileName,
                               std::vector<std::function<bool(std::map<std::string, bool>)>> &instrument) override;
  virtual void addObserver(std::tuple<std::string, Observer *> &listener) override { (void)listener; };
  void setupHelp() override;

public slots:
  void fileLoaded();
  void openHelp();

protected:
  std::string m_helpPage;

private:
  void warningBox(const QString &message);

  API::FileFinderWidget *m_files;
  QString m_instrument;
  MantidWidgets::InstrumentWidget *m_instrumentWidget;
  QPushButton *m_help;
  MantidQt::MantidWidgets::BaseCustomInstrumentPresenter *m_presenter;
};
} // namespace MantidWidgets
} // namespace MantidQt
