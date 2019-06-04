// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#ifndef MANTID_DATAHANDLING_LOADISISPOLARIZATIONEFFICIENCIESTEST_H_
#define MANTID_DATAHANDLING_LOADISISPOLARIZATIONEFFICIENCIESTEST_H_

#include <cxxtest/TestSuite.h>

#include "MantidDataHandling/LoadISISPolarizationEfficiencies.h"

#include "MantidAPI/Axis.h"
#include "MantidAPI/MatrixWorkspace.h"
#include "MantidDataObjects/Workspace2D.h"
#include "MantidDataObjects/WorkspaceCreation.h"
#include "MantidHistogramData/BinEdges.h"
#include "MantidHistogramData/Counts.h"
#include "MantidHistogramData/LinearGenerator.h"
#include "MantidKernel/Unit.h"
#include "MantidTestHelpers/ScopedFileHelper.h"

#include <array>
#include <fstream>

using Mantid::DataHandling::LoadISISPolarizationEfficiencies;
using namespace Mantid::API;
using namespace Mantid::DataObjects;
using namespace Mantid::HistogramData;
using ScopedFileHelper::ScopedFile;

class LoadISISPolarizationEfficienciesTest : public CxxTest::TestSuite {
public:
  // This pair of boilerplate methods prevent the suite being created statically
  // This means the constructor isn't called when running other tests
  static LoadISISPolarizationEfficienciesTest *createSuite() {
    return new LoadISISPolarizationEfficienciesTest();
  }
  static void destroySuite(LoadISISPolarizationEfficienciesTest *suite) {
    delete suite;
  }

  void test_initialization() {
    LoadISISPolarizationEfficiencies alg;
    alg.setRethrows(true);
    TS_ASSERT_THROWS_NOTHING(alg.initialize())
    TS_ASSERT(alg.isInitialized())
  }

  void test_load() {
    ScopedFile f1(m_data1, "Efficiency1.txt");

    LoadISISPolarizationEfficiencies alg;
    alg.setChild(true);
    alg.setRethrows(true);
    alg.initialize();
    alg.setProperty("P1", f1.getFileName());
    alg.setProperty("P2", f1.getFileName());
    alg.setProperty("OutputWorkspace", "dummy");
    alg.execute();
    MatrixWorkspace_sptr outWS = alg.getProperty("OutputWorkspace");
    TS_ASSERT(outWS);
    TS_ASSERT_EQUALS(outWS->getNumberHistograms(), 2);
    TS_ASSERT_EQUALS(outWS->blocksize(), 5);
    TS_ASSERT_EQUALS(outWS->getAxis(0)->unit()->caption(), "Wavelength");

    auto axis1 = outWS->getAxis(1);
    TS_ASSERT_EQUALS(axis1->label(0), "P1");
    TS_ASSERT_EQUALS(axis1->label(1), "P2");

    TS_ASSERT(!outWS->isHistogramData());

    {
      auto const &x = outWS->x(0);
      auto const &y = outWS->y(0);
      TS_ASSERT_EQUALS(x.size(), 5);
      TS_ASSERT_EQUALS(y.size(), 5);
      TS_ASSERT_DELTA(x.front(), 1.1, 1e-15);
      TS_ASSERT_DELTA(x.back(), 5.5, 1e-15);
      TS_ASSERT_DELTA(y.front(), 1., 1e-15);
      TS_ASSERT_DELTA(y.back(), 1., 1e-15);
    }

    {
      auto const &x = outWS->x(1);
      auto const &y = outWS->y(1);
      TS_ASSERT_EQUALS(x.size(), 5);
      TS_ASSERT_EQUALS(y.size(), 5);
      TS_ASSERT_DELTA(x.front(), 1.1, 1e-15);
      TS_ASSERT_DELTA(x.back(), 5.5, 1e-15);
      TS_ASSERT_DELTA(y.front(), 1., 1e-15);
      TS_ASSERT_DELTA(y.back(), 1., 1e-15);
    }
  }

  void test_load_diff_sizes() {
    ScopedFile f1(m_data1, "Efficiency2.txt");

    LoadISISPolarizationEfficiencies alg;
    alg.setChild(true);
    alg.setRethrows(true);
    alg.initialize();
    alg.setProperty("P1", f1.getFileName());
    alg.setProperty("P2", f1.getFileName());
    alg.setProperty("OutputWorkspace", "dummy");
    alg.execute();
    MatrixWorkspace_sptr outWS = alg.getProperty("OutputWorkspace");
    TS_ASSERT(outWS);
    TS_ASSERT_EQUALS(outWS->getNumberHistograms(), 2);
    TS_ASSERT_EQUALS(outWS->blocksize(), 5);
    TS_ASSERT_EQUALS(outWS->getAxis(0)->unit()->caption(), "Wavelength");

    auto axis1 = outWS->getAxis(1);
    TS_ASSERT_EQUALS(axis1->label(0), "P1");
    TS_ASSERT_EQUALS(axis1->label(1), "P2");

    TS_ASSERT(!outWS->isHistogramData());

    {
      auto const &x = outWS->x(0);
      auto const &y = outWS->y(0);
      TS_ASSERT_EQUALS(x.size(), 5);
      TS_ASSERT_EQUALS(y.size(), 5);
      TS_ASSERT_DELTA(x.front(), 1.1, 1e-15);
      TS_ASSERT_DELTA(x.back(), 5.5, 1e-15);
      TS_ASSERT_DELTA(y.front(), 1., 1e-15);
      TS_ASSERT_DELTA(y.back(), 1., 1e-15);
    }

    {
      auto const &x = outWS->x(1);
      auto const &y = outWS->y(1);
      TS_ASSERT_EQUALS(x.size(), 5);
      TS_ASSERT_EQUALS(y.size(), 5);
      TS_ASSERT_DELTA(x.front(), 1.1, 1e-15);
      // TS_ASSERT_DELTA(x.back(), 4.5, 1e-15);
      TS_ASSERT_DELTA(y.front(), 1., 1e-15);
      TS_ASSERT_DELTA(y.back(), 1., 1e-15);
    }
  }

  void test_diff_methods() {
    ScopedFile f1(m_data1, "Efficiency3.txt");

    LoadISISPolarizationEfficiencies alg;
    alg.setChild(true);
    alg.setRethrows(true);
    alg.initialize();
    alg.setProperty("P1", f1.getFileName());
    alg.setProperty("Pp", f1.getFileName());
    alg.setProperty("OutputWorkspace", "dummy");
    TS_ASSERT_THROWS(alg.execute(), const std::invalid_argument &);
  }

private:
  std::string const m_data1{"\n1.10000,1.000000,0.322961\n"
                            "2.20000,1.000000,0.0217908\n"
                            "3.30000,1.000000,0.00993287\n"
                            "4.50000,1.000000,0.00668106\n"
                            "5.50000,1.000000,0.0053833\n"};

  std::string const m_data2{"\n1.10000,1.000000,0.322961\n"
                            "2.20000,1.000000,0.0217908\n"
                            "3.30000,1.000000,0.00993287\n"
                            "4.50000,1.000000,0.00668106\n"};
};

#endif /* MANTID_DATAHANDLING_LOADISISPOLARIZATIONEFFICIENCIESTEST_H_ */
