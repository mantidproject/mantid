// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include <cxxtest/TestSuite.h>

#include "MantidAPI/AnalysisDataService.h"
#include "MantidAPI/WorkspaceFactory.h"
#include "MantidAlgorithms/ClearInstrumentParameters.h"
#include "MantidDataHandling/LoadInstrument.h"
#include "MantidDataObjects/Workspace2D.h"
#include "MantidFrameworkTestHelpers/WorkspaceCreationHelper.h"
#include "MantidGeometry/IComponent.h"
#include "MantidGeometry/Instrument.h"
#include "MantidGeometry/Instrument/ComponentInfo.h"
#include "MantidGeometry/Instrument/DetectorInfo.h"
#include "MantidKernel/OptionalBool.h"

using namespace Mantid::Algorithms;
using namespace Mantid::API;
using namespace Mantid::Kernel;
using namespace Mantid::Geometry;
using namespace Mantid::DataHandling;
using namespace Mantid::DataObjects;

class ClearInstrumentParametersTest : public CxxTest::TestSuite {
public:
  void testClearInstrumentParameters() {
    // Load a workspace
    prepareWorkspace();

    // Set some parameters
    setParam("nickel-holder", "testDouble", 1.23);
    setParam("nickel-holder", "testString", "hello world");
    const auto oldPos = m_ws->detectorInfo().position(0);
    m_ws->mutableDetectorInfo().setPosition(0, oldPos + V3D(1.1, 2.2, 3.3));
    TS_ASSERT_DIFFERS(oldPos, m_ws->detectorInfo().position(0));

    // Clear the parameters
    clearParameters();

    // Check the parameters
    checkEmpty("nickel-holder", "testDouble");
    checkEmpty("nickel-holder", "testString");
    TS_ASSERT_EQUALS(oldPos, m_ws->detectorInfo().position(0));
  }

  void test_pixels_of_a_moved_and_rotated_bank_are_restored() {
    MatrixWorkspace_sptr ws = WorkspaceCreationHelper::create2DWorkspaceWithRectangularInstrument(1, 3, 1);
    auto &componentInfo = ws->mutableComponentInfo();
    const size_t bankIndex = componentInfo.indexOfAny("bank1");
    const size_t pixelIndex = componentInfo.detectorsInSubtree(bankIndex).back();
    const V3D bankPosition = componentInfo.position(bankIndex);
    const Quat bankRotation = componentInfo.rotation(bankIndex);
    const V3D pixelPosition = componentInfo.position(pixelIndex);
    const Quat pixelRotation = componentInfo.rotation(pixelIndex);
    const V3D pixelScale = componentInfo.scaleFactor(pixelIndex);

    componentInfo.setRotation(bankIndex, Quat(30.0, V3D(0.0, 1.0, 0.0)));
    componentInfo.setPosition(bankIndex, bankPosition + V3D(0.1, 0.2, 0.3));
    // and a pixel calibrated relative to its bank, by less than V3D's comparison tolerance
    componentInfo.setPosition(pixelIndex, componentInfo.position(pixelIndex) + V3D(1e-9, 0.0, 0.0));
    componentInfo.setScaleFactor(pixelIndex, V3D(2.0, 2.0, 2.0));
    TS_ASSERT_DIFFERS(componentInfo.position(pixelIndex), pixelPosition);

    runClear(ws);

    const auto &cleared = ws->componentInfo();
    TS_ASSERT_EQUALS(cleared.position(bankIndex), bankPosition);
    TS_ASSERT_EQUALS(cleared.rotation(bankIndex), bankRotation);
    TS_ASSERT_EQUALS(cleared.rotation(pixelIndex), pixelRotation);
    TS_ASSERT_EQUALS(cleared.scaleFactor(pixelIndex), pixelScale);
    TS_ASSERT_DELTA(cleared.position(pixelIndex).X(), pixelPosition.X(), 1e-12);
    TS_ASSERT_DELTA(cleared.position(pixelIndex).Y(), pixelPosition.Y(), 1e-12);
    TS_ASSERT_DELTA(cleared.position(pixelIndex).Z(), pixelPosition.Z(), 1e-12);
  }

  void test_masks_are_cleared() {
    MatrixWorkspace_sptr ws = WorkspaceCreationHelper::create2DWorkspaceWithFullInstrument(3, 1);
    ws->mutableDetectorInfo().setMasked(1, true);

    runClear(ws);

    TS_ASSERT(!ws->detectorInfo().isMasked(1));
  }

  void test_parameters_can_be_added_after_clearing() {
    MatrixWorkspace_sptr ws = WorkspaceCreationHelper::create2DWorkspaceWithFullInstrument(3, 1);
    const size_t root = ws->componentInfo().root();
    ws->mutableComponentInfo().addDouble(root, "before", 1.0);

    runClear(ws);
    ws->mutableComponentInfo().addDouble(root, "after", 2.0);

    const auto &componentInfo = ws->componentInfo();
    TS_ASSERT(!componentInfo.hasParameter(root, "before"));
    TS_ASSERT_EQUALS(componentInfo.getNumberParameter(root, "after"), std::vector<double>{2.0});
    // the legacy view shares the same store, so must agree
    const auto *rootID = componentInfo.componentID(root);
    TS_ASSERT(!ws->constInstrumentParameters().contains(rootID, "before"));
    TS_ASSERT(ws->constInstrumentParameters().contains(rootID, "after"));
  }

  void test_workspace_with_detector_scan_is_rejected() {
    MatrixWorkspace_sptr ws = WorkspaceCreationHelper::create2DDetectorScanWorkspaceWithFullInstrument(3, 1, 2);
    ClearInstrumentParameters clearer;
    clearer.initialize();
    clearer.setChild(true);
    clearer.setRethrows(true);
    clearer.setProperty("Workspace", ws);
    TS_ASSERT_THROWS(clearer.execute(), const std::runtime_error &);
  }

  void runClear(const MatrixWorkspace_sptr &ws) {
    ClearInstrumentParameters clearer;
    TS_ASSERT_THROWS_NOTHING(clearer.initialize());
    clearer.setChild(true);
    clearer.setRethrows(true);
    TS_ASSERT_THROWS_NOTHING(clearer.setProperty("Workspace", ws));
    TS_ASSERT_THROWS_NOTHING(clearer.execute());
    TS_ASSERT(clearer.isExecuted());
  }

  void setParam(const std::string &cName, const std::string &pName, const std::string &value) {
    Instrument_const_sptr inst = m_ws->getInstrument();
    ParameterMap &paramMap = m_ws->instrumentParameters();
    std::shared_ptr<const IComponent> comp = inst->getComponentByName(cName);
    paramMap.addString(comp->getComponentID(), pName, value);
  }

  void setParam(const std::string &cName, const std::string &pName, double value) {
    Instrument_const_sptr inst = m_ws->getInstrument();
    ParameterMap &paramMap = m_ws->instrumentParameters();
    std::shared_ptr<const IComponent> comp = inst->getComponentByName(cName);
    paramMap.addDouble(comp->getComponentID(), pName, value);
  }

  void checkEmpty(const std::string &cName, const std::string &pName) {
    Instrument_const_sptr inst = m_ws->getInstrument();
    ParameterMap &paramMap = m_ws->instrumentParameters();
    std::shared_ptr<const IComponent> comp = inst->getComponentByName(cName);
    bool exists = paramMap.contains(comp.get(), pName);
    TS_ASSERT_EQUALS(exists, false);
  }

  void clearParameters() {
    ClearInstrumentParameters clearer;
    TS_ASSERT_THROWS_NOTHING(clearer.initialize());
    clearer.setPropertyValue("Workspace", m_ws->getName());
    TS_ASSERT_THROWS_NOTHING(clearer.execute());
    TS_ASSERT(clearer.isExecuted());
  }

  void prepareWorkspace() {
    LoadInstrument loaderIDF2;

    TS_ASSERT_THROWS_NOTHING(loaderIDF2.initialize());

    std::string wsName = "SaveParameterFileTestIDF2";
    Workspace_sptr ws = WorkspaceFactory::Instance().create("Workspace2D", 1, 1, 1);
    Workspace2D_sptr ws2D = std::dynamic_pointer_cast<Workspace2D>(ws);

    TS_ASSERT_THROWS_NOTHING(AnalysisDataService::Instance().add(wsName, ws2D));

    loaderIDF2.setPropertyValue("Filename", "unit_testing/IDF_for_UNIT_TESTING2.xml");
    loaderIDF2.setPropertyValue("Workspace", wsName);
    loaderIDF2.setProperty("RewriteSpectraMap", Mantid::Kernel::OptionalBool(true));
    TS_ASSERT_THROWS_NOTHING(loaderIDF2.execute());
    TS_ASSERT(loaderIDF2.isExecuted());

    m_ws = std::dynamic_pointer_cast<MatrixWorkspace>(ws2D);
  }

private:
  MatrixWorkspace_sptr m_ws;
};
