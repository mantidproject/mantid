// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2022 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include "DllConfig.h"

#include "DetectorTube.h"
#include "MantidKernel/V3D.h"

#include <optional>
#include <string>

#include <QWidget>

namespace Mantid::Geometry {
class ComponentInfo;
}

namespace MantidQt {

namespace API {
class FileFinderWidget;
}

namespace MantidWidgets {
class IInstrumentActor;
}

namespace CustomInterfaces {

class ALFInstrumentWidget;
class IALFInstrumentPresenter;

class MANTIDQT_DIRECT_DLL IALFInstrumentView {

public:
  virtual void setUpInstrument(std::string const &fileName) = 0;

  virtual QWidget *generateSampleLoadWidget() = 0;
  virtual QWidget *generateVanadiumLoadWidget() = 0;
  virtual ALFInstrumentWidget *getInstrumentView() = 0;

  virtual void subscribePresenter(IALFInstrumentPresenter *presenter) = 0;

  virtual void loadSettings() = 0;
  virtual void saveSettings() = 0;

  virtual void disable(std::string const &reason) = 0;
  virtual void enable() = 0;

  virtual std::optional<std::string> getSampleFile() const = 0;
  virtual std::optional<std::string> getVanadiumFile() const = 0;

  virtual void setSampleRun(std::string const &runNumber) = 0;
  virtual void setVanadiumRun(std::string const &runNumber) = 0;

  virtual MantidWidgets::IInstrumentActor const &getInstrumentActor() const = 0;

  virtual std::vector<DetectorTube> getSelectedDetectors() const = 0;

  virtual void clearShapes() = 0;
  virtual void drawRectanglesAbove(std::vector<DetectorTube> const &tubes) = 0;

  virtual void displayWarning(std::string const &message) = 0;
};

class MANTIDQT_DIRECT_DLL ALFInstrumentView final : public QWidget, public IALFInstrumentView {
  Q_OBJECT

public:
  explicit ALFInstrumentView(QWidget *parent = nullptr);

  void setUpInstrument(std::string const &fileName) override;

  QWidget *generateSampleLoadWidget() override;
  QWidget *generateVanadiumLoadWidget() override;
  ALFInstrumentWidget *getInstrumentView() override { return m_instrumentWidget; };

  void subscribePresenter(IALFInstrumentPresenter *presenter) override;

  void loadSettings() override;
  void saveSettings() override;

  void disable(std::string const &reason) override;
  void enable() override;

  std::optional<std::string> getSampleFile() const override;
  std::optional<std::string> getVanadiumFile() const override;

  void setSampleRun(std::string const &runNumber) override;
  void setVanadiumRun(std::string const &runNumber) override;

  MantidWidgets::IInstrumentActor const &getInstrumentActor() const override;

  std::vector<DetectorTube> getSelectedDetectors() const override;

  void clearShapes() override;
  void drawRectanglesAbove(std::vector<DetectorTube> const &tubes) override;

  void displayWarning(std::string const &message) override;

private slots:
  void reconnectInstrumentActor();
  void reconnectSurface();
  void sampleLoaded();
  void vanadiumLoaded();
  void notifyInstrumentActorReset();
  void notifyShapeChanged();
  void selectWholeTube();
  void notifyWholeTubeSelected(size_t pickID);

private:
  QString m_settingsGroup;

  API::FileFinderWidget *m_sample;
  API::FileFinderWidget *m_vanadium;
  ALFInstrumentWidget *m_instrumentWidget;
  IALFInstrumentPresenter *m_presenter;
};
} // namespace CustomInterfaces
} // namespace MantidQt
