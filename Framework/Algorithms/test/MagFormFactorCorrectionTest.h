// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#ifndef MAGFORMFACTORCORRECTION_H_
#define MAGFORMFACTORCORRECTION_H_

#include "MantidAPI/AnalysisDataService.h"
#include "MantidAPI/Axis.h"
#include "MantidAPI/MatrixWorkspace.h"
#include "MantidAPI/WorkspaceFactory.h"
#include "MantidAlgorithms/MagFormFactorCorrection.h"
#include "MantidKernel/UnitFactory.h"
#include <cmath>
#include <cxxtest/TestSuite.h>

using namespace Mantid::API;
using namespace Mantid::Algorithms;
using namespace Mantid::Kernel;

class MagFormFactorCorrectionTest : public CxxTest::TestSuite {
public:
  // This pair of boilerplate methods prevent the suite being created statically
  // This means the constructor isn't called when running other tests
  static MagFormFactorCorrectionTest *createSuite() {
    return new MagFormFactorCorrectionTest();
  }
  static void destroySuite(MagFormFactorCorrectionTest *suite) { delete suite; }

  MagFormFactorCorrectionTest()
      : inputWSname("inws"), outputWSname("outws"), ionname("Fe3"),
        formFactorWSname("ffws") {}

  void testInit() {
    TS_ASSERT_THROWS_NOTHING(alg.initialize());
    TS_ASSERT(alg.isInitialized());
  }

  void testFFdefault() {
    Workspace_sptr outws;
    createWorkspaceMag(true, inputWSname);
    TS_ASSERT_THROWS_NOTHING(
        alg.setPropertyValue("InputWorkspace", inputWSname));
    TS_ASSERT_THROWS_NOTHING(
        alg.setPropertyValue("OutputWorkspace", outputWSname));
    TS_ASSERT_THROWS_NOTHING(alg.setPropertyValue("IonName", ionname));
    TS_ASSERT_THROWS_NOTHING(alg.setPropertyValue("FormFactorWorkspace", ""));
    TS_ASSERT_THROWS_NOTHING(alg.execute());
    TS_ASSERT(alg.isExecuted());
    // Test that no form factor work space is created
    TS_ASSERT_THROWS(
        outws = AnalysisDataService::Instance().retrieve(formFactorWSname),
        const Exception::NotFoundError &);
  }

  void testExec() {
    Workspace_sptr outws;
    createWorkspaceMag(true, inputWSname);
    TS_ASSERT_THROWS_NOTHING(alg.initialize());
    TS_ASSERT(alg.isInitialized());
    TS_ASSERT_THROWS_NOTHING(
        alg.setPropertyValue("InputWorkspace", inputWSname));
    TS_ASSERT_THROWS_NOTHING(
        alg.setPropertyValue("OutputWorkspace", outputWSname));
    TS_ASSERT_THROWS_NOTHING(alg.setPropertyValue("IonName", ionname));
    // Test that this time, the form factor workspace _is_ created
    TS_ASSERT_THROWS_NOTHING(
        alg.setPropertyValue("FormFactorWorkspace", formFactorWSname));
    alg.execute();
    TS_ASSERT_THROWS_NOTHING(
        outws = AnalysisDataService::Instance().retrieve(outputWSname));
    MatrixWorkspace_sptr result =
        boost::dynamic_pointer_cast<MatrixWorkspace>(outws);
    TS_ASSERT_THROWS_NOTHING(
        outws = AnalysisDataService::Instance().retrieve(formFactorWSname));
    MatrixWorkspace_sptr ffout =
        boost::dynamic_pointer_cast<MatrixWorkspace>(outws);
    TS_ASSERT_THROWS_NOTHING(
        outws = AnalysisDataService::Instance().retrieve(inputWSname));
    MatrixWorkspace_sptr input =
        boost::dynamic_pointer_cast<MatrixWorkspace>(outws);
    TS_ASSERT_LESS_THAN(checkWorkspaces(input, result, ffout), 1.e-8);
  }

private:
  MagFormFactorCorrection alg;
  std::string inputWSname;
  std::string outputWSname;
  std::string ionname;
  std::string formFactorWSname;

  // Creates a fake workspace
  void createWorkspaceMag(bool isHistogram, std::string wsname) {
    const int nspecs(10);
    const int nbins(50);
    const double invfourPiSqr = 1. / (16. * M_PI * M_PI);

    std::vector<double> X, Y;
    for (int64_t i = 0; i < nbins; i++) {
      X.push_back((double)i * 0.5);
      Y.push_back(std::exp(-(double)i * invfourPiSqr));
    }
    if (isHistogram) {
      X.push_back(nbins + 0.5);
    }
    MatrixWorkspace_sptr ws = WorkspaceFactory::Instance().create(
        "Workspace2D", nspecs, isHistogram ? (nbins + 1) : nbins, nbins);
    for (int64_t i = 0; i < nspecs; i++) {
      ws->mutableX(i).assign(X.begin(), X.end());
      ws->mutableY(i).assign(Y.begin(), Y.end());
    }
    ws->getAxis(0)->unit() = UnitFactory::Instance().create("MomentumTransfer");
    ws->getAxis(1)->unit() = UnitFactory::Instance().create("DeltaE");
    AnalysisDataService::Instance().addOrReplace(wsname, ws);
  }

  // Checks that all the workspaces are consistent (in = out*ff)
  double checkWorkspaces(MatrixWorkspace_sptr in, MatrixWorkspace_sptr out,
                         MatrixWorkspace_sptr ff) {
    const int64_t nbins = in->blocksize();
    const int64_t nspecs = in->getNumberHistograms();
    double df2 = 0., df;
    auto &FF = ff->y(0);
    for (int64_t i = 0; i < nspecs; i++) {
      auto &Y1 = out->y(i);
      auto &Y0 = in->y(i);
      for (int64_t j = 0; j < nbins; j++) {
        // Magnetic intensity is proportional to |F(Q)|^2
        df = Y0[j] - Y1[j] * FF[j] * FF[j];
        if (std::isfinite(df)) {
          df2 += pow(df, 2.);
        }
      }
    }
    return sqrt(df2);
  }
};

#endif /*MAGFORMFACTORCORRECTION_H_*/
