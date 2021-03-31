// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2020 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include "MantidAPI/MatrixWorkspace.h"
#include "MantidAPI/NumericAxis.h"
#include "MantidGeometry/Instrument.h"
#include "MantidQtWidgets/Common/ImageInfoModelMatrixWS.h"
#include "MantidQtWidgets/Common/QStringUtils.h"
#include "MantidTestHelpers/InstrumentCreationHelper.h"
#include "MantidTestHelpers/WorkspaceCreationHelper.h"
#include <cxxtest/TestSuite.h>

using namespace Mantid::API;
using namespace MantidQt::MantidWidgets;
using namespace Mantid::DataObjects;
using MantidQt::API::toQStringInternal;
using InfoItems = std::vector<std::pair<QString, QString>>;

namespace {

struct DirectEFixed {
  DirectEFixed(std::string logName) : m_logName(std::move(logName)) {}
  const std::string m_logName;

  MatrixWorkspace_sptr operator()(MatrixWorkspace_sptr ws) const {
    auto &instParams = ws->instrumentParameters();
    instParams.addString(ws->getInstrument()->baseInstrument().get(), "deltaE-mode", "Direct");
    ws->mutableRun().addProperty<double>(m_logName, 60., true);
    return ws;
  }
};

struct IndirectEFixed {
  IndirectEFixed(std::string paramName) : m_paramName(std::move(paramName)) {}
  const std::string m_paramName;

  MatrixWorkspace_sptr operator()(MatrixWorkspace_sptr ws) const {
    auto &instParams = ws->instrumentParameters();
    auto baseInst = ws->getInstrument()->baseInstrument().get();
    instParams.addString(baseInst, "deltaE-mode", "Indirect");
    instParams.addDouble(baseInst, m_paramName, 50.);
    return ws;
  }
};

} // namespace

/**
 * Convenience operator for concatenating two InfoItems
 */
InfoItems operator<<(const InfoItems &lhs, const InfoItems &rhs) {
  InfoItems result(lhs);
  result.resize(lhs.size() + rhs.size());
  std::copy(rhs.begin(), rhs.end(), std::next(result.begin(), lhs.size()));
  return result;
}

class ImageInfoModelMatrixWSTest : public CxxTest::TestSuite {

public:
  // This pair of boilerplate methods prevent the suite being created statically
  // This means the constructor isn't called when running other tests
  static ImageInfoModelMatrixWSTest *createSuite() { return new ImageInfoModelMatrixWSTest(); }
  static void destroySuite(ImageInfoModelMatrixWSTest *suite) { delete suite; }

  void test_info_without_instrument() {
    auto workspace = WorkspaceCreationHelper::create2DWorkspaceBinned(10, 10, 15000.0, 100.);
    ImageInfoModelMatrixWS model(workspace);

    const auto info = model.info(15200, 4, 7);

    const InfoItems expectedInfo = {{"x", "15200.0000"},
                                    {"Spectrum", "4"},
                                    {"Signal", "7.0000"},
                                    {"Det ID", "-"},
                                    {"L2(m)", "-"},
                                    {"TwoTheta(Deg)", "-"},
                                    {"Azimuthal(Deg)", "-"},
                                    {"TOF" + toQStringInternal(L"(\u03bcs)"), "-"},
                                    {"Wavelength" + toQStringInternal(L"(\u212b)"), "-"},
                                    {"Energy(meV)", "-"},
                                    {"d-Spacing" + toQStringInternal(L"(\u212b)"), "-"},
                                    {"|Q|" + toQStringInternal(L"(\u212b\u207b\u00b9)"), "-"},
                                    {"Energy transfer(meV)", "-"}};
    int index(0);
    for (const auto &[expectedName, expectedValue] : expectedInfo) {
      TS_ASSERT_EQUALS(expectedName, info.name(index));
      TS_ASSERT_EQUALS(expectedValue, info.value(index));
      index++;
    }
  }

  void test_info_with_either_xysignal_dblmax() {
    auto workspace = WorkspaceCreationHelper::create2DWorkspaceBinned(1, 1, 15000.0, 100.);
    ImageInfoModelMatrixWS model(workspace);

    auto assertBlankInfo = [](const auto &info) {
      const InfoItems expectedInfo = {{"x", "-"},
                                      {"Spectrum", "-"},
                                      {"Signal", "-"},
                                      {"Det ID", "-"},
                                      {"L2(m)", "-"},
                                      {"TwoTheta(Deg)", "-"},
                                      {"Azimuthal(Deg)", "-"},
                                      {"TOF" + toQStringInternal(L"(\u03bcs)"), "-"},
                                      {"Wavelength" + toQStringInternal(L"(\u212b)"), "-"},
                                      {"Energy(meV)", "-"},
                                      {"d-Spacing" + toQStringInternal(L"(\u212b)"), "-"},
                                      {"|Q|" + toQStringInternal(L"(\u212b\u207b\u00b9)"), "-"},
                                      {"Energy transfer(meV)", "-"}};
      int index(0);
      for (const auto &[expectedName, expectedValue] : expectedInfo) {
        TS_ASSERT_EQUALS(expectedName, info.name(index));
        TS_ASSERT_EQUALS(expectedValue, info.value(index));
        index++;
      }
    };

    constexpr auto dblmax = std::numeric_limits<double>::max();
    assertBlankInfo(model.info(dblmax, 4, 7));
    assertBlankInfo(model.info(15200, dblmax, 7));
    assertBlankInfo(model.info(15200, 4, dblmax));
  }

  void test_info_for_monitor() {
    auto noEfixed = [](MatrixWorkspace_sptr ws) { return ws; };

    const double x(15200), y(9), signal(7);
    const auto expectedInfo = expectedTOFInfo("15200.0000", "9", "7.0000", "9", "-9.0000", "-", "-")
                              << expectedUnitsInfo("-", "-", "-", "-", "-");

    assertInfoAsExpected(noEfixed, x, y, signal, expectedInfo);
  }

  void test_info_without_efixed_defined() {
    auto noEfixed = [](MatrixWorkspace_sptr ws) { return ws; };

    const auto expectedInfo = expectedCommonTOFInfo()
                              << expectedUnitsInfo("2.4044", "14.1501", "40.1274", "0.1566", "-");
    const double x(15200), y(4), signal(7);

    assertInfoAsExpected(noEfixed, x, y, signal, expectedInfo);
  }

  void test_info_with_efixed_for_direct_mode() {
    const double x(15200), y(4), signal(7);
    const auto expectedInfo = expectedCommonTOFInfo()
                              << expectedUnitsInfo("7.3425", "14.1501", "40.1274", "0.1566", "58.4827");

    for (const auto &logName : {"Ei", "EnergyRequested", "EnergyEstimate"}) {
      assertInfoAsExpected(DirectEFixed(logName), x, y, signal, expectedInfo);
    }
  }

  void test_info_with_efixed_for_indirect_mode() {
    const double x(15200), y(4), signal(7);
    const auto expectedUnitsNoGroups = expectedUnitsInfo("2.6862", "14.1501", "40.1274", "0.1566", "-38.6633");
    const auto expectedUnitsWithGroup = expectedUnitsInfo("2.6860", "14.1541", "34.4103", "0.1826", "-38.6614");
    for (const auto &paramName : {"Efixed", "EFixed-val"}) {
      bool includeGrouping(false);
      const auto expectedInfoNoGroups = expectedCommonTOFInfo() << expectedUnitsNoGroups;
      assertInfoAsExpected(IndirectEFixed(paramName), x, y, signal, expectedInfoNoGroups, includeGrouping);

      includeGrouping = true;
      const auto expectedInfoWithGroup =
          expectedTOFInfo("15200.0000", "4", "7.0000", "4", "5.0125", "4.0038", "90.0000") << expectedUnitsWithGroup;
      assertInfoAsExpected(IndirectEFixed(paramName), x, y, signal, expectedInfoWithGroup, includeGrouping);
    }
  }

  void test_that_info_will_not_throw_when_the_x_unit_is_something_other_than_TOF() {
    const double x(15200), y(4), signal(7);

    for (const auto &logName : {"Ei", "EnergyRequested", "EnergyEstimate"}) {
      auto workspace = WorkspaceCreationHelper::create2DWorkspaceBinned(10, 10, 15000.0, 100.);
      workspace->getAxis(0)->setUnit("Wavelength");
      workspace->setYUnit("Counts");

      InstrumentCreationHelper::addFullInstrumentToWorkspace(*workspace, true, false, "test-instrument");

      ImageInfoModelMatrixWS model(DirectEFixed(logName)(workspace));

      TS_ASSERT_THROWS_NOTHING(model.info(x, y, signal));
    }
  }

private:
  template <typename EFixedProvider>
  void assertInfoAsExpected(const EFixedProvider &addEfixed, const double x, const double y, const double signal,
                            const InfoItems &expectedInfo, const bool includeGrouping = false) const {
    auto workspace = WorkspaceCreationHelper::create2DWorkspaceBinned(10, 10, 15000.0, 100.);
    workspace->getAxis(0)->setUnit("TOF");
    workspace->setYUnit("Counts");

    InstrumentCreationHelper::addFullInstrumentToWorkspace(*workspace, true, false, "test-instrument");
    if (includeGrouping) {
      workspace->getSpectrum(3).addDetectorID(5);
    }
    ImageInfoModelMatrixWS model(addEfixed(workspace));

    const auto info = model.info(x, y, signal);

    TS_ASSERT_EQUALS(expectedInfo.size(), info.size());
    int index(0);
    for (const auto &[expectedName, expectedValue] : expectedInfo) {
      TS_ASSERT_EQUALS(expectedName.toStdString(), info.name(index).toStdString());
      TS_ASSERT_EQUALS(expectedValue.toStdString(), info.value(index).toStdString());
      index++;
    }
  }

  /// Return the set of instrument related info for the common test point
  /// 15200,4,7
  InfoItems expectedCommonTOFInfo() {
    return expectedTOFInfo("15200.0000", "4", "7.0000", "4", "5.0090", "3.4336", "90.0000");
  }

  InfoItems expectedTOFInfo(const QString &tof, const QString &y, const QString &signal, const QString &detID,
                            const QString &l2, const QString &twoTheta, const QString &azimuth) const {
    const InfoItems expectedCommonInfo = {{"TOF" + toQStringInternal(L"(\u03bcs)"), tof},
                                          {"Spectrum", y},
                                          {"Signal", signal},
                                          {"Det ID", detID},
                                          {"L2(m)", l2},
                                          {"TwoTheta(Deg)", twoTheta},
                                          {"Azimuthal(Deg)", azimuth}};

    return expectedCommonInfo;
  }

  InfoItems expectedUnitsInfo(const QString &wavelength, const QString &energy, const QString &dspacing,
                              const QString &modQ, const QString &deltaE) const {
    const InfoItems expectedUnitsInfo = {{"Wavelength" + toQStringInternal(L"(\u212b)"), wavelength},
                                         {"Energy(meV)", energy},
                                         {"d-Spacing" + toQStringInternal(L"(\u212b)"), dspacing},
                                         {"|Q|" + toQStringInternal(L"(\u212b\u207b\u00b9)"), modQ},
                                         {"Energy transfer(meV)", deltaE}};
    return expectedUnitsInfo;
  }
};
