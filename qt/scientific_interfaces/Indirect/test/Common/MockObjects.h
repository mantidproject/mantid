// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2024 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include <cxxtest/TestSuite.h>
#include <gmock/gmock.h>

#include "MantidKernel/WarningSuppressions.h"

#include "Common/InstrumentConfig.h"

#include <QString>
#include <QStringList>

using namespace MantidQt::MantidWidgets;

GNU_DIAG_OFF_SUGGEST_OVERRIDE

class MockInstrumentConfig : public IInstrumentConfig {
public:
  virtual ~MockInstrumentConfig() = default;

  MOCK_METHOD0(getTechniques, QStringList());
  MOCK_METHOD1(setTechniques, void(const QStringList &techniques));

  MOCK_METHOD0(getDisabledInstruments, QStringList());
  MOCK_METHOD1(setDisabledInstruments, void(const QStringList &instrumentNames));

  MOCK_METHOD0(getFacility, QString());
  MOCK_METHOD1(setFacility, void(const QString &facilityName));

  MOCK_METHOD0(isDiffractionEnabled, bool());
  MOCK_METHOD1(enableDiffraction, void(bool enabled));

  MOCK_METHOD0(isDiffractionForced, bool());
  MOCK_METHOD1(forceDiffraction, void(bool forced));

  MOCK_METHOD0(isInstrumentLabelShown, bool());
  MOCK_METHOD1(setShowInstrumentLabel, void(bool visible));

  MOCK_METHOD0(getInstrumentName, QString());
  MOCK_METHOD1(setInstrument, void(const QString &instrumentName));

  MOCK_METHOD0(getAnalyserName, QString());
  MOCK_METHOD1(setAnalyser, void(const QString &analyserName));

  MOCK_METHOD0(getReflectionName, QString());
  MOCK_METHOD1(setReflection, void(const QString &reflectionName));

  MOCK_METHOD1(showAnalyserAndReflectionOptions, void(bool visible));
};

GNU_DIAG_ON_SUGGEST_OVERRIDE
