// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2022 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include "DllConfig.h"

#include <optional>
#include <string>

#include <QAction>
#include <QPushButton>
#include <QSplitter>
#include <QString>
#include <QWidget>

namespace MantidQt {

namespace API {
class FileFinderWidget;
}

namespace MantidWidgets {
class InstrumentWidget;
}

namespace CustomInterfaces {

class ALFInstrumentPresenter;

class MANTIDQT_DIRECT_DLL IALFInstrumentView {
public:
  virtual void setUpInstrument(const std::string &fileName) = 0;

  virtual QWidget *generateLoadWidget() = 0;
  virtual MantidWidgets::InstrumentWidget *getInstrumentView() = 0;

  virtual void subscribePresenter(ALFInstrumentPresenter *presenter) = 0;

  virtual std::optional<std::string> getFile() = 0;
  virtual void setRunQuietly(const std::string &runNumber) = 0;

  virtual void warningBox(const std::string &message) = 0;
};

class MANTIDQT_DIRECT_DLL ALFInstrumentView : public QWidget, public IALFInstrumentView {
  Q_OBJECT

public:
  explicit ALFInstrumentView(QWidget *parent = nullptr);

  void setUpInstrument(const std::string &fileName) override final;

  QWidget *generateLoadWidget() override final;
  MantidWidgets::InstrumentWidget *getInstrumentView() override final { return m_instrumentWidget; };

  void subscribePresenter(ALFInstrumentPresenter *presenter) override final;

  std::optional<std::string> getFile() override final;
  void setRunQuietly(const std::string &runNumber) override final;

  void warningBox(const std::string &message) override final;

private slots:
  void fileLoaded();
  void selectWholeTube();
  void extractSingleTube();
  void averageTube();

private:
  API::FileFinderWidget *m_files;
  MantidWidgets::InstrumentWidget *m_instrumentWidget;
  QAction *m_extractAction;
  QAction *m_averageAction;

  ALFInstrumentPresenter *m_presenter;
};
} // namespace CustomInterfaces
} // namespace MantidQt
