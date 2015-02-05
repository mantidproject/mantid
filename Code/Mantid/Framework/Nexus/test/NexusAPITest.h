/*
 * NexusAPITest.h
 *
 * Tests of the Nexus C++ api.
 *
 *  Created on: Sep 22, 2010
 *      Author: janik
 */

#ifndef NEXUSAPITEST_H_
#define NEXUSAPITEST_H_

#include <cxxtest/TestSuite.h>

#include "MantidNexusCPP/NeXusFile.hpp"
#include "MantidNexusCPP/NeXusException.hpp"

#include <cmath>
#include <Poco/File.h>
#include <iostream>
#include <string>
#include <sstream>
#include <cstring>
#include <cstdlib>
#include <cstdio>
#include <vector>

using std::cout;
using std::endl;
using std::map;
using std::string;
using std::vector;


class NexusAPITest : public CxxTest::TestSuite
{
public:

  //Various data types that will be written
  vector<int> array_dims;
  vector<uint8_t> i1_array;
  vector<int16_t> i2_array;
  vector<int32_t> i4_array;
  vector<int64_t> i8_array;
  vector<float> r4_array;
  vector<double> r8_array;

  // This pair of boilerplate methods prevent the suite being created statically
  // This means the constructor isn't called when running other tests
  static NexusAPITest *createSuite() { return new NexusAPITest(); }
  static void destroySuite( NexusAPITest *suite ) { delete suite; }

  NexusAPITest()
  {

    // 2d array
    array_dims.push_back(5);
    array_dims.push_back(4);

    // 1d uint8 array
    for (size_t i = 0; i < 4; i++) {
      i1_array.push_back(static_cast<uint8_t>(i+1));
    }
    // 1d int16 array
    for (size_t i = 0; i < 4; i++) {
      i2_array.push_back(static_cast<int16_t>(1000*(i+1)));
    }
    // 1d int32 data
    for (size_t i = 0; i < 4; i++) {
      i4_array.push_back(static_cast<int32_t>(1000000*(i+1)));
    }
    // 1d int64 data
    for (size_t i = 0; i < 4; i++) {
      i8_array.push_back(1000000000*(i+1));
    }
    // 2d float data
    for (size_t i = 0; i < 5*4; i++) {
      r4_array.push_back(static_cast<float>(i));
    }
    for (size_t i = 0; i < 5*4; i++) {
      r8_array.push_back(static_cast<double>(i+20));
    }


  }

  /** Write out a test NXS file
   * Taken from napi_test_cpp.cxx  at http://download.nexusformat.org/doxygen/html-cpp/napi__test__cpp_8cxx-example.html
   * on Sep 22, 2010
   */
  int writeTest(const string& filename, NXaccess create_code)
  {
    ::NeXus::File file(filename, create_code);
    // create group
    file.makeGroup("entry", "NXentry", true);
    // group attributes
    file.putAttr("hugo", "namenlos");
    file.putAttr("cucumber", "passion");
    // put string
    file.writeData("ch_data", "NeXus_data");
    char c1_array[5][4] = {{'a', 'b', 'c' ,'d'}, {'e', 'f', 'g' ,'h'},
                           {'i', 'j', 'k', 'l'}, {'m', 'n', 'o', 'p'},
                           {'q', 'r', 's' , 't'}};
    file.makeData("c1_data", ::NeXus::CHAR, array_dims, true);
    file.putData(c1_array);
    file.closeData();

    //Write various types of data
    file.writeData("i1_data", i1_array);

    file.writeData("i2_data", i2_array);

    file.writeData("i4_data", i4_array);

    file.writeData("i8_data", i8_array);

    file.writeData("r4_data", r4_array, array_dims);

    file.writeData("r8_data_noslab", r8_array, array_dims);

    // 2d double data - slab test
    file.makeData("r8_data", ::NeXus::FLOAT64, array_dims, true);
    vector<int64_t> slab_start;
    slab_start.push_back(4);
    slab_start.push_back(0);
    vector<int64_t> slab_size;
    slab_size.push_back(1);
    slab_size.push_back(4);
    file.putSlab(&(r8_array[16]), slab_start, slab_size);
    slab_start[0] = 0;
    slab_start[1] = 0;
    slab_size[0]=4;
    slab_size[1]=4;
    file.putSlab(&(r8_array[0]), slab_start, slab_size);

    // add some attributes
    file.putAttr("ch_attribute", "NeXus");
    file.putAttr("i4_attribute", 42);
    file.putAttr("r4_attribute", M_PI);

    // set up for creating a link
    NXlink link = file.getDataID();
    file.closeData();

    // int64 tests
    vector<int64_t> grossezahl;
  #if HAVE_LONG_LONG_INT
    grossezahl.push_back(12);
    grossezahl.push_back(555555555555LL);
    grossezahl.push_back(23);
    grossezahl.push_back(777777777777LL);
  #else
    grossezahl.push_back(12);
    grossezahl.push_back(555555);
    grossezahl.push_back(23);
    grossezahl.push_back(77777);
  #endif
    if (create_code != NXACC_CREATE) {
      file.writeData("grosszahl", grossezahl);
    }

    // create a new group inside this one
    file.makeGroup("data", "NXdata", true);

    // create a link
    file.makeLink(link);

    // compressed data
    array_dims[0] = 100;
    array_dims[1] = 20;
    vector<int> comp_array;
    for (int i = 0; i < array_dims[0]; i++) {
      for (int j = 0; j < array_dims[1]; j++) {
        comp_array.push_back(i);
      }
    }
    vector<int> cdims;
    cdims.push_back(20);
    cdims.push_back(20);
    file.writeCompData("comp_data", comp_array, array_dims, ::NeXus::LZW, cdims);

    return 0;
  }


  void testGetDataCoerce()
  {
    //First, write out a test file (HDF5)s
    std::string filename("NexusAPITest.hdf");
    writeTest(filename,  NXACC_CREATE5);


    const string SDS("SDS");
    // top level file information
    ::NeXus::File file(filename);
    vector< ::NeXus::AttrInfo> attr_infos = file.getAttrInfos();
    // check group attributes
    file.openGroup("entry", "NXentry");

    //int num = 4;
    std::vector<int> ires;
    std::vector<double> dres;

    ires.clear();
    file.openData("i1_data");
    TS_ASSERT( file.isDataInt() );
    file.getDataCoerce(ires);
    TS_ASSERT_EQUALS(ires.size(), i1_array.size());
    for (std::size_t i=0; i<ires.size();i++)
      TS_ASSERT_EQUALS(ires[i], i1_array[i]);
    file.closeData();

    ires.clear();
    file.openData("i2_data");
    TS_ASSERT( file.isDataInt() );
    file.getDataCoerce(ires);
    TS_ASSERT_EQUALS(ires.size(), i2_array.size());
    for (std::size_t i=0; i<ires.size();i++)
      TS_ASSERT_EQUALS(ires[i], i2_array[i]);
    file.closeData();

    ires.clear();
    file.openData("i4_data");
    TS_ASSERT( file.isDataInt() );
    file.getDataCoerce(ires);
    TS_ASSERT_EQUALS(ires.size(), i4_array.size());
    for (std::size_t i=0; i<ires.size();i++)
      TS_ASSERT_EQUALS(ires[i], i4_array[i]);
    file.closeData();

    dres.clear();
    file.openData("r4_data");
    file.getDataCoerce(dres);
    TS_ASSERT_EQUALS(dres.size(), r4_array.size());
    for (std::size_t i=0; i<dres.size();i++)
      TS_ASSERT_EQUALS(dres[i], r4_array[i]);
    file.closeData();

    dres.clear();
    file.openData("r8_data");
    file.getDataCoerce(dres);
    TS_ASSERT_EQUALS(dres.size(), r8_array.size());
    for (std::size_t i=0; i<dres.size();i++)
      TS_ASSERT_EQUALS(dres[i], r8_array[i]);
    file.closeData();

    //Now make it throw Exception by trying to put a double in an int.
    file.openData("r8_data");
    TS_ASSERT( !file.isDataInt() );
    TS_ASSERT_THROWS(file.getDataCoerce(ires), ::NeXus::Exception);
    file.closeData();

    file.openData("r4_data");
    TS_ASSERT( !file.isDataInt() );
    TS_ASSERT_THROWS(file.getDataCoerce(ires), ::NeXus::Exception);
    file.closeData();

    // Remove the file. Windows requires it to be closed first
    file.close();
    Poco::File(filename).remove();
  }

  void test_writeData()
  {
    std::string filename("NexusAPITest1.hdf");
    ::NeXus::File * file = new ::NeXus::File(filename, NXACC_CREATE5);
    std::vector<int> data(10, 123);
    file->makeGroup("data", "NXdata", 1);
    file->writeData("mydata", data);
    file->putAttr("empty_string", ""); // Don't want this to error
    file->close();
  }

  void test_writeExtendibleData_writeUpdatedData()
  {
    std::string filename("NexusAPITest2.hdf");
    ::NeXus::File * file = new ::NeXus::File(filename, NXACC_CREATE5);
    std::vector<int> data(10, 123);
    file->makeGroup("data", "NXdata", 1);
    file->writeExtendibleData("mydata1", data);
    file->writeExtendibleData("mydata2", data, 1000);
    std::vector<int> dims(2);
    dims[0] = 5;
    dims[1] = 2;
    std::vector<int> chunk(2, 2);
    file->writeExtendibleData("my2Ddata", data, dims, chunk);
    file->putAttr("string_attrib", "some short string");
    file->close();

    // Data vector can grow
    for (size_t i=0; i<6; i++)
      data.push_back(456);
    data[0]=789;
    file = new ::NeXus::File(filename, NXACC_RDWR);
    file->openGroup("data", "NXdata");
    file->writeUpdatedData("mydata1", data);

    dims[0] = 8;
    dims[1] = 2;
    file->writeUpdatedData("my2Ddata", data, dims);

    // Data vector can also shrink!
    data.clear();
    data.resize(5, 234);
    file->writeUpdatedData("mydata2", data);

    // Also a string
    file->putAttr("string_attrib", "some much longer string filled with interesting comments that I'm sure you are glad you are reading right now 42.");
    file->close();
  }

};


#endif




