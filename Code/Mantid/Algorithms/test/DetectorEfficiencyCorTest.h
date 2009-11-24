#ifndef DETECTOREFFICIENCYCORTEST_H_
#define DETECTOREFFICIENCYCORTEST_H_

#include <cxxtest/TestSuite.h>

#include "MantidAlgorithms/DetectorEfficiencyCor.h"
#include "MantidAlgorithms/ConvertUnits.h"
#include "MantidDataHandling/LoadRaw3.h"
#include "MantidDataHandling/LoadDetectorInfo.h"
#include "MantidDataObjects/Workspace2D.h"
#include "MantidKernel/UnitFactory.h"
#include "MantidGeometry/Instrument/Detector.h"
#include "MantidGeometry/Instrument/DetectorGroup.h"
#include "MantidAPI/SpectraDetectorMap.h"
#include "Poco/Path.h"
#include <boost/lexical_cast.hpp>
#include <algorithm>
#include <fstream>
#include <vector>
#include <iostream>
#include <string>

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
//    m_Ei = 15.0796918; m_rawFile = Poco::Path(Poco::Path::current()).resolve("../../../../Test/Data/MER02257.RAW").toString();
//    m_Ei = 398.739; m_rawFile = Poco::Path(Poco::Path::current()).resolve("../../../../Test/Data/MAP10266.RAW").toString();
    m_Ei = 12.9462875; m_rawFile = Poco::Path(Poco::Path::current()).resolve("../../../../Test/Data/MAR11001.RAW").toString();
    
    // create a .dat file in the current directory that we'll load later
    writeDatFile();
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
//    loadDetInfo(inName, "C:/mantid/Test/Data/merlin_detector.sca");
    ConvertToDeltaE(inName);

    DetectorEfficiencyCor grouper;
    TS_ASSERT_THROWS_NOTHING( grouper.initialize() )
    TS_ASSERT( grouper.isInitialized() )
    grouper.setPropertyValue("InputWorkspace", inName);
    grouper.setPropertyValue("OutputWorkspace", outName);
    grouper.setProperty("IncidentEnergy", m_Ei);
    TS_ASSERT_THROWS_NOTHING( grouper.execute());
    TS_ASSERT( grouper.isExecuted() );

    MatrixWorkspace_sptr input = boost::dynamic_pointer_cast<MatrixWorkspace>(AnalysisDataService::Instance().retrieve(inName));
    MatrixWorkspace_sptr output = boost::dynamic_pointer_cast<MatrixWorkspace>(AnalysisDataService::Instance().retrieve(outName));

    int firstNonMonitor = 5;
    for ( int i = firstNonMonitor; i < input->getNumberHistograms(); i+=10 )
    {
      for ( Mantid::MantidVec::size_type j = 0 ; j < input->readY(i).size(); j+=10 )
      {
        // the detector efficiency is always > 0
        TS_ASSERT( output->readY(i)[j] > -1e-8 )
        // the efficiency is always < 1 and the k_i/k_f factor greater than one when k_i > k_f (deltaE > 0) as correction=(k_i/k_f)/detector_efficiency
        if (output->readX(i)[j] > 0 ) TS_ASSERT( output->readY(i)[j] >= input->readY(i)[j]*(1-1e-8) )
     //This test needs to be more ... Steve Williams is waiting for data from the Excitations Group
      }
    }

    AnalysisDataService::Instance().remove(inName);
    AnalysisDataService::Instance().remove(outName);
  }

  void loadRawFile(std::string WSName, std::string file)
  {
    LoadRaw3 loader;
    loader.initialize();

    loader.setPropertyValue("Filename", file);
    loader.setPropertyValue("OutputWorkspace", WSName);

    //    loader.setPropertyValue("SpectrumList", "400, 500, 9886, 10000, 11000");
//    loader.setPropertyValue("SpectrumMin", "69600");

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

  void ConvertToDeltaE(std::string WSName)
  {
    ConvertUnits loader;
    loader.initialize();

    // Set the properties
    loader.setPropertyValue("InputWorkspace",WSName);
    loader.setPropertyValue("OutputWorkspace",WSName);
    loader.setPropertyValue("Target","DeltaE");
    loader.setPropertyValue("EMode", "Direct");
    loader.setProperty("EFixed", m_Ei);
    loader.setPropertyValue("AlignBins","0");

    TS_ASSERT_THROWS_NOTHING(loader.execute());
    TS_ASSERT( loader.isExecuted() );
  }

  ~DetectorEfficiencyCorTest()
  {
    remove(m_DatFile.c_str());
  }

  // Set up a small workspace for testing
  void makeSmallWS(std::string WSName)
  {
    MatrixWorkspace_sptr space =
      WorkspaceFactory::Instance().create("Workspace2D", NDETECTS, NBINS+1, NBINS);
    space->getAxis(0)->unit() = UnitFactory::Instance().create("TOF");
    Workspace2D_sptr space2D = boost::dynamic_pointer_cast<Workspace2D>(space);
    Histogram1D::RCtype xs, errors, data[NDETECTS];
    xs.access().resize(NBINS+1, 0.0);
    errors.access().resize(NBINS, 1.0);
    int detIDs[NDETECTS];
    int specNums[NDETECTS];
    for (int j = 0; j < NDETECTS; ++j)
    {
      space2D->setX(j,xs);
      data[j].access().resize(NBINS, j + 1);  // the y values will be different for each spectra (1+index_number) but the same for each bin
      space2D->setData(j, data[j], errors);
      space2D->getAxis(1)->spectraNo(j) = j+1;  // spectra numbers are also 1 + index_numbers because this is the tradition
      detIDs[j] = j;
      specNums[j] = j+1;
    }
    Detector *d = new Detector("det",0);
    d->setID(0);
    boost::dynamic_pointer_cast<Instrument>(space->getInstrument())->markAsDetector(d);
    Detector *d1 = new Detector("det",0);
    d1->setID(1);
    boost::dynamic_pointer_cast<Instrument>(space->getInstrument())->markAsDetector(d1);
    Detector *d2 = new Detector("det",0);
    d2->setID(2);
    boost::dynamic_pointer_cast<Instrument>(space->getInstrument())->markAsDetector(d2);
    Detector *d3 = new Detector("det",0);
    d3->setID(3);
    boost::dynamic_pointer_cast<Instrument>(space->getInstrument())->markAsDetector(d3);
    Detector *d4 = new Detector("det",0);
    d4->setID(4);
    boost::dynamic_pointer_cast<Instrument>(space->getInstrument())->markAsDetector(d4);
    Detector *d5 = new Detector("det",0);
    d5->setID(5);
    boost::dynamic_pointer_cast<Instrument>(space->getInstrument())->markAsDetector(d5);

    // Populate the spectraDetectorMap with fake data to make spectrum number = detector id = workspace index
    space->mutableSpectraMap().populate(specNums, detIDs, NDETECTS );

    // Register the workspace in the data service
    AnalysisDataService::Instance().add(WSName, space);
  }

  private:
    const std::string m_InoutWS, m_OutWS, m_DatFile;
    std::string m_rawFile;
    double m_Ei;
    enum constants { NDETECTS = 6, NBINS = 4, NOTUSED = -123456, DAT_MONTOR_IND = 1};
    static const std::string delta[NDETECTS], pressure[NDETECTS], wallThick[NDETECTS], code[NDETECTS];

    void writeDatFile()
    {
      std::ofstream file(m_DatFile.c_str());
      file << "DETECTOR.DAT writen by LoadDetecs" << std::endl;
      file << 165888  <<    14 << std::endl;
      file << "det no.  offset    l2     code     theta        phi         w_x         w_y         w_z         f_x         f_y         f_z         a_x         a_y         a_z        det_1       det_2       det_3       det4" << std::endl;
      for( int i = 0; i < NDETECTS; ++i )
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
