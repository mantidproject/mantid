// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2025 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include "MantidFrameworkTestHelpers/WorkspaceCreationHelper.h"
#include <MantidAPI/AlgorithmManager.h>
#include <MantidAPI/MatrixWorkspace.h>
#include <MantidAPI/PanelsSurfaceCalculator.h>
#include <cxxtest/TestSuite.h>

using Mantid::API::PanelsSurfaceCalculator;

class PanelsSurfaceCalculatorTest : public CxxTest::TestSuite {
public:
  // This pair of boilerplate methods prevent the suite being created statically
  // This means the constructor isn't called when running other tests
  static PanelsSurfaceCalculatorTest *createSuite() { return new PanelsSurfaceCalculatorTest(); }
  static void destroySuite(PanelsSurfaceCalculatorTest *suite) { delete suite; }

  void testCreation() { TS_ASSERT_THROWS_NOTHING(const auto p = PanelsSurfaceCalculator()); }

  void testSetupBasisAxes() {
    const auto p = PanelsSurfaceCalculator();
    const auto zAxis = V3D(0, 0, 1);
    V3D xAxis, yAxis;
    p.setupBasisAxes(zAxis, xAxis, yAxis);
    // Need to check that the generated x and y axes form a basis
    const double tol = 1e-9;
    TS_ASSERT_DELTA(0, zAxis.scalar_prod(xAxis), tol);
    TS_ASSERT_DELTA(0, zAxis.scalar_prod(yAxis), tol);
    TS_ASSERT_DELTA(0, xAxis.scalar_prod(yAxis), tol);
    TS_ASSERT_DELTA(1, xAxis.scalar_prod(xAxis), tol);
    TS_ASSERT_DELTA(1, zAxis.scalar_prod(zAxis), tol);
    TS_ASSERT_DELTA(1, yAxis.scalar_prod(yAxis), tol);
  }

  void testRetrievePanelCorners() {
    const auto ws = WorkspaceCreationHelper::create2DWorkspaceWithRectangularInstrument(1, 5, 10);
    const size_t rectangularComponentIndex = 30;
    const auto p = PanelsSurfaceCalculator();
    const auto corners = p.retrievePanelCorners(ws->componentInfo(), rectangularComponentIndex);
    const double tol = 1e-4;
    compareTwoV3Ds(V3D(0, -0.5, 5), corners[0], tol);
    compareTwoV3Ds(V3D(0.032, -0.5, 5), corners[1], tol);
    compareTwoV3Ds(V3D(0.032, 0.53205, 5), corners[2], tol);
    compareTwoV3Ds(V3D(0, 0.53205, 5), corners[3], tol);
  }

  void testCalculatePanelNormal() {
    const auto ws = WorkspaceCreationHelper::create2DWorkspaceWithRectangularInstrument(1, 5, 10);
    const size_t rectangularComponentIndex = 30;
    const auto p = PanelsSurfaceCalculator();
    const auto corners = p.retrievePanelCorners(ws->componentInfo(), rectangularComponentIndex);
    const auto normal = p.calculatePanelNormal(corners);
    compareTwoV3Ds(V3D(0, 0, 1), normal, 1e-4);
  }

  void testIsBankFlat() {
    const auto ws = WorkspaceCreationHelper::create2DWorkspaceWithFullInstrument(1, 10);
    const size_t tubeIndex = 4;
    auto p = PanelsSurfaceCalculator();
    const V3D normal(0, 1, 0);
    const std::vector<size_t> tubes{tubeIndex};
    const bool isBankFlat = p.isBankFlat(ws->componentInfo(), tubeIndex, tubes, normal);
    TS_ASSERT(isBankFlat);
  }

  void testCalculateBankNormal() {
    const auto ws = WorkspaceCreationHelper::create2DWorkspaceWithRectangularInstrument(1, 5, 10);
    auto p = PanelsSurfaceCalculator();
    const auto bankNormal = p.calculateBankNormal(ws->componentInfo(), {25, 26});
    compareTwoV3Ds(V3D(0, 0, -1), bankNormal, 1e-9);
  }

  void testSetBankVisited() {
    const auto ws = WorkspaceCreationHelper::create2DWorkspaceWithFullInstrument(1, 10);
    const size_t tubeIndex = 4;
    auto p = PanelsSurfaceCalculator();
    std::vector<bool> visitedComponents(ws->componentInfo().size(), false);
    p.setBankVisited(ws->componentInfo(), tubeIndex, visitedComponents);
    TS_ASSERT(visitedComponents[tubeIndex]);
  }

  void testFindNumDetectors() {
    const auto ws = WorkspaceCreationHelper::create2DWorkspaceWithFullInstrument(1, 10);
    auto p = PanelsSurfaceCalculator();
    std::vector<size_t> components(ws->componentInfo().size());
    std::iota(components.begin(), components.end(), 0);
    const auto numDetectors = p.findNumDetectors(ws->componentInfo(), components);
    TS_ASSERT_EQUALS(1, numDetectors);
  }

  void testCalcBankRotation() {
    const V3D detectorPosition(1, 0, 0), normal(0, 1, 0), zAxis(0, 0, 1), yAxis(0, 1, 0), samplePosition(0, 0, 0);
    auto p = PanelsSurfaceCalculator();
    const auto rotation = p.calcBankRotation(detectorPosition, normal, zAxis, yAxis, samplePosition);
    const double pi = std::acos(-1);
    const double angle = std::cos(pi / 4.0);
    const double tol = 1e-9;
    TS_ASSERT_DELTA(angle, rotation.real(), tol);
    TS_ASSERT_DELTA(angle, rotation.imagI(), tol);
    TS_ASSERT_DELTA(0, rotation.imagJ(), tol);
    TS_ASSERT_DELTA(0, rotation.imagK(), tol);
  }

  void testTransformedBoundingBoxPoints() {
    const auto ws = WorkspaceCreationHelper::create2DWorkspaceWithRectangularInstrument(1, 5, 10);
    const auto p = PanelsSurfaceCalculator();
    Mantid::Kernel::Quat rotation(45, {1, 0, 0});
    const auto boundingBoxPoints =
        p.transformedBoundingBoxPoints(ws->componentInfo(), 9, {0, 0, 0}, rotation, {1, 0, 0}, {0, 1, 0});
    const double tol = 1e-3;
    TS_ASSERT_DELTA(0.004, boundingBoxPoints[0].X(), tol);
    TS_ASSERT_DELTA(-3.510, boundingBoxPoints[0].Y(), tol);
    TS_ASSERT_DELTA(0.012, boundingBoxPoints[1].X(), tol);
    TS_ASSERT_DELTA(-3.516, boundingBoxPoints[1].Y(), tol);
  }

private:
  void compareTwoV3Ds(const V3D &a, const V3D &b, const double tol) {
    const auto diff = a - b;
    TS_ASSERT_DELTA(diff.norm(), 0.0, tol)
  }
};
