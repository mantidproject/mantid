#ifndef LOADASCIITEST_H_
#define LOADASCIITEST_H_

#include "cxxtest/TestSuite.h"
#include "MantidDataHandling/LoadAscii2.h"
#include "MantidDataHandling/SaveAscii2.h"
#include "MantidAPI/AnalysisDataService.h"
#include "MantidDataObjects/Workspace2D.h"
#include <Poco/File.h>
#include <fstream>

using namespace Mantid::API;
using namespace Mantid::DataHandling;
using namespace Mantid::Kernel;
using namespace Mantid::DataObjects;

class LoadAscii2Test : public CxxTest::TestSuite
{
public:
  static LoadAscii2Test *createSuite() { return new LoadAscii2Test(); }
  static void destroySuite(LoadAscii2Test *suite) { delete suite; }

  LoadAscii2Test()
  {
    m_filename = "example.txt";
  }

  ~LoadAscii2Test()
  {
  }

  void testProperties()
  {
    LoadAscii2 testLoad;
    TS_ASSERT_EQUALS("LoadAscii", testLoad.name());
    TS_ASSERT_EQUALS(2, testLoad.version());
    TS_ASSERT_EQUALS("DataHandling\\Text", testLoad.category());
  }

  void testConfidence()
  {
    LoadAscii2 testLoad;
    testLoad.initialize();
    m_abspath = writeTestFile(3);
    //descriptor keeps an open handle until destructor call, so the destructor must run before i can remove it
    auto* descriptor = new FileDescriptor(m_abspath);
    TS_ASSERT_EQUALS(10, testLoad.confidence(*descriptor));
    delete descriptor;
    TS_ASSERT_THROWS_NOTHING(Poco::File(m_abspath).remove());
  }

  void test_Three_Column_Example_With_No_Header()
  {
    m_abspath = writeTestFile(3,false);
    runTest(3);
    TS_ASSERT_THROWS_NOTHING(Poco::File(m_abspath).remove());
  }

  void test_Three_Column_With_Header_Info()
  {
    m_abspath = writeTestFile(3);
    runTest(3);
    TS_ASSERT_THROWS_NOTHING(Poco::File(m_abspath).remove());
  }

  void test_Two_Column_Example_With_No_Header()
  {
    m_abspath = writeTestFile(2,false);
    runTest(2);
    TS_ASSERT_THROWS_NOTHING(Poco::File(m_abspath).remove());
  }

  void test_Two_Column_With_Header_Info()
  {
    m_abspath = writeTestFile(2);
    runTest(2);
    TS_ASSERT_THROWS_NOTHING(Poco::File(m_abspath).remove());
  }

  void test_Four_Column_Example_With_No_Header()
  {
    m_abspath = writeTestFile(4,false);
    runTest(4);
    TS_ASSERT_THROWS_NOTHING(Poco::File(m_abspath).remove());
  }

  void test_Four_Column_Example_With_HeaderInfo()
  {
    m_abspath = writeTestFile(4);
    runTest(4);
    TS_ASSERT_THROWS_NOTHING(Poco::File(m_abspath).remove());
  }

  void test_Four_Column_With_HeaderInfo_CommentChange()
  {
    m_abspath = writeTestFile(4,true,"~");
    runTest(4,false, "~");
    TS_ASSERT_THROWS_NOTHING(Poco::File(m_abspath).remove());
  }

  void test_Four_Column_With_HeaderInfo_NonScientific()
  {
    m_abspath = writeTestFile(4,true,"#", false, 7);
    runTest(4);
    TS_ASSERT_THROWS_NOTHING(Poco::File(m_abspath).remove());
  }

  //void test_Spacing_Around_Separators()
  //{
  //  const std::string filename("LoadAsciiTest_test_Spaced_Separators.txt");
  //  std::ofstream file(filename.c_str());
  //  file << "X , Y0 , E0\n";
  //  file << "0.0105 , 0.374914 , 0.00584427\n"
  //    "0.0115 , 0.393394 , 0.00464693\n"
  //    "0.0125 , 0.414756 , 0.00453993\n"
  //    "0.0135 , 0.443152 , 0.00492027\n"
  //    "0.0145 , 0.460175 , 0.00478891\n"
  //    "0.0155 , 0.456802 , 0.00481\n"
  //    "0.0165 , 0.477264 , 0.00504672\n"
  //    "0.0175 , 0.478456 , 0.00524423\n"
  //    "0.0185 , 0.488523 , 0.00515007\n";
  //  file.close();
  //  using Mantid::API::MatrixWorkspace_sptr;
  //  MatrixWorkspace_sptr outputWS = runTest(filename, "CSV",true,false);
  //  TS_ASSERT_EQUALS(outputWS->getNumberHistograms(), 1);
  //  TS_ASSERT_EQUALS(outputWS->blocksize(), 9);
  //  Mantid::API::AnalysisDataService::Instance().remove(outputWS->getName());
  //  Poco::File(filename).remove();
  //}
private:

  // Write the test file
  std::string writeTestFile(const int cols, const bool header = true, const std::string & comment = "#", const bool scientific = true, const int precision = -1)
  {
    SaveAscii2 save;
    save.initialize();
    save.setPropertyValue("Filename", m_filename);
    if (cols < 3)
    {
      //saveascii2 doens't save 2 column files it has to be made manually
      std::ofstream file(m_filename.c_str());
      if (scientific)
      {
        file << std::scientific;
      }
      if( header )
      {
        file << comment << "X , Y" << std::endl;;
      }
      for (int i = 0; i < 5; i++)
      {
        file << i << std::endl;
        std::vector<double> X;
        std::vector<double> Y;
        for (int j = 0; j < 4; j++)
        {
          file << 1.5 * j / 0.9 << "," <<
            (i + 1) * (2. + 4. * (1.5 * j / 0.9)) << std::endl;
        }
      }
      file.unsetf(std::ios_base::floatfield);
    }
    else
    {
      Mantid::DataObjects::Workspace2D_sptr wsToSave = boost::dynamic_pointer_cast<
        Mantid::DataObjects::Workspace2D>(WorkspaceFactory::Instance().create("Workspace2D", 5, 4, 4));
      for (int i = 0; i < 5; i++)
      {
        std::vector<double>& X = wsToSave->dataX(i);
        std::vector<double>& Y = wsToSave->dataY(i);
        std::vector<double>& E = wsToSave->dataE(i);
        std::vector<double>& DX = wsToSave->dataDx(i);
        for (int j = 0; j < 4; j++)
        {
          X[j] = 1.5 * j / 0.9;
          Y[j] = (i + 1) * (2. + 4. * X[j]);
          E[j] = 1.;
          if (cols == 4)
          {
            DX[j] = 1.;
          }
        }
      }
      const std::string name = "SaveAsciiWS";
      AnalysisDataService::Instance().add(name, wsToSave);

      save.initialize();
      save.isInitialized();
      if (precision > -1)
      {
        save.setPropertyValue("Precision", boost::lexical_cast<std::string>(precision));
      }
      save.setPropertyValue("InputWorkspace", name);
      save.setPropertyValue("CommentIndicator", comment);
      save.setPropertyValue("ScientificFormat", boost::lexical_cast<std::string>(scientific));
      save.setPropertyValue("ColumnHeader", boost::lexical_cast<std::string>(header));
      save.setPropertyValue("WriteXError", boost::lexical_cast<std::string>(cols == 4));
      save.execute();

      AnalysisDataService::Instance().remove(name);
    }
    return save.getPropertyValue("Filename");
  }

  Mantid::API::MatrixWorkspace_sptr runTest(const int cols, const bool dataCheck = true, const std::string & comment = "#", const std::string & sep = "CSV", const bool execThrows = false)
  {
    using Mantid::DataHandling::LoadAscii2;
    using namespace Mantid::API;

    LoadAscii2 loader;
    loader.initialize();
    const std::string outputName(m_filename);
    TS_ASSERT_THROWS_NOTHING(loader.setPropertyValue("Filename", m_abspath));
    TS_ASSERT_THROWS_NOTHING(loader.setPropertyValue("OutputWorkspace",outputName));
    TS_ASSERT_THROWS_NOTHING(loader.setPropertyValue("Separator", sep));
    TS_ASSERT_THROWS_NOTHING(loader.setPropertyValue("CommentIndicator", comment));
    loader.setRethrows(true);

    if (execThrows)
    {
      TS_ASSERT_THROWS(loader.execute(),std::invalid_argument);
    }
    else
    {
      TS_ASSERT_THROWS_NOTHING(loader.execute());

      TS_ASSERT_EQUALS(loader.isExecuted(), true);

      // Check the workspace
      AnalysisDataServiceImpl &dataStore = AnalysisDataService::Instance();
      if(dataStore.doesExist(outputName))
      {
        TS_ASSERT_EQUALS( dataStore.doesExist(outputName), true);
        Workspace_sptr output;
        TS_ASSERT_THROWS_NOTHING(output = dataStore.retrieve(outputName));
        MatrixWorkspace_sptr outputWS = boost::dynamic_pointer_cast<MatrixWorkspace>(output);
        if(outputWS)
        {
          if( dataCheck )
          {
            checkData(outputWS, cols);
            //Test output
            TS_ASSERT_EQUALS(outputWS->getAxis(0)->unit()->caption(), "Energy");
            TS_ASSERT_EQUALS(outputWS->getAxis(0)->unit()->label(), "meV");
          }
          //Check if filename is saved
          TS_ASSERT_EQUALS(loader.getPropertyValue("Filename"),outputWS->run().getProperty("Filename")->value());
        }
        else
        {
          TS_FAIL(outputName + " does not exist.");
        }
        dataStore.remove(outputName);
        return outputWS;
      }
      else
      {
        TS_FAIL("Cannot retrieve output workspace");
      }
    }
    MatrixWorkspace_sptr outputWS;
    return outputWS;
  }

  void checkData(const Mantid::API::MatrixWorkspace_sptr outputWS, const int cols)
  {
    if( cols == 3 )
    {
      TS_ASSERT_EQUALS(outputWS->getNumberHistograms(), 5);
      TS_ASSERT_EQUALS(outputWS->blocksize(), 4);

      TS_ASSERT_DELTA(outputWS->readX(0)[0], 0, 1e-6);
      TS_ASSERT_DELTA(outputWS->readY(0)[0], 2, 1e-6);
      TS_ASSERT_DELTA(outputWS->readE(0)[0], 1, 1e-6);

      TS_ASSERT_DELTA(outputWS->readX(0)[1], 1.666667, 1e-6);
      TS_ASSERT_DELTA(outputWS->readY(0)[1], 8.666667, 1e-6);
      TS_ASSERT_DELTA(outputWS->readE(0)[1], 1, 1e-6);

      TS_ASSERT_DELTA(outputWS->readX(1)[2], 3.333333, 1e-6);
      TS_ASSERT_DELTA(outputWS->readY(1)[2], 30.66667, 1e-6);
      TS_ASSERT_DELTA(outputWS->readE(1)[2], 1, 1e-6);

      TS_ASSERT_DELTA(outputWS->readX(3)[3], 5, 1e-6);
      TS_ASSERT_DELTA(outputWS->readY(3)[3], 88, 1e-6);
      TS_ASSERT_DELTA(outputWS->readE(3)[3], 1, 1e-6);
    }
    else if( cols == 4 )
    {
      TS_ASSERT_EQUALS(outputWS->getNumberHistograms(), 5);
      TS_ASSERT_EQUALS(outputWS->blocksize(), 4);

      TS_ASSERT_DELTA(outputWS->readX(0)[0], 0, 1e-6);
      TS_ASSERT_DELTA(outputWS->readY(0)[0], 2, 1e-6);
      TS_ASSERT_DELTA(outputWS->readE(0)[0], 1, 1e-6);
      TS_ASSERT_DELTA(outputWS->readDx(0)[0], 1, 1e-6);

      TS_ASSERT_DELTA(outputWS->readX(0)[1], 1.666667, 1e-6);
      TS_ASSERT_DELTA(outputWS->readY(0)[1], 8.666667, 1e-6);
      TS_ASSERT_DELTA(outputWS->readE(0)[1], 1, 1e-6);
      TS_ASSERT_DELTA(outputWS->readDx(0)[1], 1, 1e-6);

      TS_ASSERT_DELTA(outputWS->readX(1)[2], 3.333333, 1e-6);
      TS_ASSERT_DELTA(outputWS->readY(1)[2], 30.66667, 1e-6);
      TS_ASSERT_DELTA(outputWS->readE(1)[2], 1, 1e-6);
      TS_ASSERT_DELTA(outputWS->readDx(1)[2], 1, 1e-6);

      TS_ASSERT_DELTA(outputWS->readX(3)[3], 5, 1e-6);
      TS_ASSERT_DELTA(outputWS->readY(3)[3], 88, 1e-6);
      TS_ASSERT_DELTA(outputWS->readE(3)[3], 1, 1e-6);
      TS_ASSERT_DELTA(outputWS->readDx(3)[3], 1, 1e-6);
    }
    else
    {
      TS_ASSERT_EQUALS(outputWS->getNumberHistograms(), 5);
      TS_ASSERT_EQUALS(outputWS->blocksize(), 4);

      TS_ASSERT_DELTA(outputWS->readX(0)[0], 0, 1e-6);
      TS_ASSERT_DELTA(outputWS->readY(0)[0], 2, 1e-6);
      TS_ASSERT_DELTA(outputWS->readE(0)[0], 0, 1e-6);

      TS_ASSERT_DELTA(outputWS->readX(0)[1], 1.666667, 1e-6);
      TS_ASSERT_DELTA(outputWS->readY(0)[1], 8.666667, 1e-6);
      TS_ASSERT_DELTA(outputWS->readE(0)[1], 0, 1e-6);

      TS_ASSERT_DELTA(outputWS->readX(1)[2], 3.333333, 1e-6);
      TS_ASSERT_DELTA(outputWS->readY(1)[2], 30.66667, 1e-6);
      TS_ASSERT_DELTA(outputWS->readE(1)[2], 0, 1e-6);

      TS_ASSERT_DELTA(outputWS->readX(3)[3], 5, 1e-6);
      TS_ASSERT_DELTA(outputWS->readY(3)[3], 88, 1e-6);
      TS_ASSERT_DELTA(outputWS->readE(3)[3], 0, 1e-6);
    }
  }
  std::string m_filename;
  std::string m_abspath;
};


#endif //LOADASCIITEST_H_
