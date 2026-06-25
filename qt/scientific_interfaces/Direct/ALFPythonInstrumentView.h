// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2022 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include "ALFInstrumentViewBase.h"
#include "ALFPythonCallbackRelay.h"
#include "DllConfig.h"
#include "IALFInstrumentView.h"
#include "MantidAPI/AnalysisDataServiceObserver.h"
#include "MantidQtWidgets/Common/MessageHandler.h"
#include "MantidQtWidgets/Common/Python/Object.h"
#include "MantidQtWidgets/InstrumentView/InstrumentActor.h"

#include <memory>
#include <qwidget.h>
#include <string>
#include <vector>

using namespace MantidQt::Widgets::Common;

namespace MantidQt {

namespace MantidWidgets {
class IInstrumentActor;
}

namespace CustomInterfaces {

class ALFInstrumentWidget;

class MANTIDQT_DIRECT_DLL ALFPythonInstrumentView final : public ALFInstrumentViewBase,
                                                          public Python::InstanceHolder,
                                                          public AnalysisDataServiceObserver {
  Q_OBJECT

public:
  explicit ALFPythonInstrumentView(QWidget *parent = nullptr);

  void setUpInstrument(std::string const &fileName) override;

  QWidget *getInstrumentView() override;

  MantidWidgets::IInstrumentActor const &getInstrumentActor() const override;

  std::vector<DetectorTube> getSelectedDetectors() const override;

  void clearShapes() override {};
  void drawRectanglesAbove(std::vector<DetectorTube> const &) override {};

  void notifyWholeTubeSelected();

  void replaceHandle(const std::string &wsName, const Workspace_sptr &workspace) override;

  void resetActor();

private:
  void ensureCallbackRelay(QWidget *instrumentView);

  ALFPythonCallbackRelay *m_callbackRelay{nullptr};

  mutable std::unique_ptr<MantidWidgets::InstrumentActor> m_actor;
  std::unique_ptr<MantidWidgets::IMessageHandler> messageHandler = std::make_unique<MantidWidgets::MessageHandler>();
  std::string m_workspaceName;
};

} // namespace CustomInterfaces
} // namespace MantidQt
