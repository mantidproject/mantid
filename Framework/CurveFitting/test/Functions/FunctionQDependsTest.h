// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#ifndef MANTID_CURVEFITTING_FUNCTIONQDEPENDSTEST_H
#define MANTID_CURVEFITTING_FUNCTIONQDEPENDSTEST_H

// Mantid Coding standars <http://www.mantidproject.org/Coding_Standards>

// Mantid Headers from the same project
#include "MantidCurveFitting/Functions/FunctionQDepends.h"
// Mantid headers from other projects
#include "MantidAPI/IFunction.h"
#include "MantidAPI/MatrixWorkspace.h"
#include "MantidAPI/NumericAxis.h"
#include "MantidAPI/ParamFunction.h"
#include "MantidDataHandling/LoadNexus.h"
#include "MantidDataObjects/Workspace2D.h"
#include "MantidKernel/EmptyValues.h"
#include "MantidTestHelpers/WorkspaceCreationHelper.h"
// 3rd party library headers
#include <cxxtest/TestSuite.h>
// Standard library
// N/A

using Attr = Mantid::API::IFunction::Attribute;

namespace {
class ImplementsFunctionQDepends
    : public Mantid::CurveFitting::Functions::FunctionQDepends {

public:
  std::string name() const override { return "ImplementsFunctionQDepends"; }

  void function1D(double *out, const double *xValues,
                  const size_t nData) const override {
    double Q = this->getAttribute("Q").asDouble();
    for (size_t i = 0; i < nData; i++) {
      out[i] = Q * xValues[i];
    }
  }
};
} // end of namespace

class FunctionQDependsTest : public CxxTest::TestSuite {

public:
  // This pair of boilerplate methods prevent the suite being created statically
  // This means the constructor isn't called when running other tests
  static FunctionQDependsTest *createSuite() {
    return new FunctionQDependsTest();
  }
  static void destroySuite(FunctionQDependsTest *suite) { delete suite; }

  void testConstruction() {
    TS_ASSERT_THROWS_NOTHING(ImplementsFunctionQDepends f);
  }

  void testInitialization() {
    ImplementsFunctionQDepends f;
    TS_ASSERT_THROWS_NOTHING(f.initialize());
  }

  void testSetWorkspace() {
    double startX{0.0}, endX{1.0};
    ImplementsFunctionQDepends f;
    f.initialize(); // declare attributes
    // test with an non matrix workspace
    TS_ASSERT_THROWS_NOTHING(
        f.setMatrixWorkspace(this->unsuitableWS(), 0, startX, endX));
    // test with a non-suitable matrix workspace
    TS_ASSERT_THROWS_NOTHING(
        f.setMatrixWorkspace(this->withoutQ(), 0, startX, endX));
    // test with a workspace containing Q values in the vertical axis
    TS_ASSERT_THROWS_NOTHING(
        f.setMatrixWorkspace(this->withQonVerticalAxis(), 0, startX, endX));
    // test with a workspace containing detectors for calculation of Q values
    TS_ASSERT_THROWS_NOTHING(
        f.setMatrixWorkspace(this->withDetectors(), 0, startX, endX));
  }

  void testQAttribute() {
    double startX{0.0}, endX{1.0};
    ImplementsFunctionQDepends f;
    f.initialize(); // declare attributes
    auto Q = f.getAttribute("Q").asDouble();
    TS_ASSERT_EQUALS(Q, Mantid::EMPTY_DBL());
    f.setMatrixWorkspace(this->unsuitableWS(), 0, startX, endX);
    TS_ASSERT_EQUALS(Q, Mantid::EMPTY_DBL());
    f.setMatrixWorkspace(this->withoutQ(), 0, startX, endX);
    TS_ASSERT_EQUALS(Q, Mantid::EMPTY_DBL());
    // test assigning Q when no matrix workspace has been set
    f.setAttribute("Q", Attr(0.18));
    TS_ASSERT_EQUALS(f.getAttribute("Q").asDouble(), 0.18);
    // test assigning Q when a workspace has been set
    f.setMatrixWorkspace(this->withQonVerticalAxis(), 1, startX, endX);
    TS_ASSERT_EQUALS(f.getAttribute("Q").asDouble(), 0.5); // Q overwritten
    f.setAttribute("Q", Attr(0.18));
    TS_ASSERT_EQUALS(f.getAttribute("Q").asDouble(), 0.5); // Q not overwritten
  }

  void testWorkspaceIndexAttribute() {
    double startX{0.0}, endX{1.0};
    ImplementsFunctionQDepends f;
    f.initialize(); // declare attributes
    auto wi = f.getAttribute("WorkspaceIndex").asInt();
    TS_ASSERT_EQUALS(wi, Mantid::EMPTY_INT());
    f.setMatrixWorkspace(this->unsuitableWS(), 0, startX, endX);
    TS_ASSERT_EQUALS(wi, Mantid::EMPTY_INT());
    f.setMatrixWorkspace(this->withoutQ(), 0, startX, endX);
    TS_ASSERT_EQUALS(wi, Mantid::EMPTY_INT());
    // test assigning wi when no matrix workspace has been set
    f.setAttribute("WorkspaceIndex", Attr(1));
    TS_ASSERT_EQUALS(f.getAttribute("WorkspaceIndex").asInt(),
                     Mantid::EMPTY_INT()); // not overwritten
    // test assigning wi when a workspace has been set
    f.setMatrixWorkspace(this->withQonVerticalAxis(), 1, startX, endX);
    TS_ASSERT_EQUALS(f.getAttribute("WorkspaceIndex").asInt(), 1);
    f.setAttribute("WorkspaceIndex", Attr(0));
    TS_ASSERT_EQUALS(f.getAttribute("WorkspaceIndex").asInt(),
                     0); // WorkspaceIndex overwritten
  }

  void testWorkspaceIndexTiesQ() {
    double startX{0.0}, endX{1.0};
    ImplementsFunctionQDepends f;
    f.initialize(); // declare attributes
    f.setMatrixWorkspace(this->withQonVerticalAxis(), 1, startX, endX);
    TS_ASSERT_EQUALS(f.getAttribute("Q").asDouble(), 0.5); // Q overwritten
    f.setAttribute("WorkspaceIndex", Attr(0));
    TS_ASSERT_EQUALS(f.getAttribute("Q").asDouble(), 0.3); // Q overwritten
    f.setMatrixWorkspace(this->withDetectors(), 9, startX, endX);
    TS_ASSERT_DELTA(f.getAttribute("Q").asDouble(), 1.82,
                    0.01); // Q overwritten
    Mantid::API::AnalysisDataService::Instance().clear();
  }

private:
  // return a MatrixWorkspace with Q values on the vertical axis
  Mantid::DataObjects::Workspace2D_sptr withQonVerticalAxis() {
    int nhist{4}, nbins{9};
    // create an axis of Q-values
    std::vector<double> qvalues{
        0.3, 0.5, 0.5, 0.9}; // as many elements as the value of variable nhist
    auto momenta = std::make_unique<Mantid::API::NumericAxis>(qvalues);
    momenta->setUnit("MomentumTransfer");
    // create the matrix workspace
    auto ws = WorkspaceCreationHelper::create2DWorkspaceBinned(nhist, nbins);
    ws->replaceAxis(1, std::move(momenta));
    return ws;
  }

  // return a MatrixWorkspace with detectors allowing computations of Q values
  Mantid::API::MatrixWorkspace_sptr withDetectors() {
    Mantid::DataHandling::LoadNexus loader;
    loader.initialize();
    loader.setPropertyValue("Filename", "irs26173_graphite002_red");
    loader.setPropertyValue("OutputWorkspace", "irs26173");
    TS_ASSERT_THROWS_NOTHING(loader.execute());
    TS_ASSERT(loader.isExecuted());
    return Mantid::API::AnalysisDataService::Instance()
        .retrieveWS<Mantid::API::MatrixWorkspace>("irs26173");
  }

  // return a MatrixWorkspace without Q values
  Mantid::DataObjects::Workspace2D_sptr withoutQ() {
    int nhist{3}, nbins{9};
    return WorkspaceCreationHelper::create2DWorkspaceBinned(nhist, nbins);
  }

  // return a Workspace not of MatrixWorkspace type
  Mantid::DataObjects::EventWorkspace_sptr unsuitableWS() {
    return WorkspaceCreationHelper::createEventWorkspace();
  }
};

#endif /* MANTID_API_FUNCTIONQDEPENDSTEST_H */
