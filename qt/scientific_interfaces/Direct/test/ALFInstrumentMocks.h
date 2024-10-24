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

  MOCK_METHOD1(disable, void(std::string const &reason));
  MOCK_METHOD0(enable, void());

  MOCK_CONST_METHOD0(getSampleFile, std::optional<std::string>());
  MOCK_CONST_METHOD0(getVanadiumFile, std::optional<std::string>());

  MOCK_METHOD1(setSampleRun, void(std::string const &runNumber));
  MOCK_METHOD1(setVanadiumRun, void(std::string const &runNumber));

  MOCK_CONST_METHOD0(getInstrumentActor, MantidWidgets::IInstrumentActor const &());

  MOCK_CONST_METHOD0(getSelectedDetectors, std::vector<DetectorTube>());

  MOCK_METHOD0(clearShapes, void());
  MOCK_METHOD1(drawRectanglesAbove, void(std::vector<DetectorTube> const &tubes));

  MOCK_METHOD1(displayWarning, void(std::string const &message));
};

class MockALFInstrumentModel : public IALFInstrumentModel {
public:
  MOCK_CONST_METHOD0(loadedWsName, std::string());

  MOCK_METHOD2(setData, void(ALFData const &dataType, Mantid::API::MatrixWorkspace_sptr const &sample));
  MOCK_CONST_METHOD1(hasData, bool(ALFData const &dataType));
  MOCK_CONST_METHOD1(data, Mantid::API::MatrixWorkspace_sptr(ALFData const &dataType));

  MOCK_CONST_METHOD1(replaceSampleWorkspaceInADS, void(Mantid::API::MatrixWorkspace_sptr const &workspace));

  MOCK_CONST_METHOD1(run, std::size_t(ALFData const &dataType));

  MOCK_CONST_METHOD1(isALFData, bool(Mantid::API::MatrixWorkspace_const_sptr const &workspace));
  MOCK_CONST_METHOD0(binningMismatch, bool());
  MOCK_CONST_METHOD0(axisIsDSpacing, bool());

  MOCK_METHOD1(setSelectedTubes, bool(std::vector<DetectorTube> tubes));
  MOCK_METHOD1(addSelectedTube, bool(DetectorTube const &tube));
  MOCK_CONST_METHOD0(hasSelectedTubes, bool());
  MOCK_CONST_METHOD0(selectedTubes, std::vector<DetectorTube>());

  MOCK_CONST_METHOD0(twoThetasClosestToZero, std::vector<double>());

  MOCK_CONST_METHOD1(loadProperties, std::unique_ptr<Mantid::API::AlgorithmRuntimeProps>(std::string const &filename));
  MOCK_CONST_METHOD1(normaliseByCurrentProperties, std::unique_ptr<Mantid::API::AlgorithmRuntimeProps>(
                                                       Mantid::API::MatrixWorkspace_sptr const &inputWorkspace));
  MOCK_CONST_METHOD0(rebinToWorkspaceProperties, std::unique_ptr<Mantid::API::AlgorithmRuntimeProps>());
  MOCK_CONST_METHOD0(divideProperties, std::unique_ptr<Mantid::API::AlgorithmRuntimeProps>());
  MOCK_CONST_METHOD1(replaceSpecialValuesProperties, std::unique_ptr<Mantid::API::AlgorithmRuntimeProps>(
                                                         Mantid::API::MatrixWorkspace_sptr const &inputWorkspace));
  MOCK_CONST_METHOD1(convertUnitsProperties, std::unique_ptr<Mantid::API::AlgorithmRuntimeProps>(
                                                 Mantid::API::MatrixWorkspace_sptr const &inputWorkspace));

  MOCK_METHOD1(createWorkspaceAlgorithmProperties, std::unique_ptr<Mantid::API::AlgorithmRuntimeProps>(
                                                       MantidQt::MantidWidgets::IInstrumentActor const &actor));
  MOCK_CONST_METHOD1(scaleXProperties, std::unique_ptr<Mantid::API::AlgorithmRuntimeProps>(
                                           Mantid::API::MatrixWorkspace_sptr const &inputWorkspace));
  MOCK_CONST_METHOD1(rebunchProperties, std::unique_ptr<Mantid::API::AlgorithmRuntimeProps>(
                                            Mantid::API::MatrixWorkspace_sptr const &inputWorkspace));
};

class MockALFInstrumentPresenter : public IALFInstrumentPresenter {
public:
  MOCK_METHOD0(getSampleLoadWidget, QWidget *());
  MOCK_METHOD0(getVanadiumLoadWidget, QWidget *());
  MOCK_METHOD0(getInstrumentView, ALFInstrumentWidget *());

  MOCK_METHOD1(subscribeAnalysisPresenter, void(IALFAnalysisPresenter *presenter));

  MOCK_METHOD0(loadSettings, void());
  MOCK_METHOD0(saveSettings, void());

  MOCK_METHOD0(loadSample, void());
  MOCK_METHOD0(loadVanadium, void());

  MOCK_METHOD0(notifyInstrumentActorReset, void());
  MOCK_METHOD0(notifyShapeChanged, void());
  MOCK_METHOD1(notifyTubesSelected, void(std::vector<DetectorTube> const &tubes));
};

GNU_DIAG_ON_SUGGEST_OVERRIDE

} // namespace CustomInterfaces
} // namespace MantidQt
