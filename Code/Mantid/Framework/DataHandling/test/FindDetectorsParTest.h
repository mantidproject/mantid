#ifndef FIND_DETECTORSPAR_H_
#define FIND_DETECTORSPAR_H_

#include <cxxtest/TestSuite.h>
#include "MantidDataHandling/FindDetectorsPar.h"
#include "MantidDataHandling/LoadInstrument.h"
#include "MantidTestHelpers/WorkspaceCreationHelper.h"
#include "MantidTestHelpers/ComponentCreationHelper.h"
#include "MantidGeometry/Instrument/DetectorGroup.h"
#include "MantidDataObjects/TableWorkspace.h"

using namespace Mantid;
using namespace Mantid::API;
using namespace Mantid::DataHandling;

class FindDetectorsParTestASCIIhelpers:public FindDetectorsPar
{
public:
    int count_changes(const char *const Buf,size_t buf_size){
        return FindDetectorsPar::count_changes(Buf,buf_size);
    }
    size_t get_my_line(std::ifstream &in, char *buf, size_t buf_size, const char DELIM){
        return FindDetectorsPar::get_my_line(in, buf, buf_size, DELIM);
    }
    FileTypeDescriptor get_ASCII_header(std::string const &fileName, std::ifstream &data_stream){
        return FindDetectorsPar::get_ASCII_header(fileName, data_stream);
    }
    void load_plain(std::ifstream &stream,std::vector<double> &Data,FileTypeDescriptor const &FILE_TYPE){
         FindDetectorsPar::load_plain(stream,Data,FILE_TYPE);
    }

};

class FindDetectorsParTest : public CxxTest::TestSuite
{
public:
 static FindDetectorsParTest *createSuite() { return new FindDetectorsParTest(); }
 static void destroySuite(FindDetectorsParTest *suite) { delete suite; }
 //*******************************************************
  void testName(){
    TS_ASSERT_EQUALS( findPar->name(), "FindDetectorsPar" );
  }

  void testVersion(){
    TS_ASSERT_EQUALS( findPar->version(), 1 );
  }

  void testCategory(){
    TS_ASSERT_EQUALS( findPar->category(), "DataHandling\\Detectors" );
  }

  void testInit(){

    TS_ASSERT_THROWS_NOTHING( findPar->initialize() );
    TS_ASSERT( findPar->isInitialized() );

    TSM_ASSERT_EQUALS("should be 3 propeties here",3,(size_t)(findPar->getProperties().size()));
  }

 void testSimpleExec(){
      inputWS = buildUngroupedWS("FindDetParTestWS");
 
      TS_ASSERT_THROWS_NOTHING(findPar->setPropertyValue("InputWorkspace", inputWS->getName()));
      TS_ASSERT_THROWS_NOTHING(findPar->setPropertyValue("OutputParTable", "DET_PAR"));

      TSM_ASSERT_THROWS_NOTHING("Calculating workspace parameters should not throw", findPar->execute() );
      TSM_ASSERT("parameters calculations should complete successfully", findPar->isExecuted() );
  
 }
 void testSimpleResults(){
   // Get the resulting table workspace
     Mantid::DataObjects::TableWorkspace_sptr spResult =
        boost::dynamic_pointer_cast<Mantid::DataObjects::TableWorkspace>(AnalysisDataService::Instance().retrieve("DET_PAR"));
     std::vector<double> azim_pattern(3,0);
     std::vector<double> pol_pattern(3,0);
     std::vector<double> sfp_pattern(3,1);
     std::vector<double> azw_pattern(3,0);
     std::vector<double> polw_pattern(3,2.86236);
     pol_pattern[0] = 170.565;
     pol_pattern[1] = 169.565;
     pol_pattern[2] = 168.565;
     azw_pattern[0] = 0.396157;
     azw_pattern[1] = 0.394998;
     azw_pattern[2] = 0.393718;
   
     for(int i=0;i<3;i++){
         TSM_ASSERT_DELTA("azimut wrong",azim_pattern[i],spResult->cell<double>(i,0),1.e-5);
         TSM_ASSERT_DELTA("polar wrong ",pol_pattern[i],spResult->cell<double>(i,1),1.e-3);
         TSM_ASSERT_DELTA("flight path wrong ",sfp_pattern[i],spResult->cell<double>(i,2),1.e-5);
         TSM_ASSERT_DELTA("azim width wrong ",azw_pattern[i],spResult->cell<double>(i,3),1.e-5);
         TSM_ASSERT_DELTA("polar width wrong ",polw_pattern[i],spResult->cell<double>(i,4),1.e-5);
     }
  
     AnalysisDataService::Instance().remove("DET_PAR");

 }
 void testSingleRingExec(){
     inputWS =buildRingGroupedWS("FindDetRingParTestWS");

     TS_ASSERT_THROWS_NOTHING(findPar->setPropertyValue("InputWorkspace", inputWS->getName()));
     TS_ASSERT_THROWS_NOTHING(findPar->setPropertyValue("OutputParTable", "DET_PAR2"));

     TSM_ASSERT_THROWS_NOTHING("Calculating workspace parameters should not throw", findPar->execute() );
     TSM_ASSERT("parameters calculations should complete successfully", findPar->isExecuted() );

 }
 void testSingleRingResults(){
    Mantid::DataObjects::TableWorkspace_sptr spResult =
        boost::dynamic_pointer_cast<Mantid::DataObjects::TableWorkspace>(AnalysisDataService::Instance().retrieve("DET_PAR2"));

       TSM_ASSERT_DELTA("azimut wrong",      0,       spResult->cell<double>(0,0),1.e-5);
       TSM_ASSERT_DELTA("polar wrong ",      37.0451, spResult->cell<double>(0,1),1.e-3);
       TSM_ASSERT_DELTA("flight path wrong ",7.52685, spResult->cell<double>(0,2),1.e-5);
       TSM_ASSERT_DELTA("azim width wrong ", 0,       spResult->cell<double>(0,3),1.e-5);
       TSM_ASSERT_DELTA("polar width wrong ",23.2429, spResult->cell<double>(0,4),1.e-4);

       AnalysisDataService::Instance().remove("DET_PAR2");

 }
 void testParFileProvided(){
      TS_ASSERT_THROWS_NOTHING(findPar->setPropertyValue("InputWorkspace", inputWS->getName()));
      TS_ASSERT_THROWS_NOTHING(findPar->setPropertyValue("OutputParTable", "DET_PAR_ASCII"));
      TS_ASSERT_THROWS_NOTHING(findPar->setPropertyValue("ParFile", "testParFile.par"));

      TSM_ASSERT_THROWS_NOTHING("Calculating workspace parameters should not throw", findPar->execute() );
      TSM_ASSERT("parameters calculations should complete successfully", findPar->isExecuted() );
 }
 void testCount_changes(){
     // testing auxiliary function count changes;
     FindDetectorsParTestASCIIhelpers ASCII_helper;
     std::string data(" aaa  bbb  ccc 444 555 666 777");

     size_t n_columns = ASCII_helper.count_changes(data.c_str(),data.size()+1);
     TS_ASSERT_EQUALS(7,n_columns);

     std::string data1("1111 222 +bbb  22222 7777");
     n_columns = ASCII_helper.count_changes(data1.c_str(),data1.size()+1);
     TS_ASSERT_EQUALS(5,n_columns);
 }
 void testGetWinLine(){
    FindDetectorsParTestASCIIhelpers ASCII_helper;
    std::ofstream test("testfile.bin",std::ios::binary);
    std::string EOL1(" ");
    std::string EOL2(" ");
    EOL1[0] = 0x0D;
    EOL2[0] = 0x0A;
    std::string windows_string(" bla bla bla "+EOL1+EOL2+"alb alb alb"+EOL1+EOL2);
    size_t st_size = windows_string.size();
    test.write(windows_string.data(),st_size);
    test.close();

    std::ifstream t1("testfile.bin",std::ios::binary);
    std::vector<char> BUF(1024);
    size_t length = ASCII_helper.get_my_line(t1,&BUF[0],1024,0x0A);
    
    TS_ASSERT_EQUALS(14,length);
    TS_ASSERT_EQUALS(" bla bla bla \r",std::string(&BUF[0]));

    t1.close();
    remove("testfile.bin");
 }
 void testGetUnixLine(){
    FindDetectorsParTestASCIIhelpers ASCII_helper;
    std::ofstream test("testfile.bin",std::ios::binary);
    std::string EOL1(" ");
    EOL1[0] = 0x0A;
    std::string unix_string(" bla bla bla "+EOL1+"alb alb alb"+EOL1);
    size_t st_size = unix_string.size();
    test.write(unix_string.data(),st_size);
    test.close();

    std::ifstream t1("testfile.bin",std::ios::binary);
    std::vector<char> BUF(1024);
    size_t length = ASCII_helper.get_my_line(t1,&BUF[0],1024,0x0A);

    TS_ASSERT_EQUALS(13,length);
    TS_ASSERT_EQUALS(" bla bla bla ",std::string(&BUF[0]));

    t1.close();
    remove("testfile.bin");

}
 void testGetOldMacLine(){
    FindDetectorsParTestASCIIhelpers ASCII_helper;
    std::ofstream test("testfile.bin",std::ios::binary);
    std::string EOL1(" ");
    EOL1[0] = 0x0A;

    std::string windows_string(" bla bla bla "+EOL1+"alb alb alb"+EOL1);
    size_t st_size = windows_string.size();
    test.write(windows_string.data(),st_size);
    test.close();

    std::ifstream t1("testfile.bin",std::ios::binary);
    std::vector<char> BUF(1024);
    size_t length = ASCII_helper.get_my_line(t1,&BUF[0],1024,0x0A);
    
    TS_ASSERT_EQUALS(13,length);
    TS_ASSERT_EQUALS(" bla bla bla ",std::string(&BUF[0]));

    t1.close();
    remove("testfile.bin");
 }
//
 void testPARfileRead(){
    std::string fileName("testParFile.par");
    FindDetectorsParTestASCIIhelpers ASCII_helper;

    writePARfile(fileName.c_str());
    std::ifstream dataStream;
    std::vector<double> result;
    FileTypeDescriptor descr = ASCII_helper.get_ASCII_header(fileName,dataStream);
    ASCII_helper.load_plain(dataStream,result,descr);

    dataStream.close();
    remove(fileName.c_str());

    TS_ASSERT_EQUALS(PAR_type,descr.Type);
    //TS_ASSERT_EQUALS(3,descr.data_start_position._Fpos);
    TS_ASSERT_EQUALS(2,descr.nData_records);
    TS_ASSERT_EQUALS(6, descr.nData_blocks);
#ifdef WIN32
    TS_ASSERT_EQUALS(0x0A,descr.line_end);
#else 
#ifdef __APPLE__
    TS_ASSERT_EQUALS(0x0A,descr.line_end);
#else
    TS_ASSERT_EQUALS(0x0D,descr.line_end);
#endif
#endif
    for(size_t j=0;j<descr.nData_records;j++){
        for(size_t i=0;i<5;i++){
            if(i!=2){
                TS_ASSERT_DELTA(double(i+j+1),result[i+j*5],FLT_EPSILON);
            }else{
                TS_ASSERT_DELTA(double(i+j+1),-result[i+j*5],FLT_EPSILON);
            }
        }
      }

 }
 void testPARfile3Read(){
    std::string fileName("testParFile.par");
    FindDetectorsParTestASCIIhelpers ASCII_helper;

    writePARfile3(fileName.c_str());
    std::ifstream dataStream;
    std::vector<double> result;
    FileTypeDescriptor descr = ASCII_helper.get_ASCII_header(fileName,dataStream);
    ASCII_helper.load_plain(dataStream,result,descr);

    dataStream.close();
    remove(fileName.c_str());

    TS_ASSERT_EQUALS(PAR_type,descr.Type);
    //TS_ASSERT_EQUALS(3,descr.data_start_position._Fpos);
    TS_ASSERT_EQUALS(3,descr.nData_records);
    TS_ASSERT_EQUALS(6, descr.nData_blocks);
#ifdef WIN32
    TS_ASSERT_EQUALS(0x0A,descr.line_end);
#else
#ifdef __APPLE__
    TS_ASSERT_EQUALS(0x0A,descr.line_end);
#else
    TS_ASSERT_EQUALS(0x0D,descr.line_end);
#endif
#endif
     for(size_t j=0;j<descr.nData_records;j++){
        for(size_t i=0;i<5;i++){
            if(i!=2){
                TS_ASSERT_DELTA(double(i+j+1),result[i+j*5],FLT_EPSILON);
            }else{
                TS_ASSERT_DELTA(double(i+j+1),-result[i+j*5],FLT_EPSILON);
            }
        }
     }
 }
 void testPHXfileRead(){
    std::string fileName("testParFile.phx");
    FindDetectorsParTestASCIIhelpers ASCII_helper;

    writePHXfile(fileName.c_str());
    std::ifstream dataStream;
    std::vector<double> result;
    FileTypeDescriptor descr = ASCII_helper.get_ASCII_header(fileName,dataStream);
    ASCII_helper.load_plain(dataStream,result,descr);
    dataStream.close();
    remove(fileName.c_str());

    TS_ASSERT_EQUALS(PHX_type,descr.Type);
    //TS_ASSERT_EQUALS(3,descr.data_start_position._Fpos);
    TS_ASSERT_EQUALS(3,descr.nData_records);
    TS_ASSERT_EQUALS(7, descr.nData_blocks);
#ifdef WIN32
    TS_ASSERT_EQUALS(0x0A,descr.line_end);
#else
#ifdef __APPLE__
    TS_ASSERT_EQUALS(0x0A,descr.line_end);
#else
    TS_ASSERT_EQUALS(0x0D,descr.line_end);
#endif
#endif
    std::vector<double> pattern(6,0);
    pattern[0]=10;
    pattern[1]=0;
    pattern[2]=5;
    pattern[3]=6;
    pattern[4]=7;
    pattern[5]=8;
    for(size_t j=0;j<descr.nData_records;j++){
        for(size_t i=0;i<6;i++){
            TS_ASSERT_DELTA(pattern[i],result[i+j*6],FLT_EPSILON)
        }      
    }
 }

 //*******************************************************
 FindDetectorsParTest()
 {// the functioning of FindDetectorsParTest is affected by a function call in the FrameworkManager's constructor, creating the algorithm in this way ensures that function is executed
    
    findPar =  FrameworkManager::Instance().createAlgorithm("FindDetectorsPar");
 }
 ~FindDetectorsParTest(){
      FrameworkManager::Instance().clearAlgorithms();
      FrameworkManager::Instance().deleteWorkspace(inputWS->getName());
 }
private:
    IAlgorithm* findPar;
    MatrixWorkspace_sptr inputWS;
    std::vector<Geometry::IDetector_sptr>  partDetectors;

    //
    MatrixWorkspace_sptr  buildUngroupedWS(const std::string &WS_Name)
    {
        const size_t NHIST=3;

        inputWS  = WorkspaceCreationHelper::Create2DWorkspaceBinned(NHIST,10,1.0);

        specid_t forSpecDetMap[NHIST];
        for (size_t j = 0; j < NHIST; ++j)
        {
            // Just set the spectrum number to match the index
            inputWS->getAxis(1)->spectraNo(j) = specid_t(j+1);
            forSpecDetMap[j] = specid_t(j+1);
        }

        AnalysisDataService::Instance().add(WS_Name,inputWS);

        // Load the instrument data
         Mantid::DataHandling::LoadInstrument loader;
         loader.initialize();
        // Path to test input file assumes Test directory checked out from SVN
         std::string inputFile = "INES_Definition.xml";
         loader.setPropertyValue("Filename", inputFile);
         loader.setPropertyValue("Workspace", WS_Name);
         loader.execute();

         inputWS->replaceSpectraMap(new SpectraDetectorMap(forSpecDetMap, forSpecDetMap, NHIST));
       
         return inputWS;

    }
    MatrixWorkspace_sptr  buildRingGroupedWS(const std::string &WS_Name)
    {
       if(inputWS.get()){
           AnalysisDataService::Instance().remove(inputWS->getName());
       }

       boost::shared_ptr<Geometry::DetectorGroup> pDet(ComponentCreationHelper::createRingOfCylindricalDetectors(4,5,4));
       const size_t NDET=pDet->nDets();

       inputWS  = WorkspaceCreationHelper::Create2DWorkspaceBinned(1,10,1.0);

       boost::shared_ptr<Geometry::Instrument> spInst(new Geometry::Instrument("basic_ring"));
       Geometry::ObjComponent *source = new Geometry::ObjComponent("source");
       source->setPos(0.0,0.0,-10.0);
       spInst->markAsSource(source);

       Geometry::ObjComponent *sample = new Geometry::ObjComponent("sample");
       sample->setPos(0.0,0.0,-2);
       spInst->markAsSamplePos(sample);

       // get pointers to the detectors, contributed into group;
       partDetectors = pDet->getDetectors();

       std::vector<specid_t>forSpecDetMap(NDET);
       inputWS->getAxis(1)->spectraNo(0) = 1;

       for(size_t i=0;i<NDET;i++){
            spInst->markAsDetector(partDetectors[i].get());
        // Just set the spectrum number to match the index
            forSpecDetMap[i] = 1;

       }
      inputWS->setInstrument(spInst);
             
  
      // underlying detectors have different id-s but the group has the ID of the first one'
      std::vector<detid_t> detIDDetails = pDet->getDetectorIDs();

      inputWS->replaceSpectraMap(new SpectraDetectorMap(&forSpecDetMap[0], &detIDDetails[0], NDET));
   

      AnalysisDataService::Instance().add(WS_Name,inputWS);
      return inputWS;

    }
//*******************************************************************
    void writePARfile(const char *fileName){
        std::string contents("2\n \
 1.     2.   -3.     4.     5.     1\n \
 2.     3.   -4.     5.     6.     2\n");
        std::ofstream testFile(fileName);
        testFile<<contents;
        testFile.close();
    }
   void writePARfile3(const char *fileName){
        std::string contents("3\n \
1.     2.   -3.     4.     5.     1\n \
2.     3.   -4.     5.     6      2\n \
3.     4.   -5.     6.     7.     3\n"); 
        std::ofstream testFile(fileName);
        testFile<<contents;
        testFile.close();
    }
   void writePHXfile(const char *fileName){
        std::string contents("3\n\
         10         0     5.000     6.000    7.000    8.0000     1\n\
         10         0     5.000     6.000    7.000    8.0000     2\n\
         10         0     5.000     6.000    7.000    8.0000     3\n");

        std::ofstream testFile(fileName);
        testFile<<contents;
        testFile.close();
   }
};
#endif
