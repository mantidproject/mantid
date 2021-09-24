// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2021 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once
#include "MantidAPI/Axis.h"
#include "MantidAPI/Sample.h"
#include "MantidAPI/SpectrumInfo.h"
#include "MantidAPI/WorkspaceGroup.h"
#include "MantidAlgorithms/CalculateMultipleScattering.h"
#include "MantidGeometry/Instrument/ReferenceFrame.h"
#include "MantidKernel/Material.h"
#include "MantidKernel/PhysicalConstants.h"
#include "MantidKernel/UnitFactory.h"
#include "MantidTestHelpers/ComponentCreationHelper.h"
#include "MantidTestHelpers/InstrumentCreationHelper.h"
#include "MantidTestHelpers/WorkspaceCreationHelper.h"
#include <cmath>
#include <cxxtest/TestSuite.h>

using namespace Mantid::DataObjects;
using namespace Mantid::Geometry;
using namespace Mantid::Kernel;

class CalculateMultipleScatteringHelper : public Mantid::Algorithms::CalculateMultipleScattering {
public:
  double interpolateGaussian(const Mantid::HistogramData::Histogram &histToInterpolate, double x) {
    return CalculateMultipleScattering::interpolateGaussian(histToInterpolate, x);
  }
  void updateTrackDirection(Mantid::Geometry::Track &track, const double cosT, const double phi) {
    CalculateMultipleScattering::updateTrackDirection(track, cosT, phi);
  }
};

class CalculateMultipleScatteringTest : public CxxTest::TestSuite {
public:
  // This pair of boilerplate methods prevent the suite being created statically
  // This means the constructor isn't called when running other tests
  static CalculateMultipleScatteringTest *createSuite() { return new CalculateMultipleScatteringTest(); }
  static void destroySuite(CalculateMultipleScatteringTest *suite) { delete suite; }

  CalculateMultipleScatteringTest() {
    SofQWorkspace = WorkspaceCreationHelper::create2DWorkspace(1, 1);
    SofQWorkspace->mutableY(0)[0] = 1.;
    SofQWorkspace->getAxis(0)->unit() = UnitFactory::Instance().create("MomentumTransfer");
  }

  void test_output_workspaces() {
    const double THICKNESS = 0.001; // metres
    auto inputWorkspace = SetupFlatPlateWorkspace(46, 1, 1, THICKNESS);

    auto alg = createAlgorithm();
    TS_ASSERT_THROWS_NOTHING(alg->setProperty("InputWorkspace", inputWorkspace));
    const int NSCATTERINGS = 3;
    TS_ASSERT_THROWS_NOTHING(alg->setProperty("NumberScatterings", NSCATTERINGS));
    TS_ASSERT_THROWS_NOTHING(alg->setProperty("NeutronPathsSingle", 10));
    TS_ASSERT_THROWS_NOTHING(alg->setProperty("NeutronPathsMultiple", 10));
    TS_ASSERT_THROWS_NOTHING(alg->execute(););
    TS_ASSERT(alg->isExecuted());
    if (alg->isExecuted()) {
      Mantid::API::WorkspaceGroup_sptr output =
          Mantid::API::AnalysisDataService::Instance().retrieveWS<Mantid::API::WorkspaceGroup>("MuscatResults");
      std::vector<std::string> wsNames = {"Scatter_1_NoAbs", "Scatter_1", "Scatter_2", "Scatter_3",
                                          "Scatter_2_3_Summed"};
      for (auto &name : wsNames) {
        Mantid::API::Workspace_sptr wsPtr = output->getItem(name);
        auto matrixWsPtr = std::dynamic_pointer_cast<Mantid::API::MatrixWorkspace>(wsPtr);
        TS_ASSERT(matrixWsPtr);
      }
      Mantid::API::AnalysisDataService::Instance().deepRemoveGroup("MuscatResults");
    }
  }

  void test_flat_plate_sample_single_scatter() {
    // generate a result corresponding to Figure 4 in the Mancinelli paper (flat
    // plate sample for once scattered neutrons) where there's an analytical solution
    const double THICKNESS = 0.001; // metres
    auto inputWorkspace = SetupFlatPlateWorkspace(46, 1, 1, THICKNESS);

    auto alg = createAlgorithm();
    TS_ASSERT_THROWS_NOTHING(alg->setProperty("InputWorkspace", inputWorkspace));
    const int NSCATTERINGS = 1;
    TS_ASSERT_THROWS_NOTHING(alg->setProperty("NumberScatterings", NSCATTERINGS));
    TS_ASSERT_THROWS_NOTHING(alg->setProperty("NeutronPathsSingle", 10000));
    TS_ASSERT_THROWS_NOTHING(alg->execute(););
    TS_ASSERT(alg->isExecuted());

    if (alg->isExecuted()) {
      Mantid::API::WorkspaceGroup_sptr output =
          Mantid::API::AnalysisDataService::Instance().retrieveWS<Mantid::API::WorkspaceGroup>("MuscatResults");
      Mantid::API::Workspace_sptr wsPtr = output->getItem("Scatter_1");
      auto singleScatterResult = std::dynamic_pointer_cast<Mantid::API::MatrixWorkspace>(wsPtr);
      // calculate result analytically
      const int SPECTRUMINDEXTOTEST = 1;
      const double analyticResult = calculateFlatPlateAnalyticalResult(
          singleScatterResult->histogram(SPECTRUMINDEXTOTEST).points()[0], inputWorkspace->sample().getMaterial(),
          inputWorkspace->spectrumInfo().twoTheta(SPECTRUMINDEXTOTEST), THICKNESS);
      const double delta(1e-05);
      TS_ASSERT_DELTA(singleScatterResult->y(SPECTRUMINDEXTOTEST)[0], analyticResult, delta);
      Mantid::API::AnalysisDataService::Instance().deepRemoveGroup("MuscatResults");
    }
  }

  void test_flat_plate_sample_multiple_scatter() {
    // same set up as previous test but increase nscatter to 2
    const double THICKNESS = 0.001; // metres
    auto inputWorkspace = SetupFlatPlateWorkspace(2, 1, 1, THICKNESS);

    auto alg = createAlgorithm();
    TS_ASSERT_THROWS_NOTHING(alg->setProperty("InputWorkspace", inputWorkspace));
    const int NSCATTERINGS = 2;
    TS_ASSERT_THROWS_NOTHING(alg->setProperty("NumberScatterings", NSCATTERINGS));
    TS_ASSERT_THROWS_NOTHING(alg->setProperty("NeutronPathsSingle", 100000));
    TS_ASSERT_THROWS_NOTHING(alg->setProperty("NeutronPathsMultiple", 100000));
    TS_ASSERT_THROWS_NOTHING(alg->execute(););
    TS_ASSERT(alg->isExecuted());

    if (alg->isExecuted()) {
      Mantid::API::WorkspaceGroup_sptr output =
          Mantid::API::AnalysisDataService::Instance().retrieveWS<Mantid::API::WorkspaceGroup>("MuscatResults");
      Mantid::API::Workspace_sptr wsPtr1 = output->getItem("Scatter_1");
      auto singleScatterResult = std::dynamic_pointer_cast<Mantid::API::MatrixWorkspace>(wsPtr1);
      Mantid::API::Workspace_sptr wsPtr2 = output->getItem("Scatter_2");
      auto doubleScatterResult = std::dynamic_pointer_cast<Mantid::API::MatrixWorkspace>(wsPtr2);
      // check single scatter result still matches analytical result
      const int SPECTRUMINDEXTOTEST = 1;
      const double analyticResult = calculateFlatPlateAnalyticalResult(
          singleScatterResult->histogram(SPECTRUMINDEXTOTEST).points()[0], inputWorkspace->sample().getMaterial(),
          inputWorkspace->spectrumInfo().twoTheta(SPECTRUMINDEXTOTEST), THICKNESS);
      const double delta(1e-05);
      TS_ASSERT_DELTA(singleScatterResult->y(SPECTRUMINDEXTOTEST)[0], analyticResult, delta);
      // no analytical result for double scatter so just check against current result that we assume is correct
      TS_ASSERT_DELTA(doubleScatterResult->y(SPECTRUMINDEXTOTEST)[0], 0.001977, delta);
      Mantid::API::AnalysisDataService::Instance().deepRemoveGroup("MuscatResults");
    }
  }

  void test_flat_plate_sample_multiple_scatter_with_wavelength_interp() {
    // same set up as previous test but increase nscatter to 2
    const double THICKNESS = 0.001; // metres
    auto inputWorkspace = SetupFlatPlateWorkspace(2, 1, 3, THICKNESS);

    auto alg = createAlgorithm();
    TS_ASSERT_THROWS_NOTHING(alg->setProperty("InputWorkspace", inputWorkspace));
    const int NSCATTERINGS = 2;
    TS_ASSERT_THROWS_NOTHING(alg->setProperty("NumberScatterings", NSCATTERINGS));
    TS_ASSERT_THROWS_NOTHING(alg->setProperty("NeutronPathsSingle", 100000));
    TS_ASSERT_THROWS_NOTHING(alg->setProperty("NeutronPathsMultiple", 100000));
    TS_ASSERT_THROWS_NOTHING(alg->setProperty("NumberOfWavelengthPoints", 2));
    TS_ASSERT_THROWS_NOTHING(alg->execute(););
    TS_ASSERT(alg->isExecuted());

    if (alg->isExecuted()) {
      Mantid::API::WorkspaceGroup_sptr output =
          Mantid::API::AnalysisDataService::Instance().retrieveWS<Mantid::API::WorkspaceGroup>("MuscatResults");
      Mantid::API::Workspace_sptr wsPtr1 = output->getItem("Scatter_1");
      auto singleScatterResult = std::dynamic_pointer_cast<Mantid::API::MatrixWorkspace>(wsPtr1);
      Mantid::API::Workspace_sptr wsPtr2 = output->getItem("Scatter_2");
      auto doubleScatterResult = std::dynamic_pointer_cast<Mantid::API::MatrixWorkspace>(wsPtr2);
      // check single scatter result still matches analytical result
      const int SPECTRUMINDEXTOTEST = 1;
      const auto &mat = inputWorkspace->sample().getMaterial();
      const auto twoTheta = inputWorkspace->spectrumInfo().twoTheta(SPECTRUMINDEXTOTEST);
      const auto xPoints = singleScatterResult->histogram(SPECTRUMINDEXTOTEST).points();
      const double analyticResult1 = calculateFlatPlateAnalyticalResult(xPoints[0], mat, twoTheta, THICKNESS);
      const double analyticResult2 = calculateFlatPlateAnalyticalResult(xPoints[2], mat, twoTheta, THICKNESS);
      const auto &singleScatterY = singleScatterResult->y(SPECTRUMINDEXTOTEST);
      const double delta(1e-05);
      TS_ASSERT_DELTA(singleScatterY[0], analyticResult1, delta);
      TS_ASSERT_DELTA(singleScatterY[2], analyticResult2, delta);
      // check interpolated point is somewhere in between
      TS_ASSERT(singleScatterY[1] < analyticResult1 || singleScatterY[1] < analyticResult2);
      TS_ASSERT(singleScatterY[1] > analyticResult1 || singleScatterY[1] > analyticResult2);
      // no analytical result for double scatter so just check against current result that we assume is correct
      auto doubleScatterY = doubleScatterResult->y(SPECTRUMINDEXTOTEST);
      TS_ASSERT_DELTA(doubleScatterY[0], 0.001977, delta);
      TS_ASSERT_DELTA(doubleScatterY[2], 0.001819, delta);
      TS_ASSERT(doubleScatterY[1] < 0.001977 || doubleScatterY[1] < 0.001819);
      TS_ASSERT(doubleScatterY[1] > 0.001977 || doubleScatterY[1] > 0.001819);
      Mantid::API::AnalysisDataService::Instance().deepRemoveGroup("MuscatResults");
    }
  }

  void test_SparseInstrument() {
    // set up instrument with five detectors at different latitudes (=5 different rows)
    // run simulation for detectors at latitude=0 and 2 degrees and interpolate at lat=1 degree
    const double THICKNESS = 0.001; // metres
    auto inputWorkspace = SetupFlatPlateWorkspace(5, 2, 1, THICKNESS);

    auto alg = createAlgorithm();
    TS_ASSERT_THROWS_NOTHING(alg->setProperty("InputWorkspace", inputWorkspace));
    const int NSCATTERINGS = 2;
    TS_ASSERT_THROWS_NOTHING(alg->setProperty("NumberScatterings", NSCATTERINGS));
    TS_ASSERT_THROWS_NOTHING(alg->setProperty("NeutronPathsSingle", 10000));
    TS_ASSERT_THROWS_NOTHING(alg->setProperty("NeutronPathsMultiple", 10000));
    TS_ASSERT_THROWS_NOTHING(alg->setProperty("SparseInstrument", true));
    TS_ASSERT_THROWS_NOTHING(alg->setProperty("NumberOfDetectorRows", 3));
    TS_ASSERT_THROWS_NOTHING(alg->setProperty("NumberOfDetectorColumns", 2));
    TS_ASSERT_THROWS_NOTHING(alg->execute(););
    TS_ASSERT(alg->isExecuted());

    if (alg->isExecuted()) {
      Mantid::API::WorkspaceGroup_sptr output =
          Mantid::API::AnalysisDataService::Instance().retrieveWS<Mantid::API::WorkspaceGroup>("MuscatResults");
      Mantid::API::Workspace_sptr wsPtr1 = output->getItem("Scatter_1");
      auto singleScatterResult = std::dynamic_pointer_cast<Mantid::API::MatrixWorkspace>(wsPtr1);
      // check single scatter result still matches analytical result
      const auto &mat = inputWorkspace->sample().getMaterial();
      auto twoTheta = inputWorkspace->spectrumInfo().twoTheta(0);
      auto xPoints = singleScatterResult->histogram(0).points();
      const double analyticResult1 = calculateFlatPlateAnalyticalResult(xPoints[0], mat, twoTheta, THICKNESS);
      twoTheta = inputWorkspace->spectrumInfo().twoTheta(4);
      xPoints = singleScatterResult->histogram(4).points();
      const double analyticResult2 = calculateFlatPlateAnalyticalResult(xPoints[0], mat, twoTheta, THICKNESS);
      const double delta(1e-05);
      auto singleScatterYLatZero = singleScatterResult->y(0)[0];
      TS_ASSERT_DELTA(singleScatterYLatZero, analyticResult1, delta);
      auto singleScatterYLatTwo = singleScatterResult->y(4)[0];
      TS_ASSERT_DELTA(singleScatterYLatTwo, analyticResult2, delta);
      // check interpolated result at lat=1 degree is in between the results at lat=0 and 2 degrees
      auto interpSingleScatterY = singleScatterResult->y(2)[0];
      TS_ASSERT(interpSingleScatterY < singleScatterYLatZero || interpSingleScatterY < singleScatterYLatTwo);
      TS_ASSERT(interpSingleScatterY > singleScatterYLatZero || interpSingleScatterY > singleScatterYLatTwo);
      Mantid::API::AnalysisDataService::Instance().deepRemoveGroup("MuscatResults");
    }
  }

  void test_interpolateGaussian() {
    CalculateMultipleScatteringHelper alg;
    const int NBINS = 10;

    auto ws2 = WorkspaceCreationHelper::create2DWorkspacePoints(1, NBINS, 0.5);
    for (auto i = 0; i < 4; i++) {
      ws2->mutableY(0)[i] = pow(2 * i, 2);
    }
    auto interpY = alg.interpolateGaussian(ws2->histogram(0), 2.0);
    TS_ASSERT_EQUALS(interpY, exp(9.0));

    // check point beyond half way point uses same three points
    interpY = alg.interpolateGaussian(ws2->histogram(0), 2.00000001);
    TS_ASSERT_DELTA(interpY, exp(9.0), 0.01);
  }

  void test_updateTrackDirection() {
    CalculateMultipleScatteringHelper alg;
    const double twoTheta = M_PI * 60. / 180.;
    const double cosTwoTheta = cos(twoTheta);
    const double sinTwoTheta = sin(twoTheta);
    const double phi = M_PI;
    Mantid::Geometry::Track track(V3D(0, 0, 0), V3D(0.0, 0.0, 1.0));
    alg.updateTrackDirection(track, cosTwoTheta, phi);
    TS_ASSERT_EQUALS(track.direction(), V3D(0.0, -sinTwoTheta, cosTwoTheta));
    // special case of track going vertically
    Mantid::Geometry::Track trackUp(V3D(0, 0, 0), V3D(0.0, 1.0, 0.0));
    alg.updateTrackDirection(trackUp, cosTwoTheta, phi);
  }

  //---------------------------------------------------------------------------
  // Failure cases
  //---------------------------------------------------------------------------

  void test_invalidSOfQ() {
    CalculateMultipleScatteringHelper alg;
    const double THICKNESS = 0.001; // metres
    auto inputWorkspace = SetupFlatPlateWorkspace(5, 2, 1, THICKNESS);
    auto SofQWorkspaceTwoSp = WorkspaceCreationHelper::create2DWorkspace(2, 1);
    SofQWorkspaceTwoSp->mutableY(0)[0] = 1.;
    SofQWorkspaceTwoSp->getAxis(0)->unit() = UnitFactory::Instance().create("MomentumTransfer");
    alg.initialize();
    TS_ASSERT_THROWS_NOTHING(alg.setProperty("InputWorkspace", inputWorkspace));
    TS_ASSERT_THROWS_NOTHING(alg.setProperty("SofqWorkspace", SofQWorkspaceTwoSp));
    const int NSCATTERINGS = 2;
    TS_ASSERT_THROWS_NOTHING(alg.setProperty("NumberScatterings", NSCATTERINGS));
    TS_ASSERT_THROWS_NOTHING(alg.setProperty("NeutronPathsSingle", 1));
    TS_ASSERT_THROWS_NOTHING(alg.setProperty("NeutronPathsMultiple", 1));
    TS_ASSERT_THROWS_NOTHING(alg.setProperty("OutputWorkspace", "MuscatResults"));
    TS_ASSERT_THROWS(alg.execute(), const std::runtime_error &);

    auto SofQWorkspaceNegative = WorkspaceCreationHelper::create2DWorkspace(1, 1);
    SofQWorkspaceNegative->mutableY(0)[0] = -1.0;
    SofQWorkspaceNegative->getAxis(0)->unit() = UnitFactory::Instance().create("MomentumTransfer");
    TS_ASSERT_THROWS_NOTHING(alg.initialize());
    TS_ASSERT_THROWS_NOTHING(alg.setProperty("InputWorkspace", inputWorkspace));
    TS_ASSERT_THROWS_NOTHING(alg.setProperty("SofqWorkspace", SofQWorkspaceNegative));
    TS_ASSERT_THROWS_NOTHING(alg.setProperty("NumberScatterings", NSCATTERINGS));
    TS_ASSERT_THROWS_NOTHING(alg.setProperty("NeutronPathsSingle", 1));
    TS_ASSERT_THROWS_NOTHING(alg.setProperty("NeutronPathsMultiple", 1));
    TS_ASSERT_THROWS_NOTHING(alg.setProperty("OutputWorkspace", "MuscatResults"));
    TS_ASSERT_THROWS(alg.execute(), const std::runtime_error &);
  }

  void test_cant_run_withAlwaysStoreInADS_false() {
    const double THICKNESS = 0.001; // metres
    CalculateMultipleScatteringHelper alg;
    alg.setAlwaysStoreInADS(false);
    alg.setRethrows(true);
    alg.initialize();
    auto inputWorkspace = SetupFlatPlateWorkspace(5, 2, 1, THICKNESS);
    alg.setProperty("InputWorkspace", inputWorkspace);
    alg.setProperty("SofqWorkspace", SofQWorkspace);
    alg.setPropertyValue("OutputWorkspace", "MuscatResults");
    TS_ASSERT_THROWS(alg.execute(), const std::runtime_error &);
  }

private:
  Mantid::API::IAlgorithm_sptr createAlgorithm() {
    using Mantid::Algorithms::CalculateMultipleScattering;
    using Mantid::API::IAlgorithm;
    auto alg = std::make_shared<CalculateMultipleScattering>();
    alg->initialize();
    alg->setRethrows(true);
    TS_ASSERT(alg->isInitialized());
    TS_ASSERT_THROWS_NOTHING(alg->setProperty("SofqWorkspace", SofQWorkspace));
    alg->setPropertyValue("OutputWorkspace", "MuscatResults");
    return alg;
  }

  Mantid::API::MatrixWorkspace_sptr SetupFlatPlateWorkspace(const int nlat, const int nlong, const int nbins,
                                                            const double thickness) {
    Mantid::API::MatrixWorkspace_sptr inputWorkspace =
        WorkspaceCreationHelper::create2DWorkspaceBinned(nlat * nlong, nbins, 0.5 /*x0*/);
    inputWorkspace->getAxis(0)->unit() = UnitFactory::Instance().create("Wavelength");
    V3D samplePosition(0., 0., 0.);
    V3D sourcePosition(0., 0., -14.);

    Instrument_sptr instrument = std::make_shared<Instrument>();
    instrument->setReferenceFrame(
        std::make_shared<ReferenceFrame>(Mantid::Geometry::Y, Mantid::Geometry::Z, Right, "0,0,0"));

    InstrumentCreationHelper::addSource(instrument, sourcePosition, "source");
    InstrumentCreationHelper::addSample(instrument, samplePosition, "sample");

    // set up detectors with one degree spacing in latitude and longitude (to match geographical angles
    // approach used in the spatial interpolation\sparse instrument functionality)
    int i = 0;
    constexpr double deg2rad = M_PI / 180.0;
    auto R = 1.0;
    for (int lat = 0; lat < nlat; ++lat) {
      for (int lng = 0; lng < nlong; ++lng) {
        std::stringstream buffer;
        buffer << "detector_" << i;
        V3D detPos;
        auto latrad = lat * deg2rad;
        auto longrad = lng * deg2rad;
        detPos[1] = R * sin(latrad);
        const double ct = R * cos(latrad);
        detPos[2] = ct * cos(longrad);
        detPos[0] = ct * sin(longrad);

        InstrumentCreationHelper::addDetector(instrument, detPos, i, buffer.str());
        // Link it to the workspace
        inputWorkspace->getSpectrum(i).addDetectorID(i);
        i++;
      }
    }
    inputWorkspace->setInstrument(instrument);

    // create flat plate that is 1mm thick
    auto flatPlateShape = ComponentCreationHelper::createCuboid((10 * thickness) / 2, (10 * thickness) / 2,
                                                                thickness / 2, 0, V3D{0, 0, 1});
    auto mat = Mantid::Kernel::Material("Ni", Mantid::PhysicalConstants::getNeutronAtom(28, 0), 0.091337537);
    flatPlateShape->setMaterial(mat);
    inputWorkspace->mutableSample().setShape(flatPlateShape);
    return inputWorkspace;
  }

  double calculateFlatPlateAnalyticalResult(const double wavelength, const Mantid::Kernel::Material &mat,
                                            const double twoTheta, const double thickness) {
    const double totalXSection = mat.totalScatterXSection() + mat.absorbXSection(wavelength);
    const double alpha = mat.absorbXSection(wavelength) / totalXSection;
    const double mfp = 0.01 / (mat.numberDensity() * totalXSection);
    const double tau = thickness / mfp;
    const double secangle = 1 / cos(twoTheta);
    if (secangle == 1.)
      return (1 - alpha) * tau * exp(-tau) / (4 * M_PI);
    else
      return (1 - alpha) * (exp(-tau * secangle) - exp(-tau)) / (4 * M_PI * (1 - secangle));
  }
  Mantid::API::MatrixWorkspace_sptr SofQWorkspace;
};