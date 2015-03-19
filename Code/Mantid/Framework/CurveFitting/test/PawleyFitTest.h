#ifndef MANTID_CURVEFITTING_PAWLEYFITTEST_H_
#define MANTID_CURVEFITTING_PAWLEYFITTEST_H_

#include <cxxtest/TestSuite.h>

#include "MantidCurveFitting/PawleyFit.h"
#include "MantidAPI/AlgorithmManager.h"
#include "MantidAPI/FunctionFactory.h"
#include "MantidAPI/TableRow.h"
#include "MantidAPI/WorkspaceFactory.h"
#include "MantidKernel/V3D.h"
#include "MantidTestHelpers/WorkspaceCreationHelper.h"

using Mantid::CurveFitting::PawleyFit;
using namespace Mantid::API;
using namespace Mantid::Kernel;

class PawleyFitTest : public CxxTest::TestSuite {
public:
  // This pair of boilerplate methods prevent the suite being created statically
  // This means the constructor isn't called when running other tests
  static PawleyFitTest *createSuite() { return new PawleyFitTest(); }
  static void destroySuite(PawleyFitTest *suite) { delete suite; }

  void testGetHKL() {
    TestablePawleyFit pfit;

    V3D referenceHKL(1, 2, 3);

    TS_ASSERT_EQUALS(pfit.getHkl("1 2 3"), referenceHKL);
    TS_ASSERT_EQUALS(pfit.getHkl(" 1 2 3 "), referenceHKL);
    TS_ASSERT_EQUALS(pfit.getHkl("1 2 3"), referenceHKL);
    TS_ASSERT_EQUALS(pfit.getHkl("1,2,3"), referenceHKL);
    TS_ASSERT_EQUALS(pfit.getHkl("1;2;3"), referenceHKL);
    TS_ASSERT_EQUALS(pfit.getHkl("[1,2,3]"), referenceHKL);
    TS_ASSERT_EQUALS(pfit.getHkl("[1;2 3]"), referenceHKL);
  }

  void testFitHexagonalCellQ() {
    /* Like in the PawleyFunctionTest, some reflections are needed.
     * In this case, 5 reflections that belong to a hexagonal cell
     * are used and stored in a TableWorkspace that has a suitable
     * format for PawleyFit. The unit of the workspace is MomentumTransfer.
     */

    ITableWorkspace_sptr hkls = getHCPTable();
    MatrixWorkspace_sptr ws =
        getWorkspace(getFunctionString(hkls, true), (2.0 * M_PI) / 2.1,
                     (2.0 * M_PI) / 1.0, 1000, "MomentumTransfer");

    IAlgorithm_sptr pFit = AlgorithmManager::Instance().create("PawleyFit");
    pFit->setProperty("InputWorkspace", ws);
    pFit->setProperty("WorkspaceIndex", 0);
    pFit->setProperty("CrystalSystem", "Hexagonal");
    pFit->setProperty("InitialCell", "2.444 2.441 3.937 90 90 120");
    pFit->setProperty("PeakTable", hkls);
    pFit->setProperty("OutputWorkspace", "HCP_output");
    pFit->setProperty("RefinedPeakParameterTable", "HCP_peaks");
    pFit->setProperty("RefinedCellTable", "HCP_cell");

    TS_ASSERT_THROWS_NOTHING(pFit->execute());

    // Examine table with cell parameters.
    ITableWorkspace_sptr cellWs =
        AnalysisDataService::Instance().retrieveWS<ITableWorkspace>("HCP_cell");

    // Three rows (a, c, ZeroShift)
    TS_ASSERT_EQUALS(cellWs->rowCount(), 3);

    // Error of 'a' should be small
    TS_ASSERT_LESS_THAN(fabs(cellWs->cell<double>(0, 2)), 1e-5);
    // a should be almost equal to 2.45
    TS_ASSERT_DELTA(cellWs->cell<double>(0, 1), 2.45, 1e-5);

    // Error of 'c' should also be small
    TS_ASSERT_LESS_THAN(fabs(cellWs->cell<double>(1, 2)), 1e-6);
    // c should be almost equal to 3.93
    TS_ASSERT_DELTA(cellWs->cell<double>(1, 1), 3.93, 1e-6);

    // Check number of peak parameters.
    ITableWorkspace_sptr peakWs =
        AnalysisDataService::Instance().retrieveWS<ITableWorkspace>(
            "HCP_peaks");
    TS_ASSERT_EQUALS(peakWs->rowCount(), 5 * 3); // 5 functions with 3 params.

    AnalysisDataService::Instance().remove("HCP_output");
    AnalysisDataService::Instance().remove("HCP_peaks");
    AnalysisDataService::Instance().remove("HCP_cell");
  }

  void testFitOrthorhombicCelld() {
    /* In analogy to the above example, an orthorhombic cell is fitted,
     * this time in dSpacing and with a FlatBackground added.
     */

    ITableWorkspace_sptr hkls = getOrthorhombicTable();
    MatrixWorkspace_sptr ws = getWorkspace(getFunctionString(hkls, false), 1.5,
                                           2.1, 1000, "dSpacing");

    IAlgorithm_sptr pFit = AlgorithmManager::Instance().create("PawleyFit");
    pFit->setProperty("InputWorkspace", ws);
    pFit->setProperty("WorkspaceIndex", 0);
    pFit->setProperty("CrystalSystem", "Orthorhombic");
    pFit->setProperty("InitialCell", "2.44 3.13 4.07 90 90 90");
    pFit->setProperty("PeakTable", hkls);
    pFit->setProperty("OutputWorkspace", "OP_output");
    pFit->setProperty("RefinedPeakParameterTable", "OP_peaks");
    pFit->setProperty("RefinedCellTable", "OP_cell");

    pFit->execute();

    // Examine table with cell parameters.
    ITableWorkspace_sptr cellWs =
        AnalysisDataService::Instance().retrieveWS<ITableWorkspace>("OP_cell");

    // Three rows (a, b, c, ZeroShift)
    TS_ASSERT_EQUALS(cellWs->rowCount(), 4);

    // Error of 'a' should be small
    TS_ASSERT_LESS_THAN(fabs(cellWs->cell<double>(0, 2)), 1e-4);
    // a should be almost equal to 2.45
    TS_ASSERT_DELTA(cellWs->cell<double>(0, 1), 2.45, 2e-3);

    // Error of 'b' should also be small
    TS_ASSERT_LESS_THAN(fabs(cellWs->cell<double>(1, 2)), 1e-4);
    // b should be almost equal to 3.12
    TS_ASSERT_DELTA(cellWs->cell<double>(1, 1), 3.12, 2e-3);

    // Error of 'c' should also be small
    TS_ASSERT_LESS_THAN(fabs(cellWs->cell<double>(2, 2)), 1e-4);
    // b should be almost equal to 4.06
    TS_ASSERT_DELTA(cellWs->cell<double>(2, 1), 4.06, 2e-3);

    // Check number of peak parameters.
    ITableWorkspace_sptr peakWs =
        AnalysisDataService::Instance().retrieveWS<ITableWorkspace>("OP_peaks");
    TS_ASSERT_EQUALS(peakWs->rowCount(), 7 * 3); // 5 functions with 3 params.

    AnalysisDataService::Instance().remove("OP_output");
    AnalysisDataService::Instance().remove("OP_peaks");
    AnalysisDataService::Instance().remove("OP_cell");
  }

private:
  class TestablePawleyFit : public PawleyFit {
    friend class PawleyFitTest;

  public:
    TestablePawleyFit() : PawleyFit() {}
    ~TestablePawleyFit() {}
  };

  ITableWorkspace_sptr getHCPTable() {
    ITableWorkspace_sptr tableWs = WorkspaceFactory::Instance().createTable();
    tableWs->addColumn("V3D", "HKL");
    tableWs->addColumn("double", "d");
    tableWs->addColumn("double", "FWHM (rel.)");
    // Check that string columns are converted if they contain numbers
    tableWs->addColumn("str", "Intensity");

    TableRow row0 = tableWs->appendRow();
    row0 << V3D(0, 0, 2) << 1.965 << 0.004 << "3800.0";

    TableRow row1 = tableWs->appendRow();
    row1 << V3D(1, 0, 1) << 1.867037 << 0.004 << "16400.0";
    TableRow row2 = tableWs->appendRow();
    row2 << V3D(1, 0, 2) << 1.441702 << 0.005 << "3700.0";
    TableRow row3 = tableWs->appendRow();
    row3 << V3D(1, 0, 3) << 1.114663 << 0.006 << "5900.0";
    TableRow row4 = tableWs->appendRow();
    row4 << V3D(2, -1, 0) << 1.225 << 0.004 << "5100.0";

    return tableWs;
  }

  ITableWorkspace_sptr getOrthorhombicTable() {
    ITableWorkspace_sptr tableWs = WorkspaceFactory::Instance().createTable();
    tableWs->addColumn("V3D", "HKL");
    tableWs->addColumn("double", "d");
    tableWs->addColumn("double", "FWHM (rel.)");
    // Check that string columns are converted if they contain numbers
    tableWs->addColumn("str", "Intensity");

    TableRow row0 = tableWs->appendRow();
    row0 << V3D(0, 0, 2) << 2.03000 << 0.004 << "110.628118";

    TableRow row1 = tableWs->appendRow();
    row1 << V3D(0, 1, 2) << 1.701542 << 0.0042 << "180.646775";

    TableRow row2 = tableWs->appendRow();
    row2 << V3D(0, 2, 0) << 1.560000 << 0.00483 << "79.365613";

    TableRow row3 = tableWs->appendRow();
    row3 << V3D(1, 0, 1) << 2.097660 << 0.0041 << "228.086161";

    TableRow row4 = tableWs->appendRow();
    row4 << V3D(1, 0, 2) << 1.563144 << 0.004 << "159.249424";

    TableRow row5 = tableWs->appendRow();
    row5 << V3D(1, 1, 0) << 1.926908 << 0.004 << "209.913635";

    TableRow row6 = tableWs->appendRow();
    row6 << V3D(1, 1, 1) << 1.740797 << 0.00472 << "372.446264";

    return tableWs;
  }

  std::string getFunctionString(const ITableWorkspace_sptr &table, bool useQ) {
    std::vector<std::string> functionStrings;

    for (size_t i = 0; i < table->rowCount(); ++i) {
      TableRow row = table->getRow(i);
      std::ostringstream fn;
      double d = row.Double(1);
      double center = useQ ? (2.0 * M_PI) / d : d;
      double fwhmAbs = row.Double(2) * center;
      fn << "name=Gaussian,PeakCentre=" << center
         << ",Sigma=" << fwhmAbs / (2.0 * sqrt(2.0 * log(2.0)))
         << ",Height=" << row.String(3);

      functionStrings.push_back(fn.str());
    }

    return boost::join(functionStrings, ";");
  }

  MatrixWorkspace_sptr getWorkspace(const std::string &functionString,
                                    double xMin, double xMax, size_t n,
                                    const std::string &unit, double bg = 0.0) {
    IFunction_sptr siFn =
        FunctionFactory::Instance().createInitialized(functionString);

    auto ws = WorkspaceFactory::Instance().create("Workspace2D", 1, n, n);

    FunctionDomain1DVector xValues(xMin, xMax, n);
    FunctionValues yValues(xValues);
    std::vector<double> eValues(n, 1.0);

    siFn->function(xValues, yValues);

    std::vector<double> &xData = ws->dataX(0);
    std::vector<double> &yData = ws->dataY(0);
    std::vector<double> &eData = ws->dataE(0);

    for (size_t i = 0; i < n; ++i) {
      xData[i] = xValues[i];
      yData[i] = yValues[i] + bg;
      eData[i] = eValues[i];
    }

    WorkspaceCreationHelper::addNoise(ws, 0, -0.5, 0.5);

    ws->getAxis(0)->setUnit(unit);

    return ws;
  }
};

#endif /* MANTID_CURVEFITTING_PAWLEYFITTEST_H_ */
