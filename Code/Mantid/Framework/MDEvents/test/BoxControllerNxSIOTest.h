#ifndef BOXCONTROLLER_NEXUS_IO_TEST_H
#define BOXCONTROLLER_NEXUS_IO_TEST_H

#include <cxxtest/TestSuite.h>
#include <map>
#include <memory>
#include <Poco/File.h>
#include <nexus/NeXusFile.hpp>
#include "MantidTestHelpers/MDEventsTestHelper.h"
#include "MantidMDEvents/BoxControllerNxSIO.h"
#include "MantidAPI/FileFinder.h"

using namespace Mantid;
using namespace Mantid::Geometry;
using namespace Mantid::Kernel;
using namespace Mantid::API;
//using namespace Mantid::MDEvens;

class BoxControllerNxSIOTest : public CxxTest::TestSuite
{
    BoxController_sptr sc;
    std::string testFileName;

    BoxControllerNxSIOTest()
    {
        sc = BoxController_sptr(new BoxController(4));
        testFileName= "BoxCntrlNexusIOtestFile.nxs";
    }



public:
static BoxControllerNxSIOTest *createSuite() { return new BoxControllerNxSIOTest(); }
static void destroySuite(BoxControllerNxSIOTest * suite) { delete suite; }    

void setUp()
{
    std::string FullPathFile = API::FileFinder::Instance().getFullPath(this->testFileName);
    if(!FullPathFile.empty())
            Poco::File(FullPathFile).remove();   

}

 void test_contstructor_setters()
 {

     MDEvents::BoxControllerNxSIO *pSaver;
     TS_ASSERT_THROWS_NOTHING(pSaver=new MDEvents::BoxControllerNxSIO(sc));

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

 void testCreateOrOpenFile()
 {
     MDEvents::BoxControllerNxSIO *pSaver;
     TS_ASSERT_THROWS_NOTHING(pSaver=new MDEvents::BoxControllerNxSIO(sc));

     std::string FullPathFile;

     TSM_ASSERT_THROWS("new file does not open in read mode",pSaver->openFile(this->testFileName,"r"), Kernel::Exception::FileError);

     TS_ASSERT_THROWS_NOTHING(pSaver->openFile(this->testFileName,"w"));
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

 void testWriteReadReadFloat()
 {
     MDEvents::BoxControllerNxSIO *pSaver;
     TS_ASSERT_THROWS_NOTHING(pSaver=new MDEvents::BoxControllerNxSIO(sc));
      std::string FullPathFile;

     TS_ASSERT_THROWS_NOTHING(pSaver->openFile(this->testFileName,"w"));
     TS_ASSERT_THROWS_NOTHING(FullPathFile = pSaver->getFileName());

     size_t nEvents=20;
     // the number of colums corresponfs to 
     size_t nColumns=pSaver->getNDataColums();
     std::vector<float> toWrite(nColumns*nEvents);
     for(size_t i = 0;i<nEvents;i++)
     {
         for(size_t j=0;j<nColumns;j++)
         {
             toWrite[i*nColumns+j]=float(j+10*i);
         }
     }
     
     TS_ASSERT_THROWS_NOTHING(pSaver->saveBlock(toWrite,100));

     std::vector<float> toRead;
     TS_ASSERT_THROWS_NOTHING(pSaver->loadBlock(toRead,100,nEvents));
     for(size_t i=0;i<nEvents*nColumns;i++)
     {
         TS_ASSERT_DELTA(toWrite[i],toRead[i],1.e-6);
     }

     TS_ASSERT(pSaver->isOpened());
     TS_ASSERT_THROWS_NOTHING(pSaver->closeFile());
     TS_ASSERT(!pSaver->isOpened());


     TS_ASSERT_THROWS_NOTHING(pSaver->openFile(FullPathFile,"r"));
     std::vector<float> toRead2;
     TS_ASSERT_THROWS_NOTHING(pSaver->loadBlock(toRead2,100+(nEvents-1),1));
     for(size_t i=0;i<nColumns;i++)
     {
         TS_ASSERT_DELTA(toWrite[(nEvents-1)*nColumns+i],toRead2[i],1.e-6);
     }


     delete pSaver;
     if(Poco::File(FullPathFile).exists())
         Poco::File(FullPathFile).remove();   

 }


};
#endif