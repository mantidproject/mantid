#ifndef MANTID_ALGORITHMS_PROCESSBACKGROUNDTEST_H_
#define MANTID_ALGORITHMS_PROCESSBACKGROUNDTEST_H_

#include <cxxtest/TestSuite.h>

#include "MantidCurveFitting/ProcessBackground.h"
#include "MantidDataObjects/Workspace2D.h"
#include "MantidAPI/WorkspaceFactory.h"

#include <fstream>

using Mantid::CurveFitting::ProcessBackground;
using namespace Mantid;
using namespace Kernel;

class ProcessBackgroundTest : public CxxTest::TestSuite
{
public:
  // This pair of boilerplate methods prevent the suite being created statically
  // This means the constructor isn't called when running other tests
  static ProcessBackgroundTest *createSuite() { return new ProcessBackgroundTest(); }
  static void destroySuite( ProcessBackgroundTest *suite ) { delete suite; }

  /*
   * Test option delete region
   */
  void test_DeleteRegion()
  {
      // 1. Create Workspace2D
      DataObjects::Workspace2D_sptr inpws
              = boost::dynamic_pointer_cast<DataObjects::Workspace2D>(API::WorkspaceFactory::Instance().create("Workspace2D", 1, 10, 10));
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

  /*
   * Test option "Add Region"
   */
  void test_AddRegion()
  {
      // 1. Create Workspace2D
      DataObjects::Workspace2D_sptr inpws
              = boost::dynamic_pointer_cast<DataObjects::Workspace2D>(API::WorkspaceFactory::Instance().create("Workspace2D", 1, 10, 10));
      for (size_t i = 0; i < 10; ++i)
      {
          inpws->dataX(0)[i] = double(i);
          inpws->dataY(0)[i] = double(i)*double(i);
      }
      API::AnalysisDataService::Instance().addOrReplace("Background2", inpws);

      DataObjects::Workspace2D_sptr refws
              = boost::dynamic_pointer_cast<DataObjects::Workspace2D>(API::WorkspaceFactory::Instance().create("Workspace2D", 1, 10, 10));
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

      /*
      for (size_t i = 0; i < newsize; ++i)
          std::cout << i << " :  " << outws->dataX(0)[i] << std::endl;
      */

      // 4. Clean
      API::AnalysisDataService::Instance().remove("Background2");
      API::AnalysisDataService::Instance().remove("NewBackground");

      return;
  }

  /*
   * Test automatic background selection
   */
  void Disabled_test_AutoBackgroundSelection()
  {
      // FIXME This will be removed later if I do not want to add data to AutoTestData.

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

      /*
      std::ofstream ofile;
      ofile.open("selectedbackground.dat");
      for (size_t i = 0; i < bkgdws->readX(0).size(); ++i)
      {
          double x = bkgdws->readX(0)[i];
          double y = bkgdws->readY(0)[i];
          double e = bkgdws->readE(0)[i];
          ofile << x << "\t" << y << "\t" << e << std::endl;
      }
      ofile.close();
      */

      return;
  }


  /*
   * Read column file to create a workspace2D
   */
  DataObjects::Workspace2D_sptr createWorkspace2D(std::string filename)
  {
      // 1. Read data
      std::vector<double> vecx, vecy, vece;
      importDataFromColumnFile(filename, vecx, vecy, vece);

      // 2. Create workspace
      size_t datasize = vecx.size();
      DataObjects::Workspace2D_sptr dataws
              = boost::dynamic_pointer_cast<DataObjects::Workspace2D>(API::WorkspaceFactory::Instance().create("Workspace2D", 1, datasize, datasize));

      for (size_t i = 0; i < vecx.size(); ++i)
      {
          dataws->dataX(0)[i] = vecx[i];
          dataws->dataY(0)[i] = vecy[i];
          dataws->dataE(0)[i] = vece[i];
      }

      std::cout << "DBT505  dataX range: " << vecx[0] << ", " << vecx.back() << " sized " << vecx.size() << std::endl;

      return dataws;
  }


  /*
   * Import data from a column data file
   */
  void importDataFromColumnFile(std::string filename, std::vector<double>& vecX, std::vector<double>& vecY, std::vector<double>& vecE)
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
