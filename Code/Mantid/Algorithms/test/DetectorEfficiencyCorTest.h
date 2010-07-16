#ifndef DETECTOREFFICIENCYCORTEST_H_
#define DETECTOREFFICIENCYCORTEST_H_

#include <cxxtest/TestSuite.h>

#include "MantidAlgorithms/DetectorEfficiencyCor.h"
#include "MantidAlgorithms/ConvertUnits.h"
#include "MantidAlgorithms/SimpleRebin.h"
#include "MantidDataHandling/GroupDetectors2.h"
#include "MantidDataHandling/LoadRaw3.h"
#include "MantidDataHandling/LoadDetectorInfo.h"
#include "MantidDataObjects/Workspace2D.h"
#include "MantidKernel/UnitFactory.h"
#include "MantidGeometry/Instrument/Detector.h"
#include "MantidGeometry/Instrument/DetectorGroup.h"
#include "MantidAPI/SpectraDetectorMap.h"
#include "Poco/Path.h"
#include "Poco/File.h"
#include <boost/lexical_cast.hpp>
#include <algorithm>
#include <fstream>
#include <vector>
#include <iostream>
#include <string>
#include <iomanip>

using namespace Mantid::Kernel;
using namespace Mantid::API;
using namespace Mantid::Geometry;
using namespace Mantid::DataObjects;
using namespace Mantid::DataHandling;
using namespace Mantid::Algorithms;

class DetectorEfficiencyCorTest : public CxxTest::TestSuite
{
public:
    DetectorEfficiencyCorTest() :
      m_InoutWS("DetectorEfficiencyCorTest_input_workspace"),
      m_OutWS("DetectorEfficiencyCorTest_output_workspace"),
      m_DatFile("DetectorEfficiencyCorTest_filename.dat")
  {
    // the Ei value depends on the RAW file, during normal testing only use the small RAW file
    m_Ei = 12.9462875; m_rawFile = Poco::Path(Poco::Path::current()).resolve("../../../../Test/Data/MAR11001.RAW").toString();
  }

  void setUp()
  {
    // create a .dat file in the current directory that we'll load later
    writeDatFile();
  }

  void tearDown()
  {
    Poco::File(m_DatFile).remove();    
  }
  
  void testInit()
  {

    DetectorEfficiencyCor grouper;
    TS_ASSERT_EQUALS( grouper.name(), "DetectorEfficiencyCor" )
    TS_ASSERT_EQUALS( grouper.version(), 1 )
    TS_ASSERT_EQUALS( grouper.category(), "CorrectionFunctions" )
    TS_ASSERT_THROWS_NOTHING( grouper.initialize() )
    TS_ASSERT( grouper.isInitialized() )
  }


  void testFromRaw()
  {
    // a smallish raw file that contains the detailed detector information stored by the excitations group 
    std::string inName("fromRaw_DetectorEfficiencyCorTest");
    std::string outName("fromAlg_DetectorEfficiencyCorTest");

    loadRawFile(inName, m_rawFile);
    loadDetInfo(inName, m_rawFile);
    ConvertToDeltaE(inName, m_Ei);
    DetectorEfficiencyCor grouper;
    TS_ASSERT_THROWS_NOTHING( grouper.initialize() )
    TS_ASSERT( grouper.isInitialized() )
    grouper.setPropertyValue("InputWorkspace", inName);
    grouper.setPropertyValue("OutputWorkspace", inName);
    grouper.setProperty("IncidentEnergy", m_Ei);
    grouper.execute();
    TS_ASSERT( grouper.isExecuted() );

    MatrixWorkspace_sptr result = boost::dynamic_pointer_cast<MatrixWorkspace>(AnalysisDataService::Instance().retrieve(inName));

    TS_ASSERT( result->getNumberHistograms() > 0 )
    TS_ASSERT( result->readY(0).size() > 0 )
    int firstNonMonitor = 5;

    // Test some values
    // Unaffected monitors
    TS_ASSERT_DELTA(result->readY(0).front(), 38006., 1e-6)
    TS_ASSERT_DELTA(result->readY(0).back(), 577803., 1e-6)

    //Affected spectra
    TS_ASSERT_DELTA(result->readY(firstNonMonitor).front(), 0.0, 1e-6)
    TS_ASSERT_DELTA(result->readY(firstNonMonitor).back(), 2339.218280, 1e-6)
    // Random spectra
    TS_ASSERT_DELTA(result->readY(42).front(), 0.276367, 1e-6)
    TS_ASSERT_DELTA(result->readY(42)[1225],1.077892 , 1e-6)

    AnalysisDataService::Instance().remove(inName);
  }

private:

  void loadRawFile(std::string WSName, std::string file, bool small_set=false)
  {
    LoadRaw3 loader;
    loader.initialize();

    loader.setPropertyValue("Filename", file);
    loader.setPropertyValue("OutputWorkspace", WSName);
    loader.setProperty("LoadLogFiles", false);
    if( small_set )
    {
      loader.setPropertyValue("SpectrumMin", "69538");
      loader.setPropertyValue("SpectrumMax", "69638");
    }
    TS_ASSERT_THROWS_NOTHING(loader.execute());
  }

  void loadDetInfo(std::string WSName, std::string file)
  {
    LoadDetectorInfo loader;
    loader.initialize();

    loader.setPropertyValue("Workspace", WSName);
    loader.setPropertyValue("DataFilename", file);

    TS_ASSERT_THROWS_NOTHING(loader.execute());
    TS_ASSERT( loader.isExecuted() );
  }

  void ConvertToDeltaE(std::string WSName, double ei)
  {
    ConvertUnits loader;
    loader.initialize();

    // Set the properties
    loader.setPropertyValue("InputWorkspace",WSName);
    loader.setPropertyValue("OutputWorkspace",WSName);
    loader.setPropertyValue("Target","DeltaE");
    loader.setPropertyValue("EMode", "Direct");
    loader.setProperty("EFixed", ei);
    loader.setPropertyValue("AlignBins","0");

    TS_ASSERT_THROWS_NOTHING(loader.execute());
    TS_ASSERT( loader.isExecuted() );
  }



  private:
    const std::string m_InoutWS, m_OutWS, m_DatFile;
    std::string m_rawFile;
    double m_Ei;
    enum constants { NSpectra = 10, NBins = 4, NOTUSED = -123456, DAT_MONTOR_IND = 1};
    static const std::string delta[NSpectra], pressure[NSpectra], wallThick[NSpectra], code[NSpectra];

    void writeDatFile()
    {
      std::ofstream file(m_DatFile.c_str());
      file << "DETECTOR.DAT writen by LoadDetecs" << std::endl;
      file << 165888  <<    14 << std::endl;
      file << "det no.  offset    l2     code     theta        phi         w_x         w_y         w_z         f_x         f_y         f_z         a_x         a_y         a_z        det_1       det_2       det_3       det4" << std::endl;
      for( int i = 0; i < NSpectra; ++i )
      {
        file << i  << "\t" << delta[i]<< "\t"  << NOTUSED<< "\t"  << code[i]<< "\t"  << NOTUSED<< "\t"   << NOTUSED<< "\t"   << NOTUSED<< "\t"   << NOTUSED<< "\t"   << NOTUSED<< "\t"   << NOTUSED<< "\t"   << NOTUSED<< "\t"   << NOTUSED<< "\t"   << NOTUSED << "\t"  << NOTUSED<< "\t"   << NOTUSED<< "\t"  << NOTUSED<< "\t"  << pressure[i]<< "\t"  << wallThick[i]<< "\t"  << NOTUSED << std::endl;
      }
      file.close();
    }
};

const std::string DetectorEfficiencyCorTest::delta[] = {"4", "4.500", "4.500", "4.500", "-6.00", "0.000"};
const std::string DetectorEfficiencyCorTest::pressure[] = {"10.0000", "10.0000", "10.0000", "10.0001", "10.000",  "10.0001"};
const std::string DetectorEfficiencyCorTest::wallThick[] = {"0.00080", "0.00080", "0.00080", "-0.00080", "0.00080",  "9.500"};
const std::string DetectorEfficiencyCorTest::code[] = {"3", "1", "3", "3", "3",  "3"};
#endif /*DETECTOREFFICIENCYCOR_H_*/
