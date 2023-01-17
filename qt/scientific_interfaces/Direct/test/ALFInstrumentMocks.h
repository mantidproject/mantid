// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2022 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include <cxxtest/TestSuite.h>
#include <gmock/gmock.h>

#include "ALFInstrumentModel.h"
#include "ALFInstrumentPresenter.h"
#include "ALFInstrumentView.h"
#include "DetectorTube.h"
#include "MantidAPI/MatrixWorkspace_fwd.h"
#include "MantidKernel/WarningSuppressions.h"

#include <QWidget>

#include <optional>
#include <string>
#include <tuple>

namespace Mantid::Geometry {
class ComponentInfo;
}

namespace MantidQt {

namespace MantidWidgets {
class IInstrumentActor;
class InstrumentWidget;
} // namespace MantidWidgets

namespace CustomInterfaces {

GNU_DIAG_OFF_SUGGEST_OVERRIDE

class MockALFInstrumentView : public IALFInstrumentView {
public:
  MOCK_METHOD1(setUpInstrument, void(const std::string &fileName));

  MOCK_METHOD0(generateSampleLoadWidget, QWidget *());
  MOCK_METHOD0(generateVanadiumLoadWidget, QWidget *());
  MOCK_METHOD0(getInstrumentView, ALFInstrumentWidget *());

  MOCK_METHOD1(subscribePresenter, void(IALFInstrumentPresenter *presenter));

  MOCK_METHOD0(loadSettings, void());
  MOCK_METHOD0(saveSettings, void());

  MOCK_CONST_METHOD0(getSampleFile, std::optional<std::string>());
  MOCK_CONST_METHOD0(getVanadiumFile, std::optional<std::string>());

  MOCK_METHOD1(setSampleRun, void(std::string const &runNumber));
  MOCK_METHOD1(setVanadiumRun, void(std::string const &runNumber));

  MOCK_CONST_METHOD0(getInstrumentActor, MantidWidgets::IInstrumentActor const &());

  MOCK_CONST_METHOD0(getSelectedDetectors, std::vector<DetectorTube>());

  MOCK_METHOD0(clearShapes, void());
  MOCK_METHOD1(drawRectanglesAbove, void(std::vector<DetectorTube> const &tubes));

  MOCK_METHOD1(warningBox, void(std::string const &message));
};

class MockALFInstrumentModel : public IALFInstrumentModel {
public:
  MOCK_METHOD1(loadAndNormalise, Mantid::API::MatrixWorkspace_sptr(std::string const &filename));
  MOCK_METHOD0(generateLoadedWorkspace, void());

  MOCK_METHOD1(setSample, void(Mantid::API::MatrixWorkspace_sptr const &sample));
  MOCK_METHOD1(setVanadium, void(Mantid::API::MatrixWorkspace_sptr const &vanadium));

  MOCK_CONST_METHOD0(loadedWsName, std::string());

  MOCK_CONST_METHOD0(sampleRun, std::size_t());
  MOCK_CONST_METHOD0(vanadiumRun, std::size_t());

  MOCK_METHOD1(setSelectedTubes, bool(std::vector<DetectorTube> tubes));
  MOCK_METHOD1(addSelectedTube, bool(DetectorTube const &tube));
  MOCK_CONST_METHOD0(selectedTubes, std::vector<DetectorTube>());

  MOCK_CONST_METHOD1(generateOutOfPlaneAngleWorkspace,
                     std::tuple<Mantid::API::MatrixWorkspace_sptr, std::vector<double>>(
                         MantidQt::MantidWidgets::IInstrumentActor const &actor));
};

class MockALFInstrumentPresenter : public IALFInstrumentPresenter {
public:
  MOCK_METHOD0(getSampleLoadWidget, QWidget *());
  MOCK_METHOD0(getVanadiumLoadWidget, QWidget *());
  MOCK_METHOD0(getInstrumentView, ALFInstrumentWidget *());

  MOCK_METHOD1(subscribeAnalysisPresenter, void(IALFAnalysisPresenter *presenter));

  MOCK_METHOD0(loadSample, void());
  MOCK_METHOD0(loadVanadium, void());

  MOCK_METHOD0(notifyInstrumentActorReset, void());
  MOCK_METHOD0(notifyShapeChanged, void());
  MOCK_METHOD1(notifyTubesSelected, void(std::vector<DetectorTube> const &tubes));
};

GNU_DIAG_ON_SUGGEST_OVERRIDE

} // namespace CustomInterfaces
} // namespace MantidQt