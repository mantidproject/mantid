#ifndef SAVEGSSTEST_H_
#define SAVEGSSTEST_H_

#include "cxxtest/TestSuite.h"
#include "MantidDataHandling/SaveGSS.h"
#include "MantidAPI/AlgorithmManager.h"
#include "MantidAPI/MatrixWorkspace.h"
#include "MantidTestHelpers/WorkspaceCreationHelper.h"
#include "MantidGeometry/Instrument.h"

#include <Poco/File.h>
#include <fstream>

using namespace Mantid;
using namespace Mantid::API;
using namespace Mantid::DataObjects;
using namespace Mantid::Geometry;
using namespace Mantid::Kernel;
using Mantid::DataHandling::SaveGSS;

class SaveGSSTest : public CxxTest::TestSuite
{
public:

  void test_TheBasics()
  {
    Mantid::DataHandling::SaveGSS saver;
    TS_ASSERT_THROWS_NOTHING( saver.initialize() )
    TS_ASSERT_EQUALS( saver.name(), "SaveGSS" )
    TS_ASSERT_EQUALS( saver.category(), "Diffraction;DataHandling\\Text" )
    TS_ASSERT_EQUALS( saver.version(), 1 )
  }

  //----------------------------------------------------------------------------------------------
  /** Save a 2 banks diffraction data with instrument
    */
  void test_2BankInstrument()
  {
    // Create a workspace for writing out
    MatrixWorkspace_sptr dataws = generateTestMatrixWorkspace();
    AnalysisDataService::Instance().addOrReplace("Test2BankWS", dataws);

    Mantid::DataHandling::SaveGSS saver;
    saver.initialize();

    // Set properties
    TS_ASSERT_THROWS_NOTHING( saver.setPropertyValue("InputWorkspace", "Test2BankWS") );
    TS_ASSERT_THROWS_NOTHING( saver.setProperty("Filename", "test1.gsa") );
    TS_ASSERT_THROWS_NOTHING( saver.setProperty("Format", "SLOG") );
    TS_ASSERT_THROWS_NOTHING( saver.setProperty("SplitFiles", false) );
    TS_ASSERT_THROWS_NOTHING( saver.setProperty("MultiplyByBinWidth", false) );
    TS_ASSERT_THROWS_NOTHING( saver.setProperty("Append", false) );

    // Execute
    saver.execute();
    TS_ASSERT(saver.isExecuted());

    // Check result
    // locate output file
    std::string outfilepath = saver.getPropertyValue("Filename");
    std::cout << "Output file is located at " << outfilepath << "\n";

    Poco::File gsasfile(outfilepath);
    TS_ASSERT(gsasfile.exists());

    // check file
    if (gsasfile.exists())
    {
      size_t numlines = 0;
      std::ifstream fs_gsas(outfilepath.c_str());
      std::string line;
      while (std::getline(fs_gsas, line))
      {
        size_t linenumber = numlines;
        std::stringstream liness(line);
        if (linenumber == 11)
        {
          std::string bank;
          int banknumber;
          liness >> bank >> banknumber;
          TS_ASSERT_EQUALS(bank, "BANK");
          TS_ASSERT_EQUALS(banknumber, 1);
        }
        else if (linenumber == 60)
        {
          double x, y, e;
          liness >> x >> y >> e;
          TS_ASSERT_DELTA(x, 8101.43, 0.01);
          TS_ASSERT_DELTA(y, 688.18, 0.01);
          TS_ASSERT_DELTA(e, 26.23, 0.01);
        }
        else if (linenumber == 114)
        {
          std::string bank;
          int banknumber;
          liness >> bank >> banknumber;
          TS_ASSERT_EQUALS(bank, "BANK");
          TS_ASSERT_EQUALS(banknumber, 2);
        }
        else if (linenumber == 173)
        {
          double x, y, e;
          liness >> x >> y >> e;
          TS_ASSERT_DELTA(x, 8949.02, 0.01);
          TS_ASSERT_DELTA(y, 1592.26, 0.01);
          TS_ASSERT_DELTA(e, 39.90, 0.01);
        }

        ++ numlines;
      }

      TS_ASSERT_EQUALS(numlines, 215);
    }

    // Clean
    AnalysisDataService::Instance().remove("Test2BankWS");
    if (gsasfile.exists())
      gsasfile.remove();

    return;
  }

  //----------------------------------------------------------------------------------------------
  /** Save a 2 banks diffraction data with instrument
    */
  void test_2BankInstrumentRALF()
  {
    // Create a workspace for writing out
    MatrixWorkspace_sptr dataws = generateTestMatrixWorkspace();
    AnalysisDataService::Instance().addOrReplace("Test2BankWS", dataws);

    Mantid::DataHandling::SaveGSS saver;
    saver.initialize();

    // Set properties
    TS_ASSERT_THROWS_NOTHING( saver.setPropertyValue("InputWorkspace", "Test2BankWS") );
    TS_ASSERT_THROWS_NOTHING( saver.setProperty("Filename", "test1r.gsa") );
    TS_ASSERT_THROWS_NOTHING( saver.setProperty("Format", "RALF") );
    TS_ASSERT_THROWS_NOTHING( saver.setProperty("SplitFiles", false) );
    TS_ASSERT_THROWS_NOTHING( saver.setProperty("MultiplyByBinWidth", false) );
    TS_ASSERT_THROWS_NOTHING( saver.setProperty("Append", false) );

    // Execute
    saver.execute();
    TS_ASSERT(saver.isExecuted());

    // Check result
    // locate output file
    std::string outfilepath = saver.getPropertyValue("Filename");
    std::cout << "Output file is located at " << outfilepath << "\n";

    Poco::File gsasfile(outfilepath);
    TS_ASSERT(gsasfile.exists());

    // check file
    if (gsasfile.exists())
    {
      size_t numlines = 0;
      std::ifstream fs_gsas(outfilepath.c_str());
      std::string line;
      while (std::getline(fs_gsas, line))
      {
        size_t linenumber = numlines;
        std::stringstream liness(line);
        if (linenumber == 8)
        {
          std::string bank;
          int banknumber;
          liness >> bank >> banknumber;
          TS_ASSERT_EQUALS(bank, "BANK");
          TS_ASSERT_EQUALS(banknumber, 1);
        }
        else if (linenumber == 57)
        {
          double x, y, e;
          liness >> x >> y >> e;
          TS_ASSERT_DELTA(x, 8101.43, 0.01);
          TS_ASSERT_DELTA(y, 688.18, 0.01);
          TS_ASSERT_DELTA(e, 26.23, 0.01);
        }
        else if (linenumber == 111)
        {
          std::string bank;
          int banknumber;
          liness >> bank >> banknumber;
          TS_ASSERT_EQUALS(bank, "BANK");
          TS_ASSERT_EQUALS(banknumber, 2);
        }
        else if (linenumber == 170)
        {
          double x, y, e;
          liness >> x >> y >> e;
          TS_ASSERT_DELTA(x, 8949.02, 0.01);
          TS_ASSERT_DELTA(y, 1592.26, 0.01);
          TS_ASSERT_DELTA(e, 39.90, 0.01);
        }

        ++ numlines;
      }

      TS_ASSERT_EQUALS(numlines, 212);
    }

    // Clean
    AnalysisDataService::Instance().remove("Test2BankWS");
    if (gsasfile.exists())
      gsasfile.remove();

    return;
  }

  //----------------------------------------------------------------------------------------------
  /** Save a 2 bank workspace in point data format and without instrument
    */
  void test_2BankNoInstrumentData()
  {
    MatrixWorkspace_sptr dataws = generateNoInstrumentWorkspace();

    AnalysisDataService::Instance().addOrReplace("TestNoInstWS", dataws);

    Mantid::DataHandling::SaveGSS saver;
    saver.initialize();

    // Set properties
    TS_ASSERT_THROWS_NOTHING( saver.setPropertyValue("InputWorkspace", "TestNoInstWS") );
    TS_ASSERT_THROWS_NOTHING( saver.setProperty("Filename", "test2.gsa") );
    TS_ASSERT_THROWS_NOTHING( saver.setProperty("Format", "SLOG") );
    TS_ASSERT_THROWS_NOTHING( saver.setProperty("SplitFiles", false) );
    TS_ASSERT_THROWS_NOTHING( saver.setProperty("MultiplyByBinWidth", false) );
    TS_ASSERT_THROWS_NOTHING( saver.setProperty("Append", false) );

    // Execute
    saver.execute();
    TS_ASSERT(saver.isExecuted());

    // Check result
    // locate output file
    std::string outfilepath = saver.getPropertyValue("Filename");
    std::cout << "Output file is located at " << outfilepath << "\n";

    Poco::File gsasfile(outfilepath);
    TS_ASSERT(gsasfile.exists());

    // check file
    if (gsasfile.exists())
    {
      size_t numlines = 0;
      std::ifstream fs_gsas(outfilepath.c_str());
      std::string line;
      while (std::getline(fs_gsas, line))
      {
        size_t linenumber = numlines;
        std::stringstream liness(line);
        if (linenumber == 10)
        {
          std::string bank;
          int banknumber;
          liness >> bank >> banknumber;
          TS_ASSERT_EQUALS(bank, "BANK");
          TS_ASSERT_EQUALS(banknumber, 1);
        }
        else if (linenumber == 59)
        {
          double x, y, e;
          liness >> x >> y >> e;
          TS_ASSERT_DELTA(x, 8101.43, 0.01);
          TS_ASSERT_DELTA(y, 688.18, 0.01);
          TS_ASSERT_DELTA(e, 26.23, 0.01);
        }
        else if (linenumber == 112)
        {
          std::string bank;
          int banknumber;
          liness >> bank >> banknumber;
          TS_ASSERT_EQUALS(bank, "BANK");
          TS_ASSERT_EQUALS(banknumber, 2);
        }
        else if (linenumber == 171)
        {
          double x, y, e;
          liness >> x >> y >> e;
          TS_ASSERT_DELTA(x, 8949.02, 0.01);
          TS_ASSERT_DELTA(y, 1592.26, 0.01);
          TS_ASSERT_DELTA(e, 39.90, 0.01);
        }

        ++ numlines;
      }

      TS_ASSERT_EQUALS(numlines, 213);
    }


    // Clean
    AnalysisDataService::Instance().remove("TestNoInstWS");
    if (gsasfile.exists())
      gsasfile.remove();

    return;
  }

private:

  //----------------------------------------------------------------------------------------------
  /**
    */
  API::MatrixWorkspace_sptr generateNoInstrumentWorkspace()
  {
    MatrixWorkspace_sptr dataws = WorkspaceCreationHelper::Create2DWorkspace(2, 100);
    dataws->getAxis(0)->setUnit("TOF");

    // Set data with logarithm bin
    double t0 = 5000.;
    double dt = 0.01;
    size_t numhist = dataws->getNumberHistograms();
    for (size_t iws = 0; iws < numhist; ++iws)
    {
      MantidVec& dataX = dataws->dataX(iws);
      dataX[0] = t0;
      for (size_t i = 1; i < dataX.size(); ++i)
        dataX[i] = (1+dt)*dataX[i-1];
    }

    // Set y and e
    for (size_t iws = 0; iws < numhist; ++iws)
    {
      const MantidVec& vecX = dataws->readX(iws);
      MantidVec& dataY = dataws->dataY(iws);
      MantidVec& dataE = dataws->dataE(iws);
      double factor = (static_cast<double>(iws)+1)*1000.;
      for (size_t i = 0; i < dataY.size(); ++i)
      {
        dataY[i] = factor*std::exp(-(vecX[i] - 7000. - factor)*(vecX[i] - 7000. - factor)/(0.01*factor*factor));
        if (dataY[i] < 0.01)
          dataE[i] = 0.1;
        else
          dataE[i] = std::sqrt(dataY[i]);
      }
    }

    return dataws;
  }

  //----------------------------------------------------------------------------------------------
  /** Generate a matrix workspace for writing to gsas file
    */
  API::MatrixWorkspace_sptr generateTestMatrixWorkspace()
  {
    // Create workspace
    MatrixWorkspace_sptr dataws = boost::dynamic_pointer_cast<MatrixWorkspace>(
          WorkspaceCreationHelper::create2DWorkspaceWithFullInstrument(2, 100, false, false, true, "TestFake"));
    dataws->getAxis(0)->setUnit("TOF");

    // Set data with logarithm bin
    double t0 = 5000.;
    double dt = 0.01;
    size_t numhist = dataws->getNumberHistograms();
    for (size_t iws = 0; iws < numhist; ++iws)
    {
      MantidVec& dataX = dataws->dataX(iws);
      dataX[0] = t0;
      for (size_t i = 1; i < dataX.size(); ++i)
        dataX[i] = (1+dt)*dataX[i-1];
    }

    // Set y and e
    for (size_t iws = 0; iws < numhist; ++iws)
    {
      const MantidVec& vecX = dataws->readX(iws);
      MantidVec& dataY = dataws->dataY(iws);
      MantidVec& dataE = dataws->dataE(iws);
      double factor = (static_cast<double>(iws)+1)*1000.;
      for (size_t i = 0; i < dataY.size(); ++i)
      {
        dataY[i] = factor*std::exp(-(vecX[i] - 7000. - factor)*(vecX[i] - 7000. - factor)/(0.01*factor*factor));
        if (dataY[i] < 0.01)
          dataE[i] = 0.1;
        else
          dataE[i] = std::sqrt(dataY[i]);
      }
    }

    return dataws;
  }

};


#endif //SAVEGSSTEST_H_
