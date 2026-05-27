// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2022 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include "ALFInstrumentViewBase.h"
#include "DllConfig.h"
#include "IALFInstrumentView.h"

#include <qwidget.h>
#include <string>
#include <vector>

namespace MantidQt {

namespace MantidWidgets {
class IInstrumentActor;
}

namespace CustomInterfaces {

class ALFInstrumentWidget;

class MANTIDQT_DIRECT_DLL ALFInstrumentView final : public ALFInstrumentViewBase {
  Q_OBJECT

public:
  explicit ALFInstrumentView(QWidget *parent = nullptr);

  void setUpInstrument(std::string const &fileName) override;

  QWidget *getInstrumentView() override;

  MantidWidgets::IInstrumentActor const &getInstrumentActor() const override;

  std::vector<DetectorTube> getSelectedDetectors() const override;

  void clearShapes() override;
  void drawRectanglesAbove(std::vector<DetectorTube> const &tubes) override;

private slots:
  void reconnectInstrumentActor();
  void reconnectSurface();
  void selectWholeTube();
  void notifyWholeTubeSelected(size_t pickID);

private:
  ALFInstrumentWidget *m_instrumentWidget;
};

} // namespace CustomInterfaces
} // namespace MantidQt
