#ifndef DETECTOREFFICIENCYCORTEST_H_
#define DETECTOREFFICIENCYCORTEST_H_

#include <cxxtest/TestSuite.h>

#include "MantidAlgorithms/DetectorEfficiencyCor.h"
#include "MantidAlgorithms/ConvertUnits.h"
#include "MantidAlgorithms/SimpleRebin.h"
#include "MantidDataHandling/LoadRaw3.h"
#include "MantidDataHandling/LoadDetectorInfo.h"
#include "MantidDataObjects/Workspace2D.h"
#include "MantidKernel/UnitFactory.h"
#include "MantidGeometry/Instrument/Detector.h"
#include "MantidGeometry/Instrument/DetectorGroup.h"
#include "MantidGeometry/Objects/ShapeFactory.h"
#include "MantidAPI/SpectraDetectorMap.h"

#include "Poco/DOM/DOMParser.h"
#include "Poco/DOM/Document.h"
#include "Poco/DOM/Element.h"
#include "Poco/Path.h"
#include "Poco/File.h"

#include <fstream>
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

  void testDataWithGroupedDetectors()
  {
    const std::string wsName = "testInput";
    makeTestWS(wsName);

    DetectorEfficiencyCor grouper;
    grouper.setChild(true);
    TS_ASSERT_THROWS_NOTHING(grouper.initialize());
    TS_ASSERT(grouper.isInitialized());
    TS_ASSERT_THROWS_NOTHING(grouper.setPropertyValue("InputWorkspace", wsName));
    TS_ASSERT_THROWS_NOTHING(grouper.setPropertyValue("OutputWorkspace", wsName));
    TS_ASSERT_THROWS_NOTHING(grouper.setProperty("IncidentEnergy", 2.1));
    grouper.execute();
    TS_ASSERT( grouper.isExecuted() );
    
    MatrixWorkspace_sptr result = boost::dynamic_pointer_cast<MatrixWorkspace>(AnalysisDataService::Instance().retrieve(wsName));

    TS_ASSERT_EQUALS(result->getNumberHistograms(), 10);

    TS_ASSERT_DELTA(result->readY(0).front(), 5.036868, 1e-6);
    TS_ASSERT_DELTA(result->readY(0).back(), 0.0, 1e-6);
    //Errors
    TS_ASSERT_DELTA(result->readE(0).front(), 2.252556, 1e-6);
    TS_ASSERT_DELTA(result->readE(0).back(), 0.0, 1e-6);


    TS_ASSERT_DELTA(result->readY(4)[2], 7.026159, 1e-6);
    TS_ASSERT_DELTA(result->readY(4).back(), 0.0, 1e-6);
    //Errors
    TS_ASSERT_DELTA(result->readE(4)[2], 2.244424, 1e-6);
    TS_ASSERT_DELTA(result->readE(4).back(), 0.0, 1e-6);

  }




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

  void tearDown()
  {
    Poco::File(m_DatFile).remove();    
  }


  // Set up a small workspace for testing
  void makeTestWS(std::string WSName)
  {
    MatrixWorkspace_sptr space = WorkspaceFactory::Instance().create("Workspace2D", NSpectra, NBins + 1, NBins);
    space->getAxis(0)->unit() = UnitFactory::Instance().create("DeltaE");
    Workspace2D_sptr space2D = boost::dynamic_pointer_cast<Workspace2D>(space);
    
    Histogram1D::RCtype x,y,e;
    x.access().resize(NBins+1, 0.0);
    y.access().resize(NBins, 0.0);
    e.access().resize(NBins, 0.01);
    for (int i = 0; i < NBins; ++i)
    {
      x.access()[i] = static_cast<double>((1 + i)/100);
      y.access()[i] = 5 + i;
      e.access()[i] = sqrt(5.0);
    }
    x.access()[NBins] = static_cast<double>(NBins);
    // Fill a couple of zeros just as a check that it doesn't get changed
    y.access()[NBins-1] = 0.0;
    e.access()[NBins-1] = 0.0;
    
    int specNums[3*NSpectra];
    int detIDs[3*NSpectra];
    for (int i=0; i< NSpectra; i++)
    {
      space2D->setX(i,x);
      space2D->setData(i,y,e);
      space2D->getAxis(1)->spectraNo(i) = i+1;
      
      specNums[i] = i+1;
      specNums[i+1] = i+1;
      specNums[i+2] = i+1;
      detIDs[i] = i+1;
      detIDs[i+1] = i+2;
      detIDs[i+2] = i+3;
    }
    space2D->mutableSpectraMap().populate(specNums, detIDs, 3*NSpectra);

    std::string xmlShape = "<cylinder id=\"shape\"> ";
    xmlShape +=	"<centre-of-bottom-base x=\"0.0\" y=\"0.0\" z=\"0.0\" /> " ; 
    xmlShape +=	"<axis x=\"0.0\" y=\"1.0\" z=\"0\" /> " ;
    xmlShape +=	"<radius val=\"0.0127\" /> " ;
    xmlShape +=	"<height val=\"1\" /> " ;
    xmlShape +=	"</cylinder>";
    xmlShape +=	"<algebra val=\"shape\" /> ";  
    
    boost::shared_ptr<Object> shape = getObject(xmlShape);
    space2D->setInstrument(boost::shared_ptr<Instrument>(new Instrument));
    boost::shared_ptr<Instrument> instrument = space2D->getBaseInstrument();
    ObjComponent *sample = new ObjComponent("sample", shape, NULL);
    sample->setPos(0,0,0);
    instrument->markAsSamplePos(sample);
    

    ParameterMap &pmap = space2D->instrumentParameters();
    //Detector info
    for( int i = 0; i < NSpectra; ++i)
    {
      for( int j = 0; j < 3; ++j)
      {
	Detector *detector = new Detector("det",shape, NULL);
	detector->setPos(j*10,i*10,5);
	detector->setID(i+j+1);
	pmap.add("double", detector, "3He(atm)", 10.0);
	pmap.add("double", detector, "wallT(m)", 0.0008);
	instrument->markAsDetector(detector);
      }
    }
	

    AnalysisDataService::Instance().add(WSName, space2D);
  }

  boost::shared_ptr<Object> getObject(std::string xmlShape)
  {
    std::string shapeXML = "<type name=\"userShape\"> " + xmlShape + " </type>";
    
    // Set up the DOM parser and parse xml string
    Poco::XML::DOMParser pParser;
    Poco::XML::Document* pDoc;
    
    pDoc = pParser.parseString(shapeXML);
    
    // Get pointer to root element
    Poco::XML::Element* pRootElem = pDoc->documentElement();
    
    //convert into a Geometry object
    ShapeFactory sFactory;
    boost::shared_ptr<Object> shape_sptr = sFactory.createShape(pRootElem);
    pDoc->release();
    return shape_sptr;
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
