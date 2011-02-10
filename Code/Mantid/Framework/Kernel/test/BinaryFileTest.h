#ifndef BINARYFILETEST_H_
#define BINARYFILETEST_H_

#include <cxxtest/TestSuite.h>
#include "MantidKernel/BinaryFile.h"
#include "MantidKernel/System.h"
#include "MantidKernel/ConfigService.h"
#include <sys/stat.h>

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


/** Creates a dummy file with so many bytes */
static void MakeDummyFile(std::string filename, size_t num_bytes)
{
  char * buffer;
  buffer = new char[num_bytes];
  for (size_t i=0; i < num_bytes; i++)
  {
    // Put 1,2,3 in 32-bit ints
    if (i%4==0)
      buffer[i]=i/4;
    else
      buffer[i]=0;
  }

  std::ofstream myFile (filename.c_str(), std::ios::out | std::ios::binary);
  myFile.write (buffer, num_bytes);
  delete [] buffer;
  myFile.close();
}


//==========================================================================================
class BinaryFileTest: public CxxTest::TestSuite
{
private:
  BinaryFile<DasEvent> file;
  std::string dummy_file;

public:
  static BinaryFileTest *createSuite() { return new BinaryFileTest(); }
  static void destroySuite(BinaryFileTest *suite) { delete suite; }

  BinaryFileTest()
  {
    dummy_file = "dummy.bin";
  }

  void testFileNotFound()
  {
    TS_ASSERT_THROWS( file.open("nonexistentfile.dat"), std::invalid_argument);
  }

 void testFileWrongSize()
 {

   MakeDummyFile(dummy_file, 3);
   TS_ASSERT_THROWS( file.open(dummy_file), std::runtime_error);
   file.close();
   Poco::File(dummy_file).remove();
 }


 void testOpen()
 {
   MakeDummyFile(dummy_file, 20*8);

   // If this throws, then the file does not exist.
   file.open(dummy_file);
   //Right size?
   size_t num = 20;
   TS_ASSERT_EQUALS(file.getNumElements(), num);
   //Get it
   std::vector<DasEvent> * data;
   TS_ASSERT_THROWS_NOTHING( data = file.loadAll() );
   TS_ASSERT_EQUALS(data->size(), num);
   //Check the first event
   TS_ASSERT_EQUALS( data->at(0).tof, 0);
   TS_ASSERT_EQUALS( data->at(0).pid, 1);
   //Check the last event
   TS_ASSERT_EQUALS( data->at(num-1).tof, 38);
   TS_ASSERT_EQUALS( data->at(num-1).pid, 39);

   delete data;
   file.close();
   Poco::File(dummy_file).remove();
 }

 void testLoadAllInto()
 {
   MakeDummyFile(dummy_file, 20*8);
   file.open(dummy_file);

   //Right size?
   size_t num = 20;
   TS_ASSERT_EQUALS(file.getNumElements(), num);
   //Get it
   std::vector<DasEvent> data;
   TS_ASSERT_THROWS_NOTHING( file.loadAllInto(data) );
   TS_ASSERT_EQUALS(data.size(), num);
   //Check the first event
   TS_ASSERT_EQUALS( data.at(0).tof, 0);
   TS_ASSERT_EQUALS( data.at(0).pid, 1);
   //Check the last event
   TS_ASSERT_EQUALS( data.at(num-1).tof, 38);
   TS_ASSERT_EQUALS( data.at(num-1).pid, 39);
   file.close();
   Poco::File(dummy_file).remove();
 }

 void testLoadInBlocks()
 {
   MakeDummyFile(dummy_file, 20*8);
   file.open(dummy_file);

   //Right size?
   size_t num = 20;
   TS_ASSERT_EQUALS(file.getNumElements(), num);
   //Get it
   size_t block_size = 10;
   size_t loaded_size = -1;
   DasEvent * data = new DasEvent[block_size];
   loaded_size = file.loadBlock(data, block_size);
   //Yes, we loaded that amount
   TS_ASSERT_EQUALS(loaded_size, block_size);

   //Check the first event
   TS_ASSERT_EQUALS( data[0].tof, 0);
   TS_ASSERT_EQUALS( data[0].pid, 1);

   delete [] data;
   //Now try to load a lot more - going past the end
   block_size = 10;
   data = new DasEvent[block_size];
   loaded_size = file.loadBlock(data, block_size);
   TS_ASSERT_EQUALS(loaded_size, 10);

   //Check the last event
   TS_ASSERT_EQUALS( data[ 9].tof, 38);
   TS_ASSERT_EQUALS( data[ 9].pid, 39);
   delete [] data;
   file.close();
   Poco::File(dummy_file).remove();
 }


  void testCallingDestructorOnUnitializedObject()
  {
    BinaryFile<DasEvent> file2;
  }

  void testReadingNotOpenFile()
  {
    BinaryFile<DasEvent> file2;
    std::vector<DasEvent> data;
    DasEvent * buffer = NULL;
    TS_ASSERT_EQUALS(file2.getNumElements(), 0);
    TS_ASSERT_THROWS(file2.getFileSize(), std::runtime_error );
    TS_ASSERT_THROWS(file2.loadAll(), std::runtime_error );
    TS_ASSERT_THROWS(file2.loadAllInto( data), std::runtime_error );
    TS_ASSERT_THROWS(file2.loadBlock(buffer, 10), std::runtime_error );
  }

};

#endif /*BINARYFILETEST_H_*/
