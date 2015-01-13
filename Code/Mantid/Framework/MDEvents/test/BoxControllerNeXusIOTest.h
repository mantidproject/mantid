#ifndef BOXCONTROLLER_NEXUS_IO_TEST_H
#define BOXCONTROLLER_NEXUS_IO_TEST_H

#include <cxxtest/TestSuite.h>
#include <map>
#include <memory>
#include <Poco/File.h>
#include <nexus/NeXusFile.hpp>
#include "MantidTestHelpers/MDEventsTestHelper.h"
#include "MantidMDEvents/BoxControllerNeXusIO.h"
#include "MantidAPI/FileFinder.h"

using namespace Mantid;
using namespace Mantid::Geometry;
using namespace Mantid::Kernel;
using namespace Mantid::API;
//using namespace Mantid::MDEvens;

class BoxControllerNeXusIOTest : public CxxTest::TestSuite
{
    BoxController_sptr sc;
    std::string xxfFileName;


    BoxControllerNeXusIOTest()
    {
        sc = BoxController_sptr(new BoxController(4));
        xxfFileName= "BoxCntrlNexusIOxxfFile.nxs";
    }



public:
static BoxControllerNeXusIOTest *createSuite() { return new BoxControllerNeXusIOTest(); }
static void destroySuite(BoxControllerNeXusIOTest * suite) { delete suite; }    

void setUp()
{
    std::string FullPathFile = API::FileFinder::Instance().getFullPath(this->xxfFileName);
    if(!FullPathFile.empty())
            Poco::File(FullPathFile).remove();   

}

 void test_contstructor_setters()
 {

     MDEvents::BoxControllerNeXusIO *pSaver(NULL);
     TS_ASSERT_THROWS_NOTHING(pSaver=new MDEvents::BoxControllerNeXusIO(sc.get()));

     size_t CoordSize;
     std::string typeName;
     TS_ASSERT_THROWS_NOTHING(pSaver->getDataType(CoordSize, typeName));
     // default settings
     TS_ASSERT_EQUALS(4,CoordSize);
     TS_ASSERT_EQUALS("MDEvent",typeName);

     //set size
    TS_ASSERT_THROWS(pSaver->setDataType(9,typeName),std::invalid_argument);
    TS_ASSERT_THROWS_NOTHING(pSaver->setDataType(8, typeName));
    TS_ASSERT_THROWS_NOTHING(pSaver->getDataType(CoordSize, typeName));
    TS_ASSERT_EQUALS(8,CoordSize);
    TS_ASSERT_EQUALS("MDEvent",typeName);

     //set type
    TS_ASSERT_THROWS(pSaver->setDataType(4,"UnknownEvent"),std::invalid_argument);
    TS_ASSERT_THROWS_NOTHING(pSaver->setDataType(4, "MDLeanEvent"));
    TS_ASSERT_THROWS_NOTHING(pSaver->getDataType(CoordSize, typeName));
    TS_ASSERT_EQUALS(4,CoordSize);
    TS_ASSERT_EQUALS("MDLeanEvent",typeName);


     delete pSaver;
 }

 void test_CreateOrOpenFile()
 {
     MDEvents::BoxControllerNeXusIO *pSaver(NULL);
     TS_ASSERT_THROWS_NOTHING(pSaver=new MDEvents::BoxControllerNeXusIO(sc.get()));
     pSaver->setDataType(sizeof(coord_t),"MDLeanEvent");
     std::string FullPathFile;

     TSM_ASSERT_THROWS("new file does not open in read mode",pSaver->openFile(this->xxfFileName,"r"), Kernel::Exception::FileError);

     TS_ASSERT_THROWS_NOTHING(pSaver->openFile(this->xxfFileName,"w"));
     TS_ASSERT_THROWS_NOTHING(FullPathFile = pSaver->getFileName());
     TS_ASSERT(pSaver->isOpened());
     TS_ASSERT_THROWS_NOTHING(pSaver->closeFile());
     TS_ASSERT(!pSaver->isOpened());

     TSM_ASSERT("file created ",!API::FileFinder::Instance().getFullPath(FullPathFile).empty());

     // now I can open this file for reading 
     TS_ASSERT_THROWS_NOTHING(pSaver->openFile(FullPathFile,"r"));
     TS_ASSERT_THROWS_NOTHING(FullPathFile = pSaver->getFileName());
     TS_ASSERT(pSaver->isOpened());
     TS_ASSERT_THROWS_NOTHING(pSaver->closeFile());
     TS_ASSERT(!pSaver->isOpened());

     // now I can open this file for writing
     TS_ASSERT_THROWS_NOTHING(pSaver->openFile(FullPathFile,"W"));
     TS_ASSERT_THROWS_NOTHING(FullPathFile = pSaver->getFileName());
     TS_ASSERT(pSaver->isOpened());
     TS_ASSERT_THROWS_NOTHING(pSaver->closeFile());
     TS_ASSERT(!pSaver->isOpened());

     delete pSaver;
     if(Poco::File(FullPathFile).exists())
         Poco::File(FullPathFile).remove();   
 }

 void test_free_space_index_is_written_out_and_read_in()
 {
     MDEvents::BoxControllerNeXusIO *pSaver(NULL);
     TS_ASSERT_THROWS_NOTHING(pSaver=new MDEvents::BoxControllerNeXusIO(sc.get()));
     std::string FullPathFile;

     TS_ASSERT_THROWS_NOTHING(pSaver->openFile(this->xxfFileName,"w"));
     TS_ASSERT_THROWS_NOTHING(FullPathFile = pSaver->getFileName());

     std::vector<uint64_t> freeSpaceVectorToSet;
     for (uint64_t i = 0; i < 20; i++) {
         freeSpaceVectorToSet.push_back(i);
     }
     pSaver->setFreeSpaceVector(freeSpaceVectorToSet);

     TS_ASSERT_THROWS_NOTHING(pSaver->closeFile());

     TS_ASSERT(!pSaver->isOpened());

     TS_ASSERT_THROWS_NOTHING(pSaver->openFile(this->xxfFileName,"w"));

     std::vector<uint64_t> freeSpaceVectorToGet;
     pSaver->getFreeSpaceVector(freeSpaceVectorToGet);

     TS_ASSERT_EQUALS(freeSpaceVectorToSet, freeSpaceVectorToGet);

     TS_ASSERT_THROWS_NOTHING(pSaver->closeFile());

     delete pSaver;
     if(Poco::File(FullPathFile).exists())
         Poco::File(FullPathFile).remove();
 }

 //---------------------------------------------------------------------------------------------------------
 // tests to read/write double/vs float events
 template<typename FROM,typename TO>
 struct IF   // if in/out formats are different we can not read different data format from it
 {
 public:
     static void compareReadTheSame(API::IBoxControllerIO *pSaver,const std::vector<FROM> &/*inputData*/,size_t /*nEvents*/,size_t /*nColumns*/)
     {
         TS_ASSERT(pSaver->isOpened());
         TS_ASSERT_THROWS_NOTHING(pSaver->closeFile());
         TS_ASSERT(!pSaver->isOpened());

     }
 };
 template<typename FROM>
 struct IF<FROM,FROM>    // if in/out formats are the same, we can read what was written earlier
 {
 public:
     static void compareReadTheSame(API::IBoxControllerIO *pSaver,const std::vector<FROM> &inputData,size_t nEvents,size_t nColumns)
     {
        std::vector<FROM> toRead;
        TS_ASSERT_THROWS_NOTHING(pSaver->loadBlock(toRead,100,nEvents));
        for(size_t i=0;i<nEvents*nColumns;i++)
        {
         TS_ASSERT_DELTA(inputData[i],toRead[i],1.e-6);
        }

         TS_ASSERT(pSaver->isOpened());
         TS_ASSERT_THROWS_NOTHING(pSaver->closeFile());
         TS_ASSERT(!pSaver->isOpened());

     }
 };

 template<typename FROM,typename TO>
 void WriteReadRead()
 {
     MDEvents::BoxControllerNeXusIO *pSaver(NULL);
     TS_ASSERT_THROWS_NOTHING(pSaver=new MDEvents::BoxControllerNeXusIO(sc.get()));
     pSaver->setDataType(sizeof(FROM),"MDEvent");
     std::string FullPathFile;

     TS_ASSERT_THROWS_NOTHING(pSaver->openFile(this->xxfFileName,"w"));
     TS_ASSERT_THROWS_NOTHING(FullPathFile = pSaver->getFileName());

     size_t nEvents=20;
     // the number of colums corresponfs to 
     size_t nColumns=pSaver->getNDataColums();
     std::vector<FROM> toWrite(nColumns*nEvents);
     for(size_t i = 0;i<nEvents;i++)
     {
         for(size_t j=0;j<nColumns;j++)
         {
             toWrite[i*nColumns+j]=static_cast<FROM>(j+10*i);
         }
     }
     
     TS_ASSERT_THROWS_NOTHING(pSaver->saveBlock(toWrite,100));

     IF<FROM,TO>::compareReadTheSame(pSaver,toWrite,nEvents,nColumns);

     // open and read what was written,
     pSaver->setDataType(sizeof(TO),"MDEvent");
     TS_ASSERT_THROWS_NOTHING(pSaver->openFile(FullPathFile,"r"));
     std::vector<TO> toRead2;
     TS_ASSERT_THROWS_NOTHING(pSaver->loadBlock(toRead2,100+(nEvents-1),1));
     for(size_t i=0;i<nColumns;i++)
     {
         TS_ASSERT_DELTA(toWrite[(nEvents-1)*nColumns+i],toRead2[i],1.e-6);
     }


     delete pSaver;
     if(Poco::File(FullPathFile).exists())
         Poco::File(FullPathFile).remove();   

 }

 void test_WriteFloatReadReadFloat()
 {
     this->WriteReadRead<float,float>();
 }
void test_WriteFloatReadReadDouble()
 {
     this->WriteReadRead<double,double>();
 }
 void test_WriteDoubleReadFloat()
 {
     this->WriteReadRead<double,float>();
 }

 void test_WriteFloatReadDouble()
 {
     this->WriteReadRead<float,double>();
 }
};
#endif
