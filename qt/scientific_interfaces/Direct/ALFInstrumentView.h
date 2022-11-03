// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2014 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include "DllConfig.h"
#include "MantidQtWidgets/Common/FileFinderWidget.h"
#include "MantidQtWidgets/Common/ObserverPattern.h"
#include "MantidQtWidgets/InstrumentView/InstrumentWidget.h"

#include <QObject>
#include <QPushButton>
#include <QSplitter>
#include <QString>
#include <string>

namespace MantidQt {
namespace CustomInterfaces {

class ALFInstrumentPresenter;

class MANTIDQT_DIRECT_DLL IALFInstrumentView {
public:
  IALFInstrumentView(){};
  virtual void subscribePresenter(ALFInstrumentPresenter *presenter) = 0;
  virtual QWidget *generateLoadWidget() = 0;
  virtual std::string getFile() = 0;
  virtual void setRunQuietly(const std::string &runNumber) = 0;
  virtual void warningBox(const std::string &message) = 0;
  virtual void setInstrumentWidget(MantidWidgets::InstrumentWidget *instrument) = 0;
  virtual MantidWidgets::InstrumentWidget *getInstrumentView() = 0;
  virtual void setUpInstrument(const std::string &fileName) = 0;
  virtual void setupHelp() = 0;
};

class MANTIDQT_DIRECT_DLL ALFInstrumentView : public QWidget, public IALFInstrumentView {
  Q_OBJECT

public:
  explicit ALFInstrumentView(const std::string &instrument, QWidget *parent = nullptr);
  void subscribePresenter(ALFInstrumentPresenter *presenter) override;
  QWidget *generateLoadWidget() override;
  std::string getFile() override;
  void setRunQuietly(const std::string &runNumber) override;
  void warningBox(const std::string &message) override;
  void setInstrumentWidget(MantidWidgets::InstrumentWidget *instrument) override { m_instrumentWidget = instrument; };
  MantidWidgets::InstrumentWidget *getInstrumentView() override { return m_instrumentWidget; };
  void setUpInstrument(const std::string &fileName) override;
  void setupHelp() override;

public slots:
  void fileLoaded();
  void openHelp();
  void selectWholeTube();
  void extractSingleTube();
  void averageTube();

protected:
  std::string m_helpPage;
  ALFInstrumentPresenter *m_presenter;

private:
  void warningBox(const QString &message);

  QAction *m_extractAction;
  QAction *m_averageAction;
  API::FileFinderWidget *m_files;
  QString m_instrument;
  MantidWidgets::InstrumentWidget *m_instrumentWidget;
  QPushButton *m_help;
};
} // namespace CustomInterfaces
} // namespace MantidQt
