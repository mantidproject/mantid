// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#ifndef MANTID_DATAHANDLING_EXTRACTPOLARIZATIONEFFICIENCIESTEST_H_
#define MANTID_DATAHANDLING_EXTRACTPOLARIZATIONEFFICIENCIESTEST_H_

#include <cxxtest/TestSuite.h>

#include "MantidAPI/Axis.h"
#include "MantidDataHandling/ExtractPolarizationEfficiencies.h"
#include "MantidDataHandling/LoadInstrument.h"
#include "MantidDataHandling/LoadParameterFile.h"
#include "MantidDataObjects/Workspace2D.h"
#include "MantidGeometry/Instrument.h"
#include "MantidGeometry/Instrument/ParameterMap.h"
#include "MantidHistogramData/Counts.h"
#include "MantidHistogramData/LinearGenerator.h"
#include "MantidHistogramData/Points.h"
#include "MantidKernel/OptionalBool.h"
#include "MantidKernel/Unit.h"
#include "MantidTestHelpers/WorkspaceCreationHelper.h"

using namespace Mantid::API;
using namespace Mantid::DataHandling;
using namespace Mantid::DataObjects;
using namespace Mantid::Geometry;
using namespace Mantid::HistogramData;
using namespace Mantid::Kernel;

class ExtractPolarizationEfficienciesTest : public CxxTest::TestSuite {
public:
  // This pair of boilerplate methods prevent the suite being created statically
  // This means the constructor isn't called when running other tests
  static ExtractPolarizationEfficienciesTest *createSuite() {
    return new ExtractPolarizationEfficienciesTest();
  }
  static void destroySuite(ExtractPolarizationEfficienciesTest *suite) {
    delete suite;
  }

  void test_init() {
    ExtractPolarizationEfficiencies alg;
    TS_ASSERT_THROWS_NOTHING(alg.initialize());
    TS_ASSERT(alg.isInitialized());
  }

  void test_no_instrument() {
    auto workspace = createPointWS(1, 0, 10);
    ExtractPolarizationEfficiencies alg;
    alg.initialize();
    alg.setChild(true);
    alg.setRethrows(true);
    alg.setProperty("InputWorkspace", workspace);
    alg.setProperty("OutputWorkspace", "dummy");
    TS_ASSERT_THROWS(alg.execute(), std::invalid_argument);
  }

  void test_wrong_method() {
    auto workspace = createInputWorkspace("Einstein");
    ExtractPolarizationEfficiencies alg;
    alg.initialize();
    alg.setChild(true);
    alg.setRethrows(true);
    alg.setProperty("InputWorkspace", workspace);
    alg.setProperty("OutputWorkspace", "dummy");
    TS_ASSERT_THROWS(alg.execute(), std::invalid_argument);
  }

  void test_no_lambda() {
    auto lambda = "";
    auto workspace = createInputWorkspace("Wildes", lambda);
    ExtractPolarizationEfficiencies alg;
    alg.initialize();
    alg.setChild(true);
    alg.setRethrows(true);
    alg.setProperty("InputWorkspace", workspace);
    alg.setProperty("OutputWorkspace", "dummy");
    TS_ASSERT_THROWS(alg.execute(), std::invalid_argument);
  }

  void test_space_sep() {
    auto lambda = "1 2 3 4";
    auto workspace = createInputWorkspace("Wildes", lambda);
    ExtractPolarizationEfficiencies alg;
    alg.initialize();
    alg.setChild(true);
    alg.setRethrows(true);
    alg.setProperty("InputWorkspace", workspace);
    alg.setProperty("OutputWorkspace", "dummy");
    alg.execute();
    MatrixWorkspace_sptr outWS = alg.getProperty("OutputWorkspace");

    TS_ASSERT(outWS);
    TS_ASSERT_EQUALS(outWS->getNumberHistograms(), 4);
    TS_ASSERT_EQUALS(outWS->blocksize(), 4);
    TS_ASSERT_DELTA(outWS->x(0)[0], 1.0, 1e-14);
    TS_ASSERT_DELTA(outWS->x(0)[1], 2.0, 1e-14);
    TS_ASSERT_DELTA(outWS->x(0)[2], 3.0, 1e-14);
    TS_ASSERT_DELTA(outWS->x(0)[3], 4.0, 1e-14);
  }

  void test_double_space_sep() {
    auto lambda = " 1  2  3  4 ";
    auto workspace = createInputWorkspace("Wildes", lambda);
    ExtractPolarizationEfficiencies alg;
    alg.initialize();
    alg.setChild(true);
    alg.setRethrows(true);
    alg.setProperty("InputWorkspace", workspace);
    alg.setProperty("OutputWorkspace", "dummy");
    alg.execute();
    MatrixWorkspace_sptr outWS = alg.getProperty("OutputWorkspace");

    TS_ASSERT(outWS);
    TS_ASSERT_EQUALS(outWS->getNumberHistograms(), 4);
    TS_ASSERT_EQUALS(outWS->blocksize(), 4);
    TS_ASSERT_DELTA(outWS->x(0)[0], 1.0, 1e-14);
    TS_ASSERT_DELTA(outWS->x(0)[1], 2.0, 1e-14);
    TS_ASSERT_DELTA(outWS->x(0)[2], 3.0, 1e-14);
    TS_ASSERT_DELTA(outWS->x(0)[3], 4.0, 1e-14);
  }

  void test_comma_space_sep() {
    auto lambda = "1, 2, 3, 4";
    auto workspace = createInputWorkspace("Wildes", lambda);
    ExtractPolarizationEfficiencies alg;
    alg.initialize();
    alg.setChild(true);
    alg.setRethrows(true);
    alg.setProperty("InputWorkspace", workspace);
    alg.setProperty("OutputWorkspace", "dummy");
    TS_ASSERT_THROWS(alg.execute(), std::invalid_argument);
  }

  void test_non_number() {
    auto lambda = "one two three four";
    auto workspace = createInputWorkspace("Wildes", lambda);
    ExtractPolarizationEfficiencies alg;
    alg.initialize();
    alg.setChild(true);
    alg.setRethrows(true);
    alg.setProperty("InputWorkspace", workspace);
    alg.setProperty("OutputWorkspace", "dummy");
    TS_ASSERT_THROWS(alg.execute(), std::invalid_argument);
  }

  void test_new_line_sep() {
    auto lambda = "1\n 2\n 3\n 4";
    auto workspace = createInputWorkspace("Wildes", lambda);
    ExtractPolarizationEfficiencies alg;
    alg.initialize();
    alg.setChild(true);
    alg.setRethrows(true);
    alg.setProperty("InputWorkspace", workspace);
    alg.setProperty("OutputWorkspace", "dummy");
    alg.execute();
    MatrixWorkspace_sptr outWS = alg.getProperty("OutputWorkspace");

    TS_ASSERT(outWS);
    TS_ASSERT_EQUALS(outWS->getNumberHistograms(), 4);
    TS_ASSERT_EQUALS(outWS->blocksize(), 4);
    TS_ASSERT_DELTA(outWS->x(0)[0], 1.0, 1e-14);
    TS_ASSERT_DELTA(outWS->x(0)[1], 2.0, 1e-14);
    TS_ASSERT_DELTA(outWS->x(0)[2], 3.0, 1e-14);
    TS_ASSERT_DELTA(outWS->x(0)[3], 4.0, 1e-14);
  }

  void test_missing_P1() {
    auto lambda = "1 2 3 4";
    bool skipP1 = true;
    auto workspace = createInputWorkspace("Wildes", lambda, skipP1);
    ExtractPolarizationEfficiencies alg;
    alg.initialize();
    alg.setChild(true);
    alg.setRethrows(true);
    alg.setProperty("InputWorkspace", workspace);
    alg.setProperty("OutputWorkspace", "dummy");
    TS_ASSERT_THROWS(alg.execute(), std::invalid_argument);
  }

  void test_Fredrikze() {
    auto workspace = createInputWorkspace("Fredrikze");

    ExtractPolarizationEfficiencies alg;
    alg.initialize();
    alg.setChild(true);
    alg.setRethrows(true);
    alg.setProperty("InputWorkspace", workspace);
    alg.setProperty("OutputWorkspace", "dummy");
    alg.execute();
    MatrixWorkspace_sptr outWS = alg.getProperty("OutputWorkspace");

    TS_ASSERT(outWS);
    TS_ASSERT_EQUALS(outWS->getNumberHistograms(), 4);
    TS_ASSERT_EQUALS(outWS->blocksize(), 4);
    TS_ASSERT_EQUALS(outWS->getAxis(0)->unit()->caption(), "Wavelength");

    auto axis1 = outWS->getAxis(1);
    TS_ASSERT_EQUALS(axis1->label(0), "Pp");
    TS_ASSERT_EQUALS(axis1->label(1), "Ap");
    TS_ASSERT_EQUALS(axis1->label(2), "Rho");
    TS_ASSERT_EQUALS(axis1->label(3), "Alpha");

    TS_ASSERT(!outWS->isHistogramData());

    TS_ASSERT_DELTA(outWS->x(0)[0], 1.0, 1e-14);
    TS_ASSERT_DELTA(outWS->x(0)[1], 2.0, 1e-14);
    TS_ASSERT_DELTA(outWS->x(0)[2], 3.0, 1e-14);
    TS_ASSERT_DELTA(outWS->x(0)[3], 4.0, 1e-14);

    TS_ASSERT_DELTA(outWS->y(0)[0], 0.991, 1e-14);
    TS_ASSERT_DELTA(outWS->y(0)[1], 0.992, 1e-14);
    TS_ASSERT_DELTA(outWS->y(0)[2], 0.993, 1e-14);
    TS_ASSERT_DELTA(outWS->y(0)[3], 0.994, 1e-14);

    TS_ASSERT_DELTA(outWS->e(0)[0], 0.0, 1e-14);
    TS_ASSERT_DELTA(outWS->e(0)[1], 0.0, 1e-14);
    TS_ASSERT_DELTA(outWS->e(0)[2], 0.0, 1e-14);
    TS_ASSERT_DELTA(outWS->e(0)[3], 0.0, 1e-14);

    TS_ASSERT_DELTA(outWS->y(1)[0], 0.981, 1e-14);
    TS_ASSERT_DELTA(outWS->y(1)[1], 0.982, 1e-14);
    TS_ASSERT_DELTA(outWS->y(1)[2], 0.983, 1e-14);
    TS_ASSERT_DELTA(outWS->y(1)[3], 0.984, 1e-14);

    TS_ASSERT_DELTA(outWS->e(1)[0], 0.0, 1e-14);
    TS_ASSERT_DELTA(outWS->e(1)[1], 0.0, 1e-14);
    TS_ASSERT_DELTA(outWS->e(1)[2], 0.0, 1e-14);
    TS_ASSERT_DELTA(outWS->e(1)[3], 0.0, 1e-14);

    TS_ASSERT_DELTA(outWS->y(2)[0], 0.971, 1e-14);
    TS_ASSERT_DELTA(outWS->y(2)[1], 0.972, 1e-14);
    TS_ASSERT_DELTA(outWS->y(2)[2], 0.973, 1e-14);
    TS_ASSERT_DELTA(outWS->y(2)[3], 0.974, 1e-14);

    TS_ASSERT_DELTA(outWS->e(2)[0], 0.0, 1e-14);
    TS_ASSERT_DELTA(outWS->e(2)[1], 0.0, 1e-14);
    TS_ASSERT_DELTA(outWS->e(2)[2], 0.0, 1e-14);
    TS_ASSERT_DELTA(outWS->e(2)[3], 0.0, 1e-14);

    TS_ASSERT_DELTA(outWS->y(3)[0], 0.961, 1e-14);
    TS_ASSERT_DELTA(outWS->y(3)[1], 0.962, 1e-14);
    TS_ASSERT_DELTA(outWS->y(3)[2], 0.963, 1e-14);
    TS_ASSERT_DELTA(outWS->y(3)[3], 0.964, 1e-14);

    TS_ASSERT_DELTA(outWS->e(3)[0], 0.0, 1e-14);
    TS_ASSERT_DELTA(outWS->e(3)[1], 0.0, 1e-14);
    TS_ASSERT_DELTA(outWS->e(3)[2], 0.0, 1e-14);
    TS_ASSERT_DELTA(outWS->e(3)[3], 0.0, 1e-14);
  }

  void test_Wildes() {
    auto workspace = createInputWorkspace("Wildes");

    ExtractPolarizationEfficiencies alg;
    alg.initialize();
    alg.setChild(true);
    alg.setRethrows(true);
    alg.setProperty("InputWorkspace", workspace);
    alg.setProperty("OutputWorkspace", "dummy");
    alg.execute();
    MatrixWorkspace_sptr outWS = alg.getProperty("OutputWorkspace");

    TS_ASSERT(outWS);
    TS_ASSERT_EQUALS(outWS->getNumberHistograms(), 4);
    TS_ASSERT_EQUALS(outWS->blocksize(), 4);
    TS_ASSERT_EQUALS(outWS->getAxis(0)->unit()->caption(), "Wavelength");

    auto axis1 = outWS->getAxis(1);
    TS_ASSERT_EQUALS(axis1->label(0), "P1");
    TS_ASSERT_EQUALS(axis1->label(1), "P2");
    TS_ASSERT_EQUALS(axis1->label(2), "F1");
    TS_ASSERT_EQUALS(axis1->label(3), "F2");

    TS_ASSERT(!outWS->isHistogramData());

    TS_ASSERT_DELTA(outWS->x(0)[0], 1.0, 1e-14);
    TS_ASSERT_DELTA(outWS->x(0)[1], 2.0, 1e-14);
    TS_ASSERT_DELTA(outWS->x(0)[2], 3.0, 1e-14);
    TS_ASSERT_DELTA(outWS->x(0)[3], 4.0, 1e-14);

    TS_ASSERT_DELTA(outWS->y(0)[0], 0.991, 1e-14);
    TS_ASSERT_DELTA(outWS->y(0)[1], 0.992, 1e-14);
    TS_ASSERT_DELTA(outWS->y(0)[2], 0.993, 1e-14);
    TS_ASSERT_DELTA(outWS->y(0)[3], 0.994, 1e-14);

    TS_ASSERT_DELTA(outWS->e(0)[0], 0.0, 1e-14);
    TS_ASSERT_DELTA(outWS->e(0)[1], 0.0, 1e-14);
    TS_ASSERT_DELTA(outWS->e(0)[2], 0.0, 1e-14);
    TS_ASSERT_DELTA(outWS->e(0)[3], 0.0, 1e-14);

    TS_ASSERT_DELTA(outWS->y(1)[0], 0.981, 1e-14);
    TS_ASSERT_DELTA(outWS->y(1)[1], 0.982, 1e-14);
    TS_ASSERT_DELTA(outWS->y(1)[2], 0.983, 1e-14);
    TS_ASSERT_DELTA(outWS->y(1)[3], 0.984, 1e-14);

    TS_ASSERT_DELTA(outWS->e(1)[0], 0.0, 1e-14);
    TS_ASSERT_DELTA(outWS->e(1)[1], 0.0, 1e-14);
    TS_ASSERT_DELTA(outWS->e(1)[2], 0.0, 1e-14);
    TS_ASSERT_DELTA(outWS->e(1)[3], 0.0, 1e-14);

    TS_ASSERT_DELTA(outWS->y(2)[0], 0.971, 1e-14);
    TS_ASSERT_DELTA(outWS->y(2)[1], 0.972, 1e-14);
    TS_ASSERT_DELTA(outWS->y(2)[2], 0.973, 1e-14);
    TS_ASSERT_DELTA(outWS->y(2)[3], 0.974, 1e-14);

    TS_ASSERT_DELTA(outWS->e(2)[0], 0.0, 1e-14);
    TS_ASSERT_DELTA(outWS->e(2)[1], 0.0, 1e-14);
    TS_ASSERT_DELTA(outWS->e(2)[2], 0.0, 1e-14);
    TS_ASSERT_DELTA(outWS->e(2)[3], 0.0, 1e-14);

    TS_ASSERT_DELTA(outWS->y(3)[0], 0.961, 1e-14);
    TS_ASSERT_DELTA(outWS->y(3)[1], 0.962, 1e-14);
    TS_ASSERT_DELTA(outWS->y(3)[2], 0.963, 1e-14);
    TS_ASSERT_DELTA(outWS->y(3)[3], 0.964, 1e-14);

    TS_ASSERT_DELTA(outWS->e(3)[0], 0.0, 1e-14);
    TS_ASSERT_DELTA(outWS->e(3)[1], 0.0, 1e-14);
    TS_ASSERT_DELTA(outWS->e(3)[2], 0.0, 1e-14);
    TS_ASSERT_DELTA(outWS->e(3)[3], 0.0, 1e-14);
  }

  void test_Wildes_errors() {
    auto workspace = createInputWorkspace("Wildes", "1 2 3 4", false, true);

    ExtractPolarizationEfficiencies alg;
    alg.initialize();
    alg.setChild(true);
    alg.setRethrows(true);
    alg.setProperty("InputWorkspace", workspace);
    alg.setProperty("OutputWorkspace", "dummy");
    alg.execute();
    MatrixWorkspace_sptr outWS = alg.getProperty("OutputWorkspace");

    TS_ASSERT(outWS);
    TS_ASSERT_EQUALS(outWS->getNumberHistograms(), 4);
    TS_ASSERT_EQUALS(outWS->blocksize(), 4);
    TS_ASSERT_EQUALS(outWS->getAxis(0)->unit()->caption(), "Wavelength");

    auto axis1 = outWS->getAxis(1);
    TS_ASSERT_EQUALS(axis1->label(0), "P1");
    TS_ASSERT_EQUALS(axis1->label(1), "P2");
    TS_ASSERT_EQUALS(axis1->label(2), "F1");
    TS_ASSERT_EQUALS(axis1->label(3), "F2");

    TS_ASSERT(!outWS->isHistogramData());

    TS_ASSERT_DELTA(outWS->x(0)[0], 1.0, 1e-14);
    TS_ASSERT_DELTA(outWS->x(0)[1], 2.0, 1e-14);
    TS_ASSERT_DELTA(outWS->x(0)[2], 3.0, 1e-14);
    TS_ASSERT_DELTA(outWS->x(0)[3], 4.0, 1e-14);

    TS_ASSERT_DELTA(outWS->y(0)[0], 0.991, 1e-14);
    TS_ASSERT_DELTA(outWS->y(0)[1], 0.992, 1e-14);
    TS_ASSERT_DELTA(outWS->y(0)[2], 0.993, 1e-14);
    TS_ASSERT_DELTA(outWS->y(0)[3], 0.994, 1e-14);

    TS_ASSERT_DELTA(outWS->e(0)[0], 0.1, 1e-14);
    TS_ASSERT_DELTA(outWS->e(0)[1], 0.2, 1e-14);
    TS_ASSERT_DELTA(outWS->e(0)[2], 0.3, 1e-14);
    TS_ASSERT_DELTA(outWS->e(0)[3], 0.4, 1e-14);

    TS_ASSERT_DELTA(outWS->y(1)[0], 0.981, 1e-14);
    TS_ASSERT_DELTA(outWS->y(1)[1], 0.982, 1e-14);
    TS_ASSERT_DELTA(outWS->y(1)[2], 0.983, 1e-14);
    TS_ASSERT_DELTA(outWS->y(1)[3], 0.984, 1e-14);

    TS_ASSERT_DELTA(outWS->e(1)[0], 0.11, 1e-14);
    TS_ASSERT_DELTA(outWS->e(1)[1], 0.21, 1e-14);
    TS_ASSERT_DELTA(outWS->e(1)[2], 0.31, 1e-14);
    TS_ASSERT_DELTA(outWS->e(1)[3], 0.41, 1e-14);

    TS_ASSERT_DELTA(outWS->y(2)[0], 0.971, 1e-14);
    TS_ASSERT_DELTA(outWS->y(2)[1], 0.972, 1e-14);
    TS_ASSERT_DELTA(outWS->y(2)[2], 0.973, 1e-14);
    TS_ASSERT_DELTA(outWS->y(2)[3], 0.974, 1e-14);

    TS_ASSERT_DELTA(outWS->e(2)[0], 0.12, 1e-14);
    TS_ASSERT_DELTA(outWS->e(2)[1], 0.22, 1e-14);
    TS_ASSERT_DELTA(outWS->e(2)[2], 0.32, 1e-14);
    TS_ASSERT_DELTA(outWS->e(2)[3], 0.42, 1e-14);

    TS_ASSERT_DELTA(outWS->y(3)[0], 0.961, 1e-14);
    TS_ASSERT_DELTA(outWS->y(3)[1], 0.962, 1e-14);
    TS_ASSERT_DELTA(outWS->y(3)[2], 0.963, 1e-14);
    TS_ASSERT_DELTA(outWS->y(3)[3], 0.964, 1e-14);

    TS_ASSERT_DELTA(outWS->e(3)[0], 0.13, 1e-14);
    TS_ASSERT_DELTA(outWS->e(3)[1], 0.23, 1e-14);
    TS_ASSERT_DELTA(outWS->e(3)[2], 0.33, 1e-14);
    TS_ASSERT_DELTA(outWS->e(3)[3], 0.43, 1e-14);
  }

  void test_loading_from_file() {
    auto workspace = createPointWS(1, 0, 10);
    LoadInstrument loader;
    loader.initialize();
    loader.setPropertyValue("Filename", "unit_testing/REFL_Definition.xml");
    loader.setProperty("Workspace", workspace);
    loader.setProperty("RewriteSpectraMap", OptionalBool(true));
    loader.execute();
    LoadParameterFile paramLoader;
    paramLoader.initialize();
    paramLoader.setPropertyValue("Filename",
                                 "unit_testing/REFL_Parameters_Fredrikze.xml");
    paramLoader.setProperty("Workspace", workspace);
    paramLoader.execute();

    ExtractPolarizationEfficiencies alg;
    alg.initialize();
    alg.setChild(true);
    alg.setRethrows(true);
    alg.setProperty("InputWorkspace", workspace);
    alg.setProperty("OutputWorkspace", "dummy");
    alg.execute();
    MatrixWorkspace_sptr outWS = alg.getProperty("OutputWorkspace");

    TS_ASSERT(outWS);
    TS_ASSERT_EQUALS(outWS->getNumberHistograms(), 4);
    TS_ASSERT_EQUALS(outWS->blocksize(), 6);
    TS_ASSERT_EQUALS(outWS->getAxis(0)->unit()->caption(), "Wavelength");

    auto axis1 = outWS->getAxis(1);
    TS_ASSERT_EQUALS(axis1->label(0), "Pp");
    TS_ASSERT_EQUALS(axis1->label(1), "Ap");
    TS_ASSERT_EQUALS(axis1->label(2), "Rho");
    TS_ASSERT_EQUALS(axis1->label(3), "Alpha");

    TS_ASSERT(!outWS->isHistogramData());

    TS_ASSERT_DELTA(outWS->x(0)[0], 0.0, 1e-14);
    TS_ASSERT_DELTA(outWS->x(0)[1], 3.0, 1e-14);
    TS_ASSERT_DELTA(outWS->x(0)[2], 6.0, 1e-14);
    TS_ASSERT_DELTA(outWS->x(0)[3], 10.0, 1e-14);
    TS_ASSERT_DELTA(outWS->x(0)[4], 15.0, 1e-14);
    TS_ASSERT_DELTA(outWS->x(0)[5], 20.0, 1e-14);

    TS_ASSERT_DELTA(outWS->y(0)[0], 0.9, 1e-14);
    TS_ASSERT_DELTA(outWS->y(0)[1], 0.9, 1e-14);
    TS_ASSERT_DELTA(outWS->y(0)[2], 0.9, 1e-14);
    TS_ASSERT_DELTA(outWS->y(0)[3], 0.9, 1e-14);
    TS_ASSERT_DELTA(outWS->y(0)[4], 0.9, 1e-14);
    TS_ASSERT_DELTA(outWS->y(0)[5], 0.9, 1e-14);

    TS_ASSERT_DELTA(outWS->y(1)[0], 0.8, 1e-14);
    TS_ASSERT_DELTA(outWS->y(1)[1], 0.8, 1e-14);
    TS_ASSERT_DELTA(outWS->y(1)[2], 0.8, 1e-14);
    TS_ASSERT_DELTA(outWS->y(1)[3], 0.8, 1e-14);
    TS_ASSERT_DELTA(outWS->y(1)[4], 0.8, 1e-14);
    TS_ASSERT_DELTA(outWS->y(1)[5], 0.8, 1e-14);

    TS_ASSERT_DELTA(outWS->y(2)[0], 0.778, 1e-14);
    TS_ASSERT_DELTA(outWS->y(2)[1], 0.778, 1e-14);
    TS_ASSERT_DELTA(outWS->y(2)[2], 0.778, 1e-14);
    TS_ASSERT_DELTA(outWS->y(2)[3], 0.778, 1e-14);
    TS_ASSERT_DELTA(outWS->y(2)[4], 0.778, 1e-14);
    TS_ASSERT_DELTA(outWS->y(2)[5], 0.778, 1e-14);

    TS_ASSERT_DELTA(outWS->y(3)[0], 0.75, 1e-14);
    TS_ASSERT_DELTA(outWS->y(3)[1], 0.75, 1e-14);
    TS_ASSERT_DELTA(outWS->y(3)[2], 0.75, 1e-14);
    TS_ASSERT_DELTA(outWS->y(3)[3], 0.75, 1e-14);
    TS_ASSERT_DELTA(outWS->y(3)[4], 0.75, 1e-14);
    TS_ASSERT_DELTA(outWS->y(3)[5], 0.75, 1e-14);
  }

  void test_short_lambda() {
    auto workspace = createInputWorkspace("Wildes", "0");

    ExtractPolarizationEfficiencies alg;
    alg.initialize();
    alg.setChild(true);
    alg.setRethrows(true);
    alg.setProperty("InputWorkspace", workspace);
    alg.setProperty("OutputWorkspace", "dummy");
    TS_ASSERT_THROWS_EQUALS(alg.execute(), std::runtime_error & e,
                            std::string(e.what()),
                            "Instrument vector parameter \"efficiency_lambda\" "
                            "must have at least 2 elements but it has 1");
  }

  void test_empty_lambda() {
    auto workspace = createInputWorkspace("Wildes", " ");

    ExtractPolarizationEfficiencies alg;
    alg.initialize();
    alg.setChild(true);
    alg.setRethrows(true);
    alg.setProperty("InputWorkspace", workspace);
    alg.setProperty("OutputWorkspace", "dummy");
    TS_ASSERT_THROWS_EQUALS(alg.execute(), std::runtime_error & e,
                            std::string(e.what()),
                            "Instrument vector parameter \"efficiency_lambda\" "
                            "must have at least 2 elements but it has 0");
  }

  void test_wrong_vector_size() {
    auto workspace = createInputWorkspace("Wildes", "1 2 3");

    ExtractPolarizationEfficiencies alg;
    alg.initialize();
    alg.setChild(true);
    alg.setRethrows(true);
    alg.setProperty("InputWorkspace", workspace);
    alg.setProperty("OutputWorkspace", "dummy");
    TS_ASSERT_THROWS_EQUALS(
        alg.execute(), std::runtime_error & e, std::string(e.what()),
        "Instrument vector parameter \"P1\" is expected to be the same size as "
        "\"efficiency_lambda\" but 4 != 3");
  }

private:
  MatrixWorkspace_sptr createPointWS(size_t size, double startX,
                                     double endX) const {
    double const dX = (endX - startX) / double(size - 1);
    Points xVals(size, LinearGenerator(startX, dX));
    Counts yVals(size, 1.0);
    auto retVal = boost::make_shared<Workspace2D>();
    retVal->initialize(1, Histogram(xVals, yVals));
    return retVal;
  }

  MatrixWorkspace_sptr
  createInputWorkspace(std::string const &method,
                       std::string const &lambda = "1 2 3 4",
                       bool skipP1 = false, bool loadErrors = false) {
    auto workspace = createPointWS(1, 0, 10);
    auto pmap = boost::make_shared<ParameterMap>();
    auto instrument = boost::make_shared<Instrument>();

    pmap->addString(instrument.get(), "polarization_correction_method", method);
    if (!lambda.empty()) {
      pmap->addString(instrument.get(), "efficiency_lambda", lambda);
    }
    if (method == "Fredrikze") {
      pmap->addString(instrument.get(), "polarization_correction_option", "PA");
      pmap->addString(instrument.get(), "Pp", "0.991 0.992 0.993 0.994");
      pmap->addString(instrument.get(), "Ap", "0.981 0.982 0.983 0.984");
      pmap->addString(instrument.get(), "Rho", "0.971 0.972 0.973 0.974");
      pmap->addString(instrument.get(), "Alpha", "0.961 0.962 0.963 0.964");
    } else {
      pmap->addString(instrument.get(), "polarization_correction_option",
                      "00,01,10,11");
      if (!skipP1) {
        pmap->addString(instrument.get(), "P1", "0.991 0.992 0.993 0.994");
        if (loadErrors) {
          pmap->addString(instrument.get(), "P1_Errors", "0.1 0.2 0.3 0.4");
        }
      }
      pmap->addString(instrument.get(), "P2", "0.981 0.982 0.983 0.984");
      pmap->addString(instrument.get(), "F1", "0.971 0.972 0.973 0.974");
      pmap->addString(instrument.get(), "F2", "0.961 0.962 0.963 0.964");
      if (loadErrors) {
        pmap->addString(instrument.get(), "P2_Errors", "0.11 0.21 0.31 0.41");
        pmap->addString(instrument.get(), "F1_Errors", "0.12 0.22 0.32 0.42");
        pmap->addString(instrument.get(), "F2_Errors", "0.13 0.23 0.33 0.43");
      }
    }

    instrument = boost::make_shared<Instrument>(instrument, pmap);
    workspace->setInstrument(instrument);

    return workspace;
  }
};

#endif /* MANTID_DATAHANDLING_EXTRACTPOLARIZATIONEFFICIENCIESTEST_H_ */
