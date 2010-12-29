#ifndef BINARYFILETEST_H_
#define BINARYFILETEST_H_

#include <cxxtest/TestSuite.h>
#include "MantidKernel/BinaryFile.h"
#include "MantidKernel/System.h"
#include <sys/stat.h>

using namespace Mantid;
using namespace Mantid::Kernel;

using std::runtime_error;
using std::size_t;
using std::vector;
using std::cout;
using std::endl;

//==========================================================================================
/// Make the code clearer by having this an explicit type
typedef uint32_t PixelType;
/// Type for the DAS time of flight (data file)
typedef uint32_t DasTofType;
/// Structure that matches the form in the binary event list.
struct DasEvent
{
  /// Time of flight.
  DasTofType tof;
  /// Pixel identifier as published by the DAS/DAE/DAQ.
  PixelType pid;
};

//==========================================================================================
class BinaryFileTest: public CxxTest::TestSuite
{
public:
  BinaryFile<DasEvent> file;

  void testFileNotFound()
  {
    TS_ASSERT_THROWS( file.open("nonexistentfile.dat"), std::invalid_argument);
  }

  void testFileWrongSize()
  {
    TS_ASSERT_THROWS( file.open("../../../../../Test/AutoTestData/DataFileOfBadSize.dat"), std::runtime_error);
  }

  void testOpen()
  {
    TS_ASSERT_THROWS_NOTHING( file.open("../../../../../Test/AutoTestData/REF_L_32035_neutron_event.dat"));
    //Right size?
    size_t num = 400104/8;
    TS_ASSERT_EQUALS(file.getNumElements(), num);
    //Get it
    std::vector<DasEvent> * data;
    TS_ASSERT_THROWS_NOTHING( data = file.loadAll() );
    TS_ASSERT_EQUALS(data->size(), num);
    //Check the first event
    TS_ASSERT_EQUALS( data->at(0).tof, 0x25781);
    TS_ASSERT_EQUALS( data->at(0).pid, 0x8d82);
    //Check the last event
    TS_ASSERT_EQUALS( data->at(num-1).tof, 0x3163f);
    TS_ASSERT_EQUALS( data->at(num-1).pid, 0x9883);

    delete data;
  }

  void testLoadAllInto()
  {
    TS_ASSERT_THROWS_NOTHING( file.open("../../../../../Test/AutoTestData/REF_L_32035_neutron_event.dat"));
    //Right size?
    size_t num = 400104/8;
    TS_ASSERT_EQUALS(file.getNumElements(), num);
    //Get it
    std::vector<DasEvent> data;
    TS_ASSERT_THROWS_NOTHING( file.loadAllInto(data) );
    TS_ASSERT_EQUALS(data.size(), num);
    //Check the first event
    TS_ASSERT_EQUALS( data.at(0).tof, 0x25781);
    TS_ASSERT_EQUALS( data.at(0).pid, 0x8d82);
    //Check the last event
    TS_ASSERT_EQUALS( data.at(num-1).tof, 0x3163f);
    TS_ASSERT_EQUALS( data.at(num-1).pid, 0x9883);
  }

  void testLoadInBlocks()
  {
    TS_ASSERT_THROWS_NOTHING( file.open("../../../../../Test/AutoTestData/REF_L_32035_neutron_event.dat"));
    //Right size?
    size_t num = 400104/8;
    TS_ASSERT_EQUALS(file.getNumElements(), num);
    //Get it
    size_t block_size = 1000;
    size_t loaded_size = -1;
    DasEvent * data = new DasEvent[block_size];
    loaded_size = file.loadBlock(data, block_size);
    //Yes, we loaded that amount
    TS_ASSERT_EQUALS(loaded_size, block_size);

    //Check the first event
    TS_ASSERT_EQUALS( data[0].tof, 0x25781);
    TS_ASSERT_EQUALS( data[0].pid, 0x8d82);

    delete [] data;
    //Now try to load a lot more
    block_size = 1000000; // 1 million
    data = new DasEvent[block_size];
    loaded_size = file.loadBlock(data, block_size);
    TS_ASSERT_EQUALS(loaded_size, num - 1000);

    //Check the last event
    TS_ASSERT_EQUALS( data[ num - 1001].tof, 0x3163f);
    TS_ASSERT_EQUALS( data[ num - 1001].pid, 0x9883);

  }

};

#endif /*BINARYFILETEST_H_*/
