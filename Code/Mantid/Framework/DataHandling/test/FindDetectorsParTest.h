#ifndef FIND_DETECTORSPAR_H_
#define FIND_DETECTORSPAR_H_

#include <cxxtest/TestSuite.h>
#include "MantidDataHandling/FindDetectorsPar.h"
#include "MantidDataHandling/LoadInstrument.h"
#include "MantidTestHelpers/WorkspaceCreationHelper.h"
#include "MantidTestHelpers/ComponentCreationHelper.h"
#include "MantidGeometry/Instrument/DetectorGroup.h"
#include "MantidDataObjects/TableWorkspace.h"
#include "MantidAPI/TableRow.h"
#include "MantidAPI/FrameworkManager.h"

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

  void testInit(){

    TS_ASSERT_THROWS_NOTHING( findPar->initialize() );
    TS_ASSERT( findPar->isInitialized() );

    TSM_ASSERT_EQUALS("should be 4 properties here",4,(size_t)(findPar->getProperties().size()));
  }

 void testSNSExec(){
      inputWS = buildUngroupedWS("FindDetParTestWS");
 
      TS_ASSERT_THROWS_NOTHING(findPar->setPropertyValue("InputWorkspace", inputWS->getName()));
      TS_ASSERT_THROWS_NOTHING(findPar->setPropertyValue("OutputParTable", "DET_PAR"));

      TSM_ASSERT_THROWS_NOTHING("Calculating workspace parameters should not throw", findPar->execute() );
      TSM_ASSERT("parameters calculations should complete successfully", findPar->isExecuted() );
  
 }
 void testSNSResults(){
   // Get the resulting table workspace
     Mantid::DataObjects::TableWorkspace_sptr spResult =
        AnalysisDataService::Instance().retrieveWS<Mantid::DataObjects::TableWorkspace>("DET_PAR");

       check_SNS_patterns(spResult);
       AnalysisDataService::Instance().remove("DET_PAR");

 }
 void testParFileProvided(){
     // 3 point par file will be used with the 3-detector workspace defined above
      std::string fileName("testParFile.par");
      writePARfile3(fileName.c_str());
      TS_ASSERT_THROWS_NOTHING(findPar->setPropertyValue("InputWorkspace", inputWS->getName()));
      TS_ASSERT_THROWS_NOTHING(findPar->setPropertyValue("OutputParTable", "DET_PAR_ASCII"));
      TS_ASSERT_THROWS_NOTHING(findPar->setPropertyValue("ParFile", fileName));

      TSM_ASSERT_THROWS_NOTHING("Calculating workspace parameters should not throw", findPar->execute() );
      TSM_ASSERT("parameters calculations should complete successfully", findPar->isExecuted() );

      remove(fileName.c_str());
 }
 void testParFileLoadedCorrectly(){
    Mantid::DataObjects::TableWorkspace_sptr spResult =
        AnalysisDataService::Instance().retrieveWS<Mantid::DataObjects::TableWorkspace>("DET_PAR_ASCII");

        TS_ASSERT_EQUALS(3,inputWS->getNumberHistograms());

        std::vector<std::string> pattern(5);
        pattern[2] = "1,2,3,";  // dist
        pattern[0] = "2,3,4,";    // azimutal
        pattern[1] = "-3,-4,-5,"; //polar
        pattern[3] = "78.6901,71.5651,66.8014,"; // atan(5,6,7)/dist;    // pol_width
        pattern[4] = "-75.9638,-68.1986,-63.4349,"; // atan(4,5,6)/dist;    // az_width
        for(int i=0;i<5;i++){
            std::stringstream buf;
            for(int j=0;j<3;j++){
                buf<<spResult->cell<double>(j,i)<<",";
            }
            TS_ASSERT_EQUALS(pattern[i],buf.str());
        }


        AnalysisDataService::Instance().remove("DET_PAR_ASCII");
 }
 
 void testParFileProvidedWrong(){
      std::string fileName("testParFile.par");
      // this is 2 row par file for 3 detectors workspace -- will be ignored, warning  and internal algorithm used instead
      writePARfile(fileName.c_str());
      // should use internal algorithm
      TS_ASSERT_THROWS_NOTHING(findPar->setPropertyValue("InputWorkspace", inputWS->getName()));
      TS_ASSERT_THROWS_NOTHING(findPar->setPropertyValue("OutputParTable", "DET_PAR_ASCII"));
      TS_ASSERT_THROWS_NOTHING(findPar->setPropertyValue("ParFile", fileName));

      TSM_ASSERT_THROWS_NOTHING("Calculating workspace parameters should not throw", findPar->execute() );
      TSM_ASSERT("parameters calculations should complete successfully", findPar->isExecuted() );

      remove(fileName.c_str());
      // check -- this workspace and wrong par file have to result in warning and internal algorithm excecuted;
      Mantid::DataObjects::TableWorkspace_sptr spResult =
        AnalysisDataService::Instance().retrieveWS<Mantid::DataObjects::TableWorkspace>("DET_PAR_ASCII");

      check_SNS_patterns(spResult);
      AnalysisDataService::Instance().remove("DET_PAR_ASCII");
 }
 void testSingleRingExec(){
     inputWS =buildRingGroupedWS("FindDetRingParTestWS");

     TS_ASSERT_THROWS_NOTHING(findPar->setPropertyValue("InputWorkspace", inputWS->getName()));
     TS_ASSERT_THROWS_NOTHING(findPar->setPropertyValue("OutputParTable", "DET_PAR2"));
     // set par file to undefined file trying not to load it
     TS_ASSERT_THROWS_NOTHING(findPar->setPropertyValue("ParFile", ""));

     TSM_ASSERT_THROWS_NOTHING("Calculating workspace parameters should not throw", findPar->execute() );
     TSM_ASSERT("parameters calculations should complete successfully", findPar->isExecuted() );

 }
 void testSingleRingResults(){
    Mantid::DataObjects::TableWorkspace_sptr spResult =
        AnalysisDataService::Instance().retrieveWS<Mantid::DataObjects::TableWorkspace>("DET_PAR2");

       TSM_ASSERT_DELTA("polar wrong ",      37.0451,  spResult->cell<double>(0,0),1.e-3);
       TSM_ASSERT_DELTA("azimut wrong: some average angle -> around initial detector's angle for many detectors", -114.5454, spResult->cell<double>(0,1),1.e-3);
       TSM_ASSERT_DELTA("flight path wrong ",7.5248,  spResult->cell<double>(0,2),1.e-3);
       TSM_ASSERT_DELTA("polar width wrong ",20.0598,  spResult->cell<double>(0,3),1.e-3);
       TSM_ASSERT_DELTA("azim width wrong ring of ~360deg",364.8752, spResult->cell<double>(0,4),1.e-3);

       AnalysisDataService::Instance().remove("DET_PAR2");

 }
 void testPHXExecCorrectly(){
      std::string fileName("testPhxFile.phx");
      // this is 1 row phx file for 1 detector workspace
      writePHX1file(fileName.c_str());
      // should use internal algorithm
      TS_ASSERT_THROWS_NOTHING(findPar->setPropertyValue("InputWorkspace", inputWS->getName()));
      TS_ASSERT_THROWS_NOTHING(findPar->setPropertyValue("OutputParTable", "DET_PHX_ASCII"));
      TS_ASSERT_THROWS_NOTHING(findPar->setPropertyValue("ParFile", fileName));

      TSM_ASSERT_THROWS_NOTHING("Calculating workspace parameters should not throw", findPar->execute() );
      TSM_ASSERT("parameters calculations should complete successfully", findPar->isExecuted() );

      remove(fileName.c_str());
 }
 void testPHXProcessedCorrectly(){
    Mantid::DataObjects::TableWorkspace_sptr spResult =
        AnalysisDataService::Instance().retrieveWS<Mantid::DataObjects::TableWorkspace>("DET_PHX_ASCII");

       TSM_ASSERT_DELTA("polar wrong ",      5,       spResult->cell<double>(0,0),1.e-5);
       TSM_ASSERT_DELTA("azimut wrong",      6,       spResult->cell<double>(0,1),1.e-3);
       TSM_ASSERT_DELTA("flight path wrong ",7.5248,  spResult->cell<double>(0,2),1.e-4);
       TSM_ASSERT_DELTA("polar width wrong ",7,       spResult->cell<double>(0,3),1.e-4);
       TSM_ASSERT_DELTA("azim width wrong ", 8,       spResult->cell<double>(0,4),1.e-4);

       AnalysisDataService::Instance().remove("DET_PHX_ASCII");


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
    TS_ASSERT_EQUALS(char(0x0A),descr.line_end);
#else 
  //  TS_ASSERT_EQUALS(char(0x0A),descr.line_end);
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
    TS_ASSERT_EQUALS(char(0x0A),descr.line_end);
#else
  //  TS_ASSERT_EQUALS(char(0x0A),descr.line_end);
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
    TS_ASSERT_EQUALS(char(0x0A),descr.line_end);
#else
 //   TS_ASSERT_EQUALS(char(0x0A),descr.line_end);
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
    std::vector<Geometry::IDetector_const_sptr>  partDetectors;

    //
    MatrixWorkspace_sptr  buildUngroupedWS(const std::string &WS_Name)
    {
        const int NHIST=3;

        inputWS  = WorkspaceCreationHelper::Create2DWorkspaceBinned(NHIST,10,1.0);

        for (int j = 0; j < NHIST; ++j)
        {
            // Just set the spectrum number to match the index
            ISpectrum * spec = inputWS->getSpectrum(j);
            spec->setSpectrumNo(j+1);
            spec->setDetectorID(j+1);
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

       inputWS->getSpectrum(0)->setSpectrumNo(1);
       inputWS->getSpectrum(0)->clearDetectorIDs();
       inputWS->getSpectrum(0)->addDetectorIDs(pDet->getDetectorIDs());

       for(size_t i=0;i<NDET;i++){
            spInst->markAsDetector(partDetectors[i].get());
       }
      inputWS->setInstrument(spInst);

      AnalysisDataService::Instance().add(WS_Name,inputWS);
      return inputWS;

    }
//*******************************************************************
   void writePARfile(const char *fileName){
        std::vector<std::string> cont(3);
        cont[0] = "2";
        cont[1] = " 1.     2.   -3.     4.     5.     1";
        cont[2] = " 2.     3.   -4.     5.     6.     2";

        std::ofstream testFile(fileName);
        for(size_t i=0;i<cont.size();i++){
            testFile<<cont[i]<<std::endl;
        }
        testFile.close();
    }
   void writePARfile3(const char *fileName){
        std::vector<std::string> cont(4);
        cont[0] = "3";
        cont[1] = "1.     2.   -3.     4.     5.     1";
        cont[2] = "2.     3.   -4.     5.     6      2";
        cont[3] = "3.     4.   -5.     6.     7.     3";

        std::ofstream testFile(fileName);
        for(size_t i=0;i<cont.size();i++){
            testFile<<cont[i]<<std::endl;
        }  
        testFile.close();
    }
   void writePHXfile(const char *fileName){
      std::vector<std::string> cont(4);
        cont[0] = "3";
        cont[1] = "10         0     5.000     6.000    7.000    8.0000     1";
        cont[2] = "10         0     5.000     6.000    7.000    8.0000     2";
        cont[3] = "10         0     5.000     6.000    7.000    8.0000     3";

        std::ofstream testFile(fileName);
        for(size_t i=0;i<cont.size();i++){
            testFile<<cont[i]<<std::endl;
        }
        testFile.close();
   }
   void writePHX1file(const char *fileName){
      std::vector<std::string> cont(2);
        cont[0] = "1";
        cont[1] = "10         0     5.000     6.000    7.000    8.0000     1";
   
        std::ofstream testFile(fileName);
        for(size_t i=0;i<cont.size();i++){
            testFile<<cont[i]<<std::endl;
        }
        testFile.close();
   }
 
  void check_SNS_patterns(const Mantid::DataObjects::TableWorkspace_sptr &spResult){
     std::string azim_pattern("0,0,0,");
     std::string pol_pattern("170.565,169.565,168.565,");
     std::string sfp_pattern("1,1,1,");
     std::string polw_pattern("0.804071,0.804258,0.804442,");
     std::string azw_pattern("5.72472,5.72472,5.72472,");
 

     std::auto_ptr<std::stringstream> bufs[5];
     for(int j=0;j<5;j++){
         bufs[j] = std::auto_ptr<std::stringstream>(new std::stringstream);
         for(int i=0;i<3;i++){
             *(bufs[j])<<spResult->cell<double>(i,j)<<",";
         }
     }
     TSM_ASSERT_EQUALS("azimut wrong",pol_pattern,bufs[0]->str());
     TSM_ASSERT_EQUALS("polar wrong ",azim_pattern,bufs[1]->str());
     TSM_ASSERT_EQUALS("flight path wrong ",sfp_pattern,bufs[2]->str());
     TSM_ASSERT_EQUALS("polar width wrong ",polw_pattern,bufs[3]->str());
     TSM_ASSERT_EQUALS("azimuthal width wrong ",azw_pattern,bufs[4]->str());

  }
};
#endif
