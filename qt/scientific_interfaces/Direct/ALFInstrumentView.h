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
  virtual ALFInstrumentWidget *getInstrumentView() = 0;

  virtual void subscribePresenter(IALFInstrumentPresenter *presenter) = 0;

  virtual std::optional<std::string> getSampleFile() const = 0;
  virtual void setSampleRun(std::string const &runNumber) = 0;

  virtual MantidWidgets::IInstrumentActor const &getInstrumentActor() const = 0;

  virtual std::vector<DetectorTube> getSelectedDetectors() const = 0;

  virtual void clearShapes() = 0;
  virtual void drawRectanglesAbove(std::vector<DetectorTube> const &tubes) = 0;

  virtual void warningBox(std::string const &message) = 0;
};

class MANTIDQT_DIRECT_DLL ALFInstrumentView final : public QWidget, public IALFInstrumentView {
  Q_OBJECT

public:
  explicit ALFInstrumentView(QWidget *parent = nullptr);

  void setUpInstrument(std::string const &fileName) override;

  QWidget *generateSampleLoadWidget() override;
  ALFInstrumentWidget *getInstrumentView() override { return m_instrumentWidget; };

  void subscribePresenter(IALFInstrumentPresenter *presenter) override;

  std::optional<std::string> getSampleFile() const override;
  void setSampleRun(std::string const &runNumber) override;

  MantidWidgets::IInstrumentActor const &getInstrumentActor() const override;

  std::vector<DetectorTube> getSelectedDetectors() const override;

  void clearShapes() override;
  void drawRectanglesAbove(std::vector<DetectorTube> const &tubes) override;

  void warningBox(std::string const &message) override;

private slots:
  void reconnectInstrumentActor();
  void sampleLoaded();
  void notifyInstrumentActorReset();
  void notifyShapeChanged();
  void selectWholeTube();
  void notifyWholeTubeSelected(size_t pickID);

private:
  API::FileFinderWidget *m_sample;
  ALFInstrumentWidget *m_instrumentWidget;
  IALFInstrumentPresenter *m_presenter;
};
} // namespace CustomInterfaces
} // namespace MantidQt
