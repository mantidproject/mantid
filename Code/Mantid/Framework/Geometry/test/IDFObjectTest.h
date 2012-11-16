#ifndef MANTID_GEOMETRY_IDFOBJECTTEST_H_
#define MANTID_GEOMETRY_IDFOBJECTTEST_H_

#include <cxxtest/TestSuite.h>
#include "MantidKernel/ConfigService.h"
#include "MantidGeometry/Instrument/IDFObject.h"
#include "MantidTestHelpers/ScopedFileHelper.h"
#include <Poco/Path.h>
#include <Poco/DateTimeFormatter.h>
#include <Poco/Thread.h>

using Mantid::Geometry::IDFObject;
using Mantid::Kernel::ConfigService;

class IDFObjectTest : public CxxTest::TestSuite
{
public:
  // This pair of boilerplate methods prevent the suite being created statically
  // This means the constructor isn't called when running other tests
  static IDFObjectTest *createSuite() { return new IDFObjectTest(); }
  static void destroySuite( IDFObjectTest *suite ) { delete suite; }

  void testExpectedExtensionIsXML()
  {
    TS_ASSERT_EQUALS(".xml", IDFObject::expectedExtension());
  }

  void testExists()
  {
    const std::string filename = ConfigService::Instance().getInstrumentDirectory() + "/IDFs_for_UNIT_TESTING/IDF_for_UNIT_TESTING.xml";
    IDFObject obj(filename);
    TS_ASSERT(obj.exists());
  }

  void testDoesntExistIfEmpty()
  {
    IDFObject obj("");
    TS_ASSERT(!obj.exists());
  }

  void testDoesntExist()
  {
    const std::string filename = "made_up_file.xml";
    IDFObject obj(filename);
    TS_ASSERT(!obj.exists());
  }

  void testGetParentDirectory()
  {
    const Poco::Path expectedDir = Poco::Path( ConfigService::Instance().getInstrumentDirectory() + "/IDFs_for_UNIT_TESTING/" );
    std::string filename = expectedDir.toString() + "IDF_for_UNIT_TESTING.xml";
    IDFObject obj(filename);
    TS_ASSERT_EQUALS(expectedDir.toString(), obj.getParentDirectory().toString());
  }

  void testGetFullPath()
  {
    const std::string filename = ConfigService::Instance().getInstrumentDirectory() + "/IDFs_for_UNIT_TESTING/IDF_for_UNIT_TESTING.xml";
    IDFObject obj(filename);
    TS_ASSERT_EQUALS(Poco::Path(filename).toString(), obj.getFileFullPath().toString());
  }

  void testGetExtension()
  {
    const std::string filename = ConfigService::Instance().getInstrumentDirectory() + "/IDFs_for_UNIT_TESTING/IDF_for_UNIT_TESTING.xml";
    IDFObject obj(filename);
    TS_ASSERT_EQUALS(".xml", obj.getExtension());
  }

  void testGetFileNameOnly()
  {
    const std::string filenameonly = "IDF_for_UNIT_TESTING.xml";
    const std::string filename = ConfigService::Instance().getInstrumentDirectory() + "/IDFs_for_UNIT_TESTING/" + filenameonly;
    IDFObject obj(filename);
    TS_ASSERT_EQUALS(filenameonly, obj.getFileNameOnly());
  }

  void testGetModifiedTimestamp()
  {
    const std::string filename = ConfigService::Instance().getInstrumentDirectory() + "/IDFs_for_UNIT_TESTING/IDF_for_UNIT_TESTING.xml";
    Poco::File file(filename);

    IDFObject obj(filename);
    TS_ASSERT_EQUALS(file.getLastModified(), obj.getLastModified());
  }

  void testGetFormattedModifiedTimestamp()
  {
    const std::string filename = ConfigService::Instance().getInstrumentDirectory() + "/IDFs_for_UNIT_TESTING/IDF_for_UNIT_TESTING.xml";
    Poco::File file(filename);

    IDFObject obj(filename);
    TS_ASSERT_EQUALS(Poco::DateTimeFormatter::format(file.getLastModified(), "%Y-%d-%mT%H:%M:%S"), obj.getFormattedLastModified());
  }

  void testGetMangledName()
  {
    const std::string filename = ConfigService::Instance().getInstrumentDirectory() + "/IDFs_for_UNIT_TESTING/IDF_for_UNIT_TESTING.xml";
    
    Poco::File file(filename);
    auto head = "IDF_for_UNIT_TESTING.xml";
    auto tail = Poco::DateTimeFormatter::format(file.getLastModified(), "%Y-%d-%mT%H:%M:%S");
    
    IDFObject obj(filename);

    TS_ASSERT_EQUALS(head+tail, obj.getMangledName());
  }

  // Test that the last modified date fetched on each platform does actually make sense!
  void testGetModifiedTimestampAfterChange()
  {
    const std::string fileName = "check_last_modified_date.xml";
    const std::string fileContents = "some_idf_contents_that_donesn't_matter";
    // Create a file.
    ScopedFileHelper::ScopedFile file(fileContents, fileName);
    IDFObject IDF(file.getFileName());
    // Record the modification timestamp
    Poco::Timestamp timeOfCreation = IDF.getLastModified(); 
    // Delay
    const int delay = 1;
    Poco::Thread::sleep(delay);
    // Modify the file.
    std::ofstream modIDF;
    modIDF.open(IDF.getFileFullPathStr(), std::ios::out | std::ios::app);
    if (!modIDF.is_open())
    {
      throw std::runtime_error("Cannot run test since file cannot be opened.");
    }
    modIDF << "\nchange" << std::endl;
    modIDF.close();
    // Record the modification timestamp.
    Poco::Timestamp timeOfModification = IDF.getLastModified(); 
    
    // Compare the modification dates.
    TSM_ASSERT_LESS_THAN("The file modification dates do not reflect the fact that the file has been modified.", timeOfCreation, timeOfModification);
  }

  void testGetFileFullPathStr()
  {
    const std::string filename = ConfigService::Instance().getInstrumentDirectory() + "/IDFs_for_UNIT_TESTING/IDF_for_UNIT_TESTING.xml";
    IDFObject obj(filename);
    TS_ASSERT_EQUALS(Poco::Path(filename).toString(), obj.getFileFullPathStr());
  }

};


#endif /* MANTID_GEOMETRY_IDFOBJECTTEST_H_ */
