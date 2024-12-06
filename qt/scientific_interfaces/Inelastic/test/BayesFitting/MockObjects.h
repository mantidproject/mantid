// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2024 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include <cxxtest/TestSuite.h>
#include <gmock/gmock.h>

#include "BayesFitting/QuasiModel.h"
#include "BayesFitting/QuasiView.h"

#include "MantidKernel/WarningSuppressions.h"
#include "MantidQtWidgets/Common/IConfiguredAlgorithm.h"

#include <optional>
#include <string>
#include <utility>

using namespace MantidQt;
using namespace MantidQt::CustomInterfaces;

GNU_DIAG_OFF_SUGGEST_OVERRIDE

class MockQuasiView : public IQuasiView {
public:
  virtual ~MockQuasiView() = default;

  MOCK_METHOD(void, subscribe, (IQuasiPresenter * presenter), (override));
  MOCK_METHOD(IRunView *, getRunView, (), (const, override));
  MOCK_METHOD(MantidWidgets::DataSelector *, sampleSelector, (), (const, override));
  MOCK_METHOD(MantidWidgets::DataSelector *, resolutionSelector, (), (const, override));
  MOCK_METHOD(MantidWidgets::DataSelector *, resNormSelector, (), (const, override));
  MOCK_METHOD(API::FileFinderWidget *, fixWidthFileFinder, (), (const, override));
  MOCK_METHOD(void, setPreviewSpectrumMax, (std::size_t const max), (override));
  MOCK_METHOD(void, setXRange, ((std::pair<double, double> const &range)), (override));
  MOCK_METHOD(void, watchADS, (bool const watch), (override));
  MOCK_METHOD(void, clearPlot, (), (override));
  MOCK_METHOD(bool, hasSpectrum, (std::string const &label), (const, override));
  MOCK_METHOD(void, addSpectrum,
              (std::string const &label, Mantid::API::MatrixWorkspace_sptr const &workspace,
               std::size_t const spectrumIndex, std::string const &colour),
              (override));
  MOCK_METHOD(std::size_t, previewSpectrum, (), (const, override));
  MOCK_METHOD(std::string, sampleName, (), (const, override));
  MOCK_METHOD(std::string, resolutionName, (), (const, override));
  MOCK_METHOD(std::string, resNormName, (), (const, override));
  MOCK_METHOD(std::string, fixWidthName, (), (const, override));
  MOCK_METHOD(std::string, programName, (), (const, override));
  MOCK_METHOD(std::string, backgroundName, (), (const, override));
  MOCK_METHOD(std::string, plotName, (), (const, override));
  MOCK_METHOD(double, eMin, (), (const, override));
  MOCK_METHOD(double, eMax, (), (const, override));
  MOCK_METHOD(int, sampleBinning, (), (const, override));
  MOCK_METHOD(int, resolutionBinning, (), (const, override));
  MOCK_METHOD(bool, useResolution, (), (const, override));
  MOCK_METHOD(bool, fixWidth, (), (const, override));
  MOCK_METHOD(bool, elasticPeak, (), (const, override));
  MOCK_METHOD(bool, sequentialFit, (), (const, override));
  MOCK_METHOD(void, setPlotResultEnabled, (bool const enable), (override));
  MOCK_METHOD(void, setSaveResultEnabled, (bool const enable), (override));
  MOCK_METHOD(void, enableUseResolution, (bool const enable), (override));
  MOCK_METHOD(void, enableView, (bool const enable), (override));
  MOCK_METHOD(bool, displaySaveDirectoryMessage, (), (const, override));
  MOCK_METHOD(void, setFileExtensionsByName, (bool const filter), (override));
  MOCK_METHOD(void, setLoadHistory, (bool const loadHistory), (override));
  MOCK_METHOD(void, loadSettings, (const QSettings &settings), (override));
};

class MockQuasiModel : public IQuasiModel {
public:
  virtual ~MockQuasiModel() = default;

  MOCK_METHOD(void, setSample, (std::string const &workspaceName), (override));
  MOCK_METHOD(Mantid::API::MatrixWorkspace_sptr, sample, (), (const, override));

  MOCK_METHOD(void, setResolution, (std::string const &workspaceName), (override));
  MOCK_METHOD(Mantid::API::MatrixWorkspace_sptr, resolution, (), (const, override));

  MOCK_METHOD(void, setOutputResult, (std::string const &workspaceName), (override));
  MOCK_METHOD(void, setOutputProbability, (std::string const &workspaceName), (override));
  MOCK_METHOD(void, setOutputFitGroup, (std::string const &workspaceName), (override));

  MOCK_METHOD(Mantid::API::MatrixWorkspace_sptr, outputFit, (std::size_t const index), (const, override));
  MOCK_METHOD(Mantid::API::MatrixWorkspace_sptr, outputResult, (), (const, override));
  MOCK_METHOD(Mantid::API::MatrixWorkspace_sptr, outputProbability, (), (const, override));
  MOCK_METHOD(Mantid::API::WorkspaceGroup_sptr, outputFitGroup, (), (const, override));

  MOCK_METHOD(bool, isResolution, (std::string const &workspaceName), (const, override));

  MOCK_METHOD(std::optional<std::string>, curveColour, (std::string const &label), (const, override));

  MOCK_METHOD(API::IConfiguredAlgorithm_sptr, setupBayesQuasiAlgorithm,
              (std::string const &resNormName, std::string const &fixWidthName, std::string const &program,
               std::string const &baseName, std::string const &background, double const eMin, double const eMax,
               int const sampleBinning, int const resolutionBinning, bool const elasticPeak, bool const fixWidth,
               bool const useResNorm, bool const sequentialFit),
              (const, override));

  MOCK_METHOD(API::IConfiguredAlgorithm_sptr, setupBayesQuasi2Algorithm,
              (std::string const &program, std::string const &baseName, std::string const &background,
               double const eMin, double const eMax, bool const elasticPeak),
              (const, override));

  MOCK_METHOD(API::IConfiguredAlgorithm_sptr, setupSaveAlgorithm, (Mantid::API::Workspace_sptr workspace),
              (const, override));
};

GNU_DIAG_ON_SUGGEST_OVERRIDE
