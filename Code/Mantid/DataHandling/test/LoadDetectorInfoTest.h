#ifndef LOADDETECTORINFOTEST_H_
#define LOADDETECTORINFOTEST_H_

#include <cxxtest/TestSuite.h>

#include "MantidDataHandling/LoadDetectorInfo.h"
#include "MantidDataHandling/LoadRaw3.h"
#include "MantidAPI/WorkspaceProperty.h"
#include "MantidKernel/ArrayProperty.h"
#include "MantidDataObjects/Workspace2D.h"
#include "MantidKernel/UnitFactory.h"
#include "MantidGeometry/Instrument/Detector.h"
#include "MantidGeometry/Instrument/DetectorGroup.h"
#include "MantidAPI/SpectraDetectorMap.h"
#include "Poco/Path.h"
#include <boost/lexical_cast.hpp>
#include <fstream>
#include <iostream>

using namespace Mantid::DataHandling;
using namespace Mantid::Kernel;
using namespace Mantid::API;
using namespace Mantid::Geometry;
using namespace Mantid::DataObjects;

class LoadDetectorInfoTest : public CxxTest::TestSuite
{
public:
  void testLoadDat()
  {

    LoadDetectorInfo grouper;

    TS_ASSERT_EQUALS( grouper.name(), "LoadDetectorInfo" )
    TS_ASSERT_EQUALS( grouper.version(), 1 )
    TS_ASSERT_EQUALS( grouper.category(), "DataHandling\\Detectors" )
    TS_ASSERT_THROWS_NOTHING( grouper.initialize() )
    TS_ASSERT( grouper.isInitialized() )

    // Set up a small workspace for testing
    makeSmallWS();
    grouper.setPropertyValue("Workspace", m_InoutWS);
    grouper.setPropertyValue("DataFilename", m_DatFile);  
  
    TS_ASSERT_THROWS_NOTHING(grouper.execute());
    TS_ASSERT( grouper.isExecuted() );

    MatrixWorkspace_const_sptr WS = boost::dynamic_pointer_cast<MatrixWorkspace>(
      AnalysisDataService::Instance().retrieve(m_InoutWS));
          
    for ( int i = 0; i < NDETECTS; ++i)
    {

      boost::shared_ptr<IDetector> detector =WS->getInstrument()->getDetector(i);
      boost::shared_ptr<IComponent> comp =
        boost::dynamic_pointer_cast<IComponent>(detector);
      
      ParametrizedComponent* pcomp =
        dynamic_cast<ParametrizedComponent*>(comp.get());
      TS_ASSERT(pcomp)
      const IComponent* baseComp = pcomp->base();
      TS_ASSERT(baseComp)
    
      ParameterMap& pmap = WS->instrumentParameters();
    
      Parameter_sptr par = pmap.get(baseComp,"gas pressure (atm)");
      // this is only for PSD detectors, code 3
      if ( code[i] == "3" )
      {
        TS_ASSERT(par)
        TS_ASSERT_EQUALS(par->asString(), castaround(pressure[i]))
        par = pmap.get(baseComp,"wall thickness (m)");
        TS_ASSERT(par)
        TS_ASSERT_EQUALS(par->asString(), castaround(wallThick[i]))
        par = pmap.get(baseComp,"time offset (microseconds)");
        TS_ASSERT(par)
        TS_ASSERT_EQUALS(par->asString(), castaround(delta[i]) )
      }
      else TS_ASSERT ( ! par )
    }

    AnalysisDataService::Instance().remove(m_InoutWS);
  }

  void testFromRaw()
  {
    LoadDetectorInfo grouper;

    TS_ASSERT_THROWS_NOTHING( grouper.initialize() )
    TS_ASSERT( grouper.isInitialized() )
    
    loadRawFile();
    grouper.setPropertyValue("Workspace", m_MariWS);

    grouper.setPropertyValue("DataFilename", m_RawFile);  
  
    TS_ASSERT_THROWS_NOTHING( grouper.execute());
    TS_ASSERT( grouper.isExecuted() );

    MatrixWorkspace_const_sptr WS = boost::dynamic_pointer_cast<MatrixWorkspace>(
      AnalysisDataService::Instance().retrieve(m_MariWS));
    ParameterMap& pmap = WS->instrumentParameters();
          
    // read the parameters from some random detectors, they're parameters are all set to the same thing
    const int numRandom = 6;
    // we have the first listed dectetor, last lowerest detID, largest, and a repeated value
    const int randDetects[] = { 4101, 4804, 1323, 1101, 3805, 1323, 3832 };
    for ( int i = 0; i < 7; ++i)
    {
      int detID = randDetects[i];
      boost::shared_ptr<IDetector> detector =WS->getInstrument()->getDetector(detID);
      boost::shared_ptr<IComponent> comp =
        boost::dynamic_pointer_cast<IComponent>(detector);
      
      ParametrizedComponent* pcomp =
        dynamic_cast<ParametrizedComponent*>(comp.get());
      TS_ASSERT(pcomp)
      const IComponent* baseComp = pcomp->base();
      TS_ASSERT(baseComp)
    
      Parameter_sptr par = pmap.get(baseComp,"gas pressure (atm)");

      TS_ASSERT(par)
      TS_ASSERT_EQUALS(par->asString(), castaround("10.0"))
      par = pmap.get(baseComp,"wall thickness (m)");
      TS_ASSERT(par)
      TS_ASSERT_EQUALS(par->asString(), castaround("0.0008"))
      par = pmap.get(baseComp,"time offset (microseconds)");
      TS_ASSERT(par)
      TS_ASSERT_EQUALS(par->asString(), castaround("3.9"))
    }

    AnalysisDataService::Instance().remove(m_InoutWS);
    AnalysisDataService::Instance().remove(m_MariWS);
  }

  void loadRawFile()
  {
    LoadRaw3 loader;
    loader.initialize();

    loader.setPropertyValue("Filename", m_RawFile);
    loader.setPropertyValue("OutputWorkspace", m_MariWS);

    TS_ASSERT_THROWS_NOTHING(loader.execute());
  }

  LoadDetectorInfoTest() :
      m_InoutWS("loaddetectorinfotest_input_workspace"),
      m_DatFile("loaddetectorinfotest_filename.dat"),
      m_MariWS("MARfromRaw")
  {
    // a smallish raw file that contains the detailed detector information stored by the excitations group 
    m_RawFile = Poco::Path(Poco::Path::current()).resolve("../../../../Test/Data/MAR11015.RAW").toString();

    // create a .dat file in the current directory that we'll load later
    writeDatFile();
  }
  
  ~LoadDetectorInfoTest()
  {
    remove(m_DatFile.c_str());
  }

  // Set up a small workspace for testing
  void makeSmallWS()
  {
    MatrixWorkspace_sptr space =
      WorkspaceFactory::Instance().create("Workspace2D", NDETECTS, NBINS+1, NBINS);
    space->getAxis(0)->unit() = UnitFactory::Instance().create("TOF");
    Workspace2D_sptr space2D = boost::dynamic_pointer_cast<Workspace2D>(space);
    Histogram1D::RCtype xs, errors, data[NDETECTS];
    xs.access().resize(NBINS+1, 10.0);
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
    AnalysisDataService::Instance().add(m_InoutWS, space);
  }

  private:
    const std::string m_InoutWS, m_DatFile, m_MariWS;
    std::string m_RawFile;
    enum constants { NDETECTS = 6, NBINS = 4, NOTUSED = -123456 };
    static const std::string delta[NDETECTS], deadTime[NDETECTS], pressure[NDETECTS], wallThick[NDETECTS], code[NDETECTS];

    void writeDatFile()
    {
      std::ofstream file(m_DatFile.c_str());
      file << "DETECTOR.DAT writen by LoadDetecs" << std::endl;
      file << 165888  <<    14 << std::endl;
      file << "det no.  offset    l2     code     theta        phi         w_x         w_y         w_z         f_x         f_y         f_z         a_x         a_y         a_z        det_1       det_2       det_3       det4" << std::endl;
      for( int i = 0; i < NDETECTS; ++i )
      {
        file << i  << "\t" << delta[i]<< "\t"  << NOTUSED<< "\t"  << code[i]<< "\t"  << NOTUSED<< "\t"   << NOTUSED<< "\t"   << NOTUSED<< "\t"   << NOTUSED<< "\t"   << NOTUSED<< "\t"   << NOTUSED<< "\t"   << NOTUSED<< "\t"   << NOTUSED<< "\t"   << NOTUSED << "\t"  << NOTUSED<< "\t"   << NOTUSED<< "\t"  << deadTime[i]<< "\t"  << pressure[i]<< "\t"  << wallThick[i]<< "\t"  << NOTUSED << std::endl;
      }
      file.close();
    }
    std::string castaround(std::string floatNum)
    {
      return boost::lexical_cast<std::string>(boost::lexical_cast<float>(floatNum));
    }
};

const std::string LoadDetectorInfoTest::delta[] = {"4", "4.500", "-6.00", "3.3", "0.000",  "9.500"};
const std::string LoadDetectorInfoTest::deadTime[] = {"0.00000", "100.00" , "-1", "0.00000", "0.000",  "43.7550"};
const std::string LoadDetectorInfoTest::pressure[] = {"10.0000", "10.0000", "10.0000", "10.0001", "10.000",  "10.0001"};
const std::string LoadDetectorInfoTest::wallThick[] = {"0.00080", "0.00080", "0.00080", "-0.00080", "0.00080",  "9.500"};
const std::string LoadDetectorInfoTest::code[] = {"3", "1", "3", "3", "3",  "3"};

#endif /*LOADDETECTORINFOTEST_H_*/
