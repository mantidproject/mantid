#ifndef MANTID_ALGORITHMS_PROCESSBACKGROUNDTEST_H_
#define MANTID_ALGORITHMS_PROCESSBACKGROUNDTEST_H_

#include <cxxtest/TestSuite.h>

#include "MantidCurveFitting/ProcessBackground.h"
#include "MantidDataObjects/Workspace2D.h"
#include "MantidDataObjects/TableWorkspace.h"
#include "MantidAPI/WorkspaceFactory.h"
#include "MantidAPI/TableRow.h"

#include <fstream>

using Mantid::CurveFitting::ProcessBackground;
using namespace Mantid;
using namespace Mantid::API;
using namespace Kernel;
using namespace Mantid::DataObjects;

class ProcessBackgroundTest : public CxxTest::TestSuite
{
public:
  // This pair of boilerplate methods prevent the suite being created statically
  // This means the constructor isn't called when running other tests
  static ProcessBackgroundTest *createSuite() { return new ProcessBackgroundTest(); }
  static void destroySuite( ProcessBackgroundTest *suite ) { delete suite; }

  /** Test option delete region
   */
  void test_DeleteRegion()
  {
    // 1. Create Workspace2D
    DataObjects::Workspace2D_sptr inpws
        = boost::dynamic_pointer_cast<DataObjects::Workspace2D>
        (API::WorkspaceFactory::Instance().create("Workspace2D", 1, 10, 10));
    for (size_t i = 0; i < 10; ++i)
    {
      inpws->dataX(0)[i] = double(i);
      inpws->dataY(0)[i] = double(i)*double(i);
    }
    API::AnalysisDataService::Instance().addOrReplace("Background1", inpws);

    // 2. Do the job
    ProcessBackground alg;
    TS_ASSERT_THROWS_NOTHING(alg.initialize());
    TS_ASSERT(alg.isInitialized());

    TS_ASSERT_THROWS_NOTHING(alg.setProperty("InputWorkspace", inpws));
    TS_ASSERT_THROWS_NOTHING(alg.setProperty("OutputWorkspace", "NewBackground"));
    TS_ASSERT_THROWS_NOTHING(alg.setProperty("Options", "DeleteRegion"));
    TS_ASSERT_THROWS_NOTHING(alg.setProperty("LowerBound", 4.5));
    TS_ASSERT_THROWS_NOTHING(alg.setProperty("UpperBound", 6.3));

    TS_ASSERT_THROWS_NOTHING(alg.execute());
    TS_ASSERT(alg.isExecuted());

    // 3. Check
    DataObjects::Workspace2D_sptr outws = boost::dynamic_pointer_cast<DataObjects::Workspace2D>
        (API::AnalysisDataService::Instance().retrieve("NewBackground"));
    size_t newsize = outws->dataX(0).size();

    TS_ASSERT_EQUALS(newsize, 8);

    // 4. Clean
    API::AnalysisDataService::Instance().remove("Background1");
    API::AnalysisDataService::Instance().remove("NewBackground");

    return;
  }

  /** Test option "Add Region"
   */
  void test_AddRegion()
  {
    // 1. Create Workspace2D
    DataObjects::Workspace2D_sptr inpws
        = boost::dynamic_pointer_cast<DataObjects::Workspace2D>
        (API::WorkspaceFactory::Instance().create("Workspace2D", 1, 10, 10));
    for (size_t i = 0; i < 10; ++i)
    {
      inpws->dataX(0)[i] = double(i);
      inpws->dataY(0)[i] = double(i)*double(i);
    }
    API::AnalysisDataService::Instance().addOrReplace("Background2", inpws);

    DataObjects::Workspace2D_sptr refws
        = boost::dynamic_pointer_cast<DataObjects::Workspace2D>(
          API::WorkspaceFactory::Instance().create("Workspace2D", 1, 10, 10));
    for (size_t i = 0; i < 10; ++i)
    {
      refws->dataX(0)[i] = double(i)*0.3+1.01;
      refws->dataY(0)[i] = double(i)*double(i);
    }
    API::AnalysisDataService::Instance().addOrReplace("RefBackground", refws);

    // 2. Do the job
    ProcessBackground alg;
    TS_ASSERT_THROWS_NOTHING(alg.initialize());
    TS_ASSERT(alg.isInitialized());

    TS_ASSERT_THROWS_NOTHING(alg.setProperty("InputWorkspace", inpws));
    TS_ASSERT_THROWS_NOTHING(alg.setProperty("OutputWorkspace", "NewBackground"));
    TS_ASSERT_THROWS_NOTHING(alg.setProperty("ReferenceWorkspace", refws));
    TS_ASSERT_THROWS_NOTHING(alg.setProperty("Options", "AddRegion"));
    TS_ASSERT_THROWS_NOTHING(alg.setProperty("LowerBound", 1.001));
    TS_ASSERT_THROWS_NOTHING(alg.setProperty("UpperBound", 1.99));

    TS_ASSERT_THROWS_NOTHING(alg.execute());
    TS_ASSERT(alg.isExecuted());

    // 3. Check
    DataObjects::Workspace2D_sptr outws = boost::dynamic_pointer_cast<DataObjects::Workspace2D>
        (API::AnalysisDataService::Instance().retrieve("NewBackground"));
    size_t newsize = outws->dataX(0).size();

    TS_ASSERT_EQUALS(newsize, 14);

    // 4. Clean
    API::AnalysisDataService::Instance().remove("Background2");
    API::AnalysisDataService::Instance().remove("NewBackground");

    return;
  }

  //----------------------------------------------------------------------------------------------
  /** Test automatic background selection
    * Disabled because it requires a data file
   */
  void Passed_test_AutoBackgroundSelection()
  {

    // 1. Prepare for data
    std::string datafile("/home/wzz/Mantid/Code/debug/MyTestData/4862b7.inp");
    DataObjects::Workspace2D_sptr dataws = createWorkspace2D(datafile);
    API::AnalysisDataService::Instance().addOrReplace("DiffractionData", dataws);

    std::vector<double> bkgdpts;
    /// Background points for bank 7
    bkgdpts.push_back(57741.0);
    bkgdpts.push_back(63534.0);
    bkgdpts.push_back(69545.0);
    bkgdpts.push_back(89379.0);
    bkgdpts.push_back(115669.0);
    bkgdpts.push_back(134830.0);
    bkgdpts.push_back(165131.0);
    bkgdpts.push_back(226847.0);

    // 2. Prepare algorithm
    ProcessBackground alg;
    alg.initialize();

    alg.setProperty("InputWorkspace", dataws);
    alg.setProperty("OutputWorkspace", "SelectedBackgroundPoints");
    alg.setProperty("Options", "SelectBackgroundPoints");

    alg.setProperty("BackgroundType", "Polynomial");
    alg.setProperty("BackgroundPoints", bkgdpts);

    alg.setProperty("WorkspaceIndex", 0);
    alg.setProperty("NoiseTolerance", 100.0);

    TS_ASSERT_THROWS_NOTHING(alg.execute());

    TS_ASSERT(alg.isExecuted());

    // 3. Check the result
    DataObjects::Workspace2D_sptr bkgdws = boost::dynamic_pointer_cast<DataObjects::Workspace2D>
        (API::AnalysisDataService::Instance().retrieve("SelectedBackgroundPoints"));
    TS_ASSERT(bkgdws);

    return;
  }

  //----------------------------------------------------------------------------------------------
  /** Test automatic background selection
   */
  void test_SimpleBackgroundGeneration()
  {
    // 1. Create Workspace2D
    DataObjects::Workspace2D_sptr dataws
        = boost::dynamic_pointer_cast<DataObjects::Workspace2D>
        (API::WorkspaceFactory::Instance().create("Workspace2D", 1, 1000, 1000));
    for (size_t i = 0; i < 10; ++i)
    {
      dataws->dataX(0)[i] = double(i);
      dataws->dataY(0)[i] = double(i)*double(i);
    }

    API::AnalysisDataService::Instance().addOrReplace("DiffractionData", dataws);

    std::vector<double> bkgdpts;

    bkgdpts.push_back(577.400);
    bkgdpts.push_back(635.340);
    bkgdpts.push_back(695.450);
    bkgdpts.push_back(893.790);

    // 2. Prepare algorithm
    ProcessBackground alg;
    alg.initialize();

    alg.setProperty("InputWorkspace", dataws);
    alg.setProperty("OutputWorkspace", "SelectedBackgroundPoints");
    alg.setProperty("Options", "SelectBackgroundPoints");
    alg.setProperty("BackgroundPointSelectMode", "Input Background Points Only");

    alg.setProperty("SelectionMode", "FitGivenDataPoints");
    alg.setProperty("BackgroundType", "Polynomial");
    alg.setProperty("BackgroundPoints", bkgdpts);

    alg.setProperty("WorkspaceIndex", 0);
    alg.setProperty("NoiseTolerance", 100.0);

    TS_ASSERT_THROWS_NOTHING(alg.execute());

    TS_ASSERT(alg.isExecuted());

    // 3. Check the result
    DataObjects::Workspace2D_sptr bkgdws = boost::dynamic_pointer_cast<DataObjects::Workspace2D>
        (API::AnalysisDataService::Instance().retrieve("SelectedBackgroundPoints"));
    TS_ASSERT(bkgdws);
    if (bkgdws)
    {
      TS_ASSERT_EQUALS(bkgdws->readX(0).size(), bkgdpts.size());
    }

    AnalysisDataService::Instance().remove("DiffractionData");
    AnalysisDataService::Instance().remove("SelectedBackgroundPoints");

    return;
  }

  //----------------------------------------------------------------------------------------------
  /** Test automatic background selection from a given data
   */
  void test_SelectBackgroundFromInputFunction()
  {
    // Create input data
    DataObjects::Workspace2D_sptr dataws
        = boost::dynamic_pointer_cast<DataObjects::Workspace2D>
        (API::WorkspaceFactory::Instance().create("Workspace2D", 1, 1000, 1000));
    for (size_t i = 0; i < 1000; ++i)
    {
      dataws->dataX(0)[i] = double(i);
      dataws->dataY(0)[i] = double(i)*double(i)+sin(double(i)/180.*3.14);
    }
    API::AnalysisDataService::Instance().addOrReplace("DiffractionData2", dataws);

    // Create background function
    TableWorkspace_sptr functablews = boost::make_shared<TableWorkspace>();
    functablews->addColumn("str", "Name");
    functablews->addColumn("double", "Value");
    TableRow row0 = functablews->appendRow();
    row0 << "A0" << 0.;
    TableRow row1 = functablews->appendRow();
    row1 << "A1" << 0.;
    TableRow row2 = functablews->appendRow();
    row2 << "A2" << 1.;
    AnalysisDataService::Instance().addOrReplace("BackgroundParameters", functablews);

    // Create and set up algorithm
    ProcessBackground alg;
    alg.initialize();

    alg.setProperty("InputWorkspace", dataws);
    alg.setProperty("WorkspaceIndex", 0);
    alg.setProperty("OutputWorkspace", "SelectedBackgroundPoints2");
    alg.setProperty("Options", "SelectBackgroundPoints");

    alg.setProperty("BackgroundType", "Polynomial");
    alg.setProperty("SelectionMode", "UserFunction");
    alg.setProperty("BackgroundTableWorkspace", functablews);

    alg.setProperty("OutputBackgroundParameterWorkspace", "OutBackgroundParameters");
    alg.setProperty("UserBackgroundWorkspace", "VisualWS");
    alg.setProperty("OutputBackgroundType", "Chebyshev");
    alg.setProperty("OutputBackgroundOrder", 6);

    alg.setProperty("NoiseTolerance", 0.25);

    TS_ASSERT_THROWS_NOTHING(alg.execute());

    TS_ASSERT(alg.isExecuted());

    // 3. Check the result
    DataObjects::Workspace2D_sptr bkgdws = boost::dynamic_pointer_cast<DataObjects::Workspace2D>
        (API::AnalysisDataService::Instance().retrieve("SelectedBackgroundPoints2"));
    TS_ASSERT(bkgdws);
    if (bkgdws)
    {
      TS_ASSERT(bkgdws->readX(0).size() > 10);
      TS_ASSERT_EQUALS(bkgdws->getNumberHistograms(), 3);
    }

    TableWorkspace_sptr bkgdparws = boost::dynamic_pointer_cast<TableWorkspace>(
          API::AnalysisDataService::Instance().retrieve("OutBackgroundParameters"));
    TS_ASSERT(bkgdparws);

    AnalysisDataService::Instance().remove("DiffractionData2");
    AnalysisDataService::Instance().remove("SelectedBackgroundPoints2");
    AnalysisDataService::Instance().remove("BackgroundParameters");

    return;
  }


  //----------------------------------------------------------------------------------------------
  /** Read column file to create a workspace2D
   */
  DataObjects::Workspace2D_sptr createWorkspace2D(std::string filename)
  {
    // 1. Read data
    std::vector<double> vecx, vecy, vece;
    importDataFromColumnFile(filename, vecx, vecy, vece);

    // 2. Create workspace
    size_t datasize = vecx.size();
    DataObjects::Workspace2D_sptr dataws
        = boost::dynamic_pointer_cast<DataObjects::Workspace2D>(
          API::WorkspaceFactory::Instance().create("Workspace2D", 1, datasize, datasize));

    for (size_t i = 0; i < vecx.size(); ++i)
    {
      dataws->dataX(0)[i] = vecx[i];
      dataws->dataY(0)[i] = vecy[i];
      dataws->dataE(0)[i] = vece[i];
    }

    std::cout << "DBT505  dataX range: " << vecx[0] << ", " << vecx.back() << " sized " << vecx.size() << std::endl;

    return dataws;
  }


  /** Import data from a column data file
   */
  void importDataFromColumnFile(std::string filename, std::vector<double>& vecX, std::vector<double>& vecY,
                                std::vector<double>& vecE)
  {
    // 1. Open file
    std::ifstream ins;
    ins.open(filename.c_str());
    if (!ins.is_open())
    {
      std::cout << "File " << filename << " cannot be opened. " << std::endl;
      throw std::invalid_argument("Unable to open data file. ");
    }

    // 2. Read file
    char line[256];
    while(ins.getline(line, 256))
    {
      if (line[0] != '#')
      {
        double x, y;
        std::stringstream ss;
        ss.str(line);
        ss >> x >> y;
        vecX.push_back(x);
        vecY.push_back(y);
        double e = 1.0;
        if (y > 1.0E-5)
          e = std::sqrt(y);
        vecE.push_back(e);
      }
    }

    return;
  }

};


#endif /* MANTID_ALGORITHMS_PROCESSBACKGROUNDTEST_H_ */
