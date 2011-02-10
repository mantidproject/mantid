#ifndef LOADASCIITEST_H_
#define LOADASCIITEST_H_

#include "cxxtest/TestSuite.h"
#include "MantidDataHandling/LoadAscii.h"
#include "MantidAPI/AnalysisDataService.h"
#include <Poco/File.h>
#include <fstream>

class LoadAsciiTest : public CxxTest::TestSuite
{
public:

  void test_Three_Column_Example_With_No_Header()
  {
    const std::string filename("LoadAsciiTest_test_No_Header_3.txt");
    writeThreeColumnTestFile(filename,  false);
    runTest(filename, "CSV", true);
    Poco::File(filename).remove();
  }
  
  void test_Three_Column_With_Header_Info()
  {
    const std::string filename("LoadAsciiTest_test_With_Header_3.txt");
    writeThreeColumnTestFile(filename, true);
    runTest(filename, "CSV", true);
    Poco::File(filename).remove();
  }

  void test_Two_Column_Example_With_No_Header()
  {
    const std::string filename("LoadAsciiTest_test_No_Header_2.txt");
    writeTwoColumnTestFile(filename, false);
    runTest(filename, "Space", false);
    Poco::File(filename).remove();
  }
  
  void test_Two_Column_With_Header_Info()
  {
    const std::string filename("LoadAsciiTest_test_With_Header_2.txt");
    writeTwoColumnTestFile(filename, true);
    runTest(filename, "Space", false);
    Poco::File(filename).remove();
  }

  void test_Spacing_Around_Separators()
  {
    const std::string filename("LoadAsciiTest_test_Spaced_Separators");
    std::ofstream file(filename.c_str());
    file << "X , Y0 , E0\n";
    file << "0.0105 , 0.374914 , 0.00584427\n"
      "0.0115 , 0.393394 , 0.00464693\n"
      "0.0125 , 0.414756 , 0.00453993\n"
      "0.0135 , 0.443152 , 0.00492027\n"
      "0.0145 , 0.460175 , 0.00478891\n"
      "0.0155 , 0.456802 , 0.00481\n"
      "0.0165 , 0.477264 , 0.00504672\n"
      "0.0175 , 0.478456 , 0.00524423\n"
      "0.0185 , 0.488523 , 0.00515007\n";
    file.close();
    using Mantid::API::MatrixWorkspace_sptr;
    MatrixWorkspace_sptr outputWS = runTest(filename, "CSV",true,false);
    TS_ASSERT_EQUALS(outputWS->getNumberHistograms(), 1);
    TS_ASSERT_EQUALS(outputWS->blocksize(), 9);
    
    Mantid::API::AnalysisDataService::Instance().remove(outputWS->getName());
    Poco::File(filename).remove();
  }

private:
  
  // Write the test file
  void writeThreeColumnTestFile(const std::string & filename, bool header)
  {
    std::ofstream file(filename.c_str());
    if( header )
    {
      file << " PRL985/Lawsonite data WC(RAL/Hoybide-NK)/LA profile 1/4mm Cd 1mm gap FF\n"
	" \n"
	" D-spacing (Ang)\n"
	" Attenuation I/Io\n"
	" \n";
    }
    // Main contents
    file << "\n#\n 1,0.4577471236305,0.4583269753105\n"
      " 2,0.36808374279,0.3361919003876\n"
      " 3,0.5247352519303,0.7957701345866\n"
      " 4,0.7798699911496,0.1859797967467\n"
      " 5,0.174779503769,0.0634479812006\n"
      " 6,0.002655110324412,0.7216711935789\n"
      " 7,0.5001983703116,0.07010101626637\n"
      " 8,0.5070039979247,0.9710074159978\n"
      " 9,0.1597338785974,0.1830805383465\n"
      " 10,0.1679128391369,0.04217658009583\n"
      " 11,0.7866756187628,0.7596057008576\n"
      " 12,0.8730735190893,0.8811609241005\n"
      " 13,0.6683553575243,0.7220984527116\n"
      " 14,0.9721366008484,0.00183111056856\n"
      " 15,0.9330729087191,0.9965819269387\n"
      "#\n"
      " 16,0.1107211523789,0.2854091006195\n"
      " 17,0.8644672994171,0.7749870296335\n"
      " 18,0.8381298257393,0.2118594927824\n"
      " 19,0.4269539475692,0.7621692556536\n"
      " 20,0.9880977813044,0.295571764275\n"
      " 21,0.2509231849116,0.3411664174322\n"
      " 22,0.3361613818781,0.1708120975372\n"
      " 23,0.8218024231697,0.5710928678243\n"
      " 24,0.552476577044,0.8368785668508\n"
      " 25,0.06305124057741,0.7369609668264\n"
      " 26,0.1279030732139,0.1528061769463\n"
      " 27,0.5297708059938,0.4314706869716\n"
      " 28,0.8762779625843,0.8930631427961\n"
      " 29,0.6566362498856,0.4864040040284\n"
      " 30,0.9277321695608,0.6603289895322\n";
    file.close();
  }

  // Write the test file
  void writeTwoColumnTestFile(const std::string & filename, bool header)
  {
    std::ofstream file(filename.c_str());
    if( header )
    {
      file << " PRL985/Lawsonite data WC(RAL/Hoybide-NK)/LA profile 1/4mm Cd 1mm gap FF\n"
	" \n"
	" D-spacing (Ang)\n"
	" Attenuation I/Io\n"
	" \n";
    }
    // Main contents
    file << "  0.25000E+00  0.19104E+00\n"
      "  0.25500E+00  0.19045E+00\n"
      "  0.26000E+00  0.19015E+00\n"
      "  0.26500E+00  0.18977E+00\n"
      "  0.27000E+00  0.18923E+00\n"
      "  0.27500E+00  0.18874E+00\n"
      "  0.28000E+00  0.18841E+00\n"
      "  0.28500E+00  0.18799E+00\n"
      "  0.29000E+00  0.18742E+00\n"
      "  0.29500E+00  0.18692E+00\n"
      "  0.30000E+00  0.18655E+00\n"
      "  0.30500E+00  0.18619E+00\n"
      "  0.31000E+00  0.18567E+00\n"
      "  0.31500E+00  0.18518E+00\n"
      "  0.32000E+00  0.18486E+00\n"
      "  0.32500E+00  0.18448E+00\n"
      "  0.33000E+00  0.18387E+00\n"
      "  0.33500E+00  0.18318E+00\n"
      "  0.34000E+00  0.18250E+00\n"
      "  0.34500E+00  0.18187E+00\n"
      "  0.35000E+00  0.18131E+00\n"
      "  0.35500E+00  0.18081E+00\n"
      "  0.36000E+00  0.18032E+00\n"
      "  0.36500E+00  0.17974E+00\n"
      "  0.37000E+00  0.17927E+00\n"
      "  0.37500E+00  0.17895E+00\n"
      "  0.38000E+00  0.17856E+00\n"
      "  0.38500E+00  0.17810E+00\n"
      "  0.39000E+00  0.17762E+00\n"
      "  0.39500E+00  0.17708E+00\n"
      "  0.40000E+00  0.17644E+00\n"
      "  0.40500E+00  0.17578E+00\n"
      "  0.41000E+00  0.17523E+00\n"
      "  0.41500E+00  0.17469E+00\n"
      "  0.42000E+00  0.17403E+00\n"
      "  0.42500E+00  0.17341E+00\n"
      "  0.43000E+00  0.17295E+00\n"
      "  0.43500E+00  0.17258E+00\n"
      "  0.44000E+00  0.17216E+00\n"
      "  0.44500E+00  0.17166E+00\n"
      "  0.45000E+00  0.17112E+00\n"
      "  0.45500E+00  0.17061E+00\n"
      "  0.46000E+00  0.17010E+00\n"
      "  0.46500E+00  0.16957E+00\n"
      "  0.47000E+00  0.16906E+00\n"
      "  0.47500E+00  0.16858E+00\n"
      "  0.48000E+00  0.16808E+00\n"
      "  0.48500E+00  0.16757E+00\n"
      "  0.49000E+00  0.16707E+00\n"
      "  0.49500E+00  0.16659E+00\n"
      "  0.50000E+00  0.16611E+00\n";
    file.close();
  }


  Mantid::API::MatrixWorkspace_sptr 
  runTest(const std::string & filename, const std::string & sep, const bool threeColumn,
	  const bool dataCheck = true)
  {
    using Mantid::DataHandling::LoadAscii;
    using namespace Mantid::API;

    LoadAscii loader;
    loader.initialize();
    TS_ASSERT_THROWS_NOTHING(loader.setPropertyValue("Filename", filename));
    const std::string outputName(filename);
    TS_ASSERT_THROWS_NOTHING(loader.setPropertyValue("OutputWorkspace",outputName));
    TS_ASSERT_THROWS_NOTHING(loader.setPropertyValue("Separator", sep));
    loader.setRethrows(true);
    loader.execute();

    TS_ASSERT_EQUALS(loader.isExecuted(), true);
    
    // Check the workspace
    AnalysisDataServiceImpl &dataStore = AnalysisDataService::Instance();
    TS_ASSERT_EQUALS( dataStore.doesExist(outputName), true);
    Workspace_sptr output;
    TS_ASSERT_THROWS_NOTHING(output = dataStore.retrieve(outputName));
    MatrixWorkspace_sptr outputWS = boost::dynamic_pointer_cast<MatrixWorkspace>(output);
    if(outputWS)
    {
      if( dataCheck )
      {
	checkData(outputWS, threeColumn);
	//Test output
	TS_ASSERT_EQUALS(outputWS->getAxis(0)->unit()->caption(), "Energy");
	TS_ASSERT_EQUALS(outputWS->getAxis(0)->unit()->label(), "meV");
	dataStore.remove(outputName);
	outputWS = MatrixWorkspace_sptr();
      }
    }
    else
    {
      TS_FAIL("Cannot retrieve output workspace");
    }
    return outputWS;
  }

  void checkData(const Mantid::API::MatrixWorkspace_sptr outputWS, const bool threeColumn)
  {
    if( threeColumn )
    {
      TS_ASSERT_EQUALS(outputWS->getNumberHistograms(), 1);
      TS_ASSERT_EQUALS(outputWS->blocksize(), 30);

      TS_ASSERT_EQUALS(outputWS->readX(0)[0], 1.0);
      TS_ASSERT_EQUALS(outputWS->readY(0)[0], 0.4577471236305);
      TS_ASSERT_EQUALS(outputWS->readE(0)[0], 0.4583269753105);

      TS_ASSERT_EQUALS(outputWS->readX(0)[18], 19.0);
      TS_ASSERT_EQUALS(outputWS->readY(0)[18], 0.4269539475692);
      TS_ASSERT_EQUALS(outputWS->readE(0)[18], 0.7621692556536);

      TS_ASSERT_EQUALS(outputWS->readX(0)[29], 30.0);
      TS_ASSERT_EQUALS(outputWS->readY(0)[29], 0.9277321695608);
      TS_ASSERT_EQUALS(outputWS->readE(0)[29], 0.6603289895322);
    }
    else
    {
      TS_ASSERT_EQUALS(outputWS->getNumberHistograms(), 1);
      TS_ASSERT_EQUALS(outputWS->blocksize(), 51);

      TS_ASSERT_EQUALS(outputWS->readX(0)[0], 0.25);
      TS_ASSERT_EQUALS(outputWS->readY(0)[0], 0.19104);
      TS_ASSERT_EQUALS(outputWS->readE(0)[0], 0.0);

      TS_ASSERT_EQUALS(outputWS->readX(0)[18], 0.34);
      TS_ASSERT_EQUALS(outputWS->readY(0)[18], 0.1825);
      TS_ASSERT_EQUALS(outputWS->readE(0)[18], 0.0);

      TS_ASSERT_EQUALS(outputWS->readX(0)[50], 0.50); 
      TS_ASSERT_EQUALS(outputWS->readY(0)[50], 0.16611);
      TS_ASSERT_EQUALS(outputWS->readE(0)[50], 0.0);
    }
  }
  

};


#endif //LOADASCIITEST_H_
