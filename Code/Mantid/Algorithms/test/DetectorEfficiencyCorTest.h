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
//    m_Ei = 15.0796918; m_rawFile = Poco::Path(Poco::Path::current()).resolve("../../../../Test/Data/MER02257.RAW").toString();
//    m_Ei = 398.739; m_rawFile = Poco::Path(Poco::Path::current()).resolve("../../../../Test/Data/MAP10266.RAW").toString();
    m_Ei = 12.9462875; m_rawFile = Poco::Path(Poco::Path::current()).resolve("../../../../Test/Data/MAR11001.RAW").toString();
  }

  void setUp()
  {
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
    // regular bin boundaries help with our analysis later
    rebin(inName);
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

    //test that the tests will be run (that the loops will be entered
    TS_ASSERT( input->getNumberHistograms() > 0 )
    TS_ASSERT( input->readY(0).size() > 0 )
    
    int firstNonMonitor = 5;
    // do santity check a selection of the histograms
    for ( int i = firstNonMonitor; i < input->getNumberHistograms(); i+=10 )
    {
      for ( Mantid::MantidVec::size_type j = 0 ; j < input->readY(i).size(); j++ )
      {
        // the efficiency is always > 0 and < 1 and the k_i/k_f factor greater than one when k_i > k_f (deltaE > 0) as correction=(k_i/k_f)/detector_efficiency
        if (input->readX(i)[j] > 0 && input->readY(i)[j] > 0)
        {
          TS_ASSERT( output->readY(i)[j] > input->readY(i)[j] )
        }
      }
    }

    // look at the ratio between the input and the output, the ratio doesn't like zeros so I merge lots of spectra together to avoid them
    mergeTestSpectra(inName, outName);
    input = boost::dynamic_pointer_cast<MatrixWorkspace>(AnalysisDataService::Instance().retrieve(inName));
    output = boost::dynamic_pointer_cast<MatrixWorkspace>(AnalysisDataService::Instance().retrieve(outName));

    for ( Mantid::MantidVec::size_type j = 0; j < input->readY(0).size(); ++j )
    {
      TS_ASSERT_DELTA( output->readY(0)[j]/input->readY(0)[j], ratios[j], 1e-6 )
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
    //only load some data inorder to reduce processing time
    loader.setPropertyValue("SpectrumList", "100-200");
    loader.setPropertyValue("LoadMonitors", "Exclude");
    loader.setProperty("LoadLogFiles", false);
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

  void mergeTestSpectra(std::string inName, std::string outName)
  {
    //select all the spectra to group
    std::string all("0-100");

    GroupDetectors2 loader;
    loader.initialize();

    // Set the properties
    loader.setPropertyValue("InputWorkspace",inName);
    loader.setPropertyValue("OutputWorkspace",inName);
    loader.setPropertyValue("WorkspaceIndexList", all);

    TS_ASSERT_THROWS_NOTHING(loader.execute());
    TS_ASSERT( loader.isExecuted() );

    loader.setPropertyValue("InputWorkspace",outName);
    loader.setPropertyValue("OutputWorkspace",outName);
    loader.setPropertyValue("WorkspaceIndexList", all);

    TS_ASSERT_THROWS_NOTHING(loader.execute());
    TS_ASSERT( loader.isExecuted() );
  }

  void rebin(std::string inName)
  {
    SimpleRebin loader;
    loader.initialize();

    // Set the properties
    loader.setPropertyValue("InputWorkspace",inName);
    loader.setPropertyValue("OutputWorkspace",inName);
    //use fairly course binning over the start of the range to speed things up, do some fine binning and go until ConvertUnits runs out of output
    loader.setPropertyValue("Params", "-10, 0.5, 10, 0.01, 12.41");

    TS_ASSERT_THROWS_NOTHING(loader.execute());
    TS_ASSERT( loader.isExecuted() );
  }

  void tearDown()
  {
    Poco::File(m_DatFile).remove();    
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
    static const double ratios[282];

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
const double DetectorEfficiencyCorTest::ratios[] = {0.83246517, 0.83997907, 0.84776862, 0.85585046, 0.86424268, 0.87296492, 0.88203859, 0.89148706, 0.9013359, 0.91161316, 0.92234968, 0.93357942, 0.94533995, 0.95767288, 0.97062449, 0.98424637, 0.99859626, 1.013739, 1.0297478, 1.0467054, 1.064706, 1.0838572, 1.1042826, 1.1261252, 1.149551, 1.1747542, 1.2019641, 1.2314527, 1.2635464, 1.2986404, 1.3372182, 1.3798796, 1.4273779, 1.4806756, 1.541025, 1.6100905, 1.690142, 1.7843703, 1.8974316, 2.0364438, 2.1207873, 2.1243197, 2.1278704, 2.1314396, 2.1350274, 2.138634, 2.1422595, 2.1459041, 2.149568, 2.1532513, 2.1569542, 2.1606769, 2.1644195, 2.1681823, 2.1719654, 2.175769, 2.1795933, 2.1834384, 2.1873046, 2.191192, 2.1951009, 2.1990314, 2.2029838, 2.2069582, 2.2109548, 2.2149738, 2.2190156, 2.2230802, 2.2271678, 2.2312788, 2.2354133, 2.2395715, 2.2437537, 2.2479601, 2.252191, 2.2564465, 2.2607269, 2.2650324, 2.2693634, 2.27372, 2.2781025, 2.2825111, 2.2869461, 2.2914079, 2.2958965, 2.3004124, 2.3049557, 2.3095269, 2.314126, 2.3187535, 2.3234096, 2.3280946, 2.3328088, 2.3375526, 2.3423262, 2.3471299, 2.351964, 2.3568289, 2.3617249, 2.3666524, 2.3716116, 2.3766029, 2.3816266, 2.3866832, 2.3917729, 2.3968961, 2.4020532, 2.4072445, 2.4124705, 2.4177315, 2.423028, 2.4283602, 2.4337286, 2.4391337, 2.4445758, 2.4500553, 2.4555728, 2.4611285, 2.466723, 2.4723567, 2.4780301, 2.4837437, 2.4894978, 2.4952931, 2.5011299, 2.5070088, 2.5129303, 2.5188948, 2.524903, 2.5309553, 2.5370524, 2.5431946, 2.5493827, 2.5556171, 2.5618985, 2.5682274, 2.5746044, 2.5810302, 2.5875053, 2.5940305, 2.6006063, 2.6072333, 2.6139124, 2.620644, 2.627429, 2.6342681, 2.6411619, 2.6481112, 2.6551167, 2.6621792, 2.6692995, 2.6764783, 2.6837165, 2.6910148, 2.6983742, 2.7057954, 2.7132793, 2.7208268, 2.7284389, 2.7361164, 2.7438602, 2.7516714, 2.7595509, 2.7674997, 2.7755188, 2.7836092, 2.7917721, 2.8000085, 2.8083195, 2.8167062, 2.8251698, 2.8337115, 2.8423324, 2.8510338, 2.859817, 2.8686832, 2.8776338, 2.88667, 2.8957933, 2.905005, 2.9143067, 2.9236997, 2.9331855, 2.9427657, 2.9524419, 2.9622157, 2.9720887, 2.9820625, 2.992139, 3.0023199, 3.012607, 3.0230021, 3.0335071, 3.0441241, 3.0548549, 3.0657017, 3.0766664, 3.0877514, 3.0989587, 3.1102906, 3.1217496, 3.1333378, 3.1450578, 3.1569122, 3.1689033, 3.181034, 3.193307, 3.2057249, 3.2182908, 3.2310074, 3.243878, 3.2569056, 3.2700933, 3.2834446, 3.2969628, 3.3106513, 3.3245139, 3.3385542, 3.352776, 3.3671833, 3.3817801, 3.3965706, 3.4115591, 3.42675, 3.442148, 3.4577578, 3.4735843, 3.4896324, 3.5059075, 3.5224149, 3.5391602, 3.5561491, 3.5733876, 3.5908818, 3.6086382, 3.6266633, 3.6449639, 3.6635471, 3.6824203, 3.701591, 3.7210672, 3.740857, 3.760969, 3.7814118, 3.8021948, 3.8233273, 3.8448194, 3.8666812, 3.8889235, 3.9115575, 3.9345947, 3.9580473, 3.9819278, 4.0062494, 4.0310259, 4.0562715, 4.0820012, 4.1082308, 4.1349764, 4.1622554, 4.1900855, 4.2184856, 4.2474754, 4.2770755, 4.3073077, 4.3381946, 4.3697603, 4.40203, 4.4350301, 4.4687887, 4.5033351, 4.5387004, 4.5749176, 4.6120213, 4.6500482, 4.6890371, 4.7290294, 4.7700687, 4.8122015, 4.8554772, 4.8999483};
#endif /*DETECTOREFFICIENCYCOR_H_*/
