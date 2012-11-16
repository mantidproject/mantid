#ifndef MANTID_GEOMETRY_IDFOBJECTTEST_H_
#define MANTID_GEOMETRY_IDFOBJECTTEST_H_

#include <cxxtest/TestSuite.h>
#include "MantidKernel/ConfigService.h"
#include "MantidGeometry/Instrument/IDFObject.h"
#include <Poco/Path.h>

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

  void testGetLastModified()
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

  void testGetFileFullPathStr()
  {
    const std::string filename = ConfigService::Instance().getInstrumentDirectory() + "/IDFs_for_UNIT_TESTING/IDF_for_UNIT_TESTING.xml";
	IDFObject obj(filename);
	TS_ASSERT_EQUALS(Poco::Path(filename).toString(), obj.getFileFullPathStr());

  }

};


#endif /* MANTID_GEOMETRY_IDFOBJECTTEST_H_ */
