// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2025 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include "MantidNexus/NexusException.h"
#include "MantidNexus/UniqueID.h"

#include "test_helper.h"

#include <chrono>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <cxxtest/TestSuite.h>
#include <hdf5.h>
#include <thread>

using namespace NexusTest;
using std::cout;
using std::endl;
using std::map;
using std::multimap;
using std::string;
using std::vector;

using GroupID = Mantid::Nexus::UniqueID<&H5Gclose>;
using ParameterID = Mantid::Nexus::UniqueID<&H5Pclose>;
using UniqueFileID = Mantid::Nexus::UniqueID<&H5Fclose>;
using SharedFileID = Mantid::Nexus::SharedID<&H5Fclose>;

namespace {
// an ID which will always return as valid
const hid_t GOOD_ID1 = H5T_NATIVE_INT;
const hid_t GOOD_ID2 = H5T_NATIVE_CHAR;
const hid_t BAD_ID = 101;
static int call_count = 0;
int blank_deleter(hid_t) { return ++call_count; }
} // namespace

using TestHdf5ID = Mantid::Nexus::Hdf5ID<blank_deleter>;
using TestUniqueID = Mantid::Nexus::UniqueID<blank_deleter>;
using TestSharedID = Mantid::Nexus::SharedID<blank_deleter>;
using Mantid::Nexus::INVALID_HID;

class UniqueIDTest : public CxxTest::TestSuite {

public:
  // // This pair of boilerplate methods prevent the suite being created statically
  // // This means the constructor isn't called when running other tests
  static UniqueIDTest *createSuite() { return new UniqueIDTest(); }
  static void destroySuite(UniqueIDTest *suite) { delete suite; }

  void setUp() override { call_count = 0; }

  // ******************************************************************
  // HDF ID -- tests of basic functionality
  // ******************************************************************

  void test_hdf5ID_isValid() {
    cout << "\ntest hdf5ID isValid" << endl;

    // if set with default value, will be invalid
    {
      TestHdf5ID uid;
      TS_ASSERT(!uid.isValid());
    }

    // if set with the invalid ID, will be invalid
    {
      TestHdf5ID uid(INVALID_HID);
      TS_ASSERT(!uid.isValid());
    }

    // if set with a positive integer that is not actually an hdf5 object, will be invalid
    hid_t test = BAD_ID;
    TS_ASSERT(!H5Iis_valid(test));
    {
      TestHdf5ID uid(test);
      TS_ASSERT(!uid.isValid());
    }

    // if set with a valid identifier, returns valid
    hid_t good = GOOD_ID1;
    TS_ASSERT(H5Iis_valid(good));
    {
      TestHdf5ID uid(good);
      TS_ASSERT(uid.isValid());
    }
  }

  void test_hdf5ID_construct_empty() {
    cout << "\ntest hdf5ID constructor empty" << endl;
    // construct empty
    {
      TestHdf5ID uid;
      TS_ASSERT_EQUALS(uid.get(), INVALID_HID);
      TS_ASSERT(!uid.isValid());
    }
  }

  void test_hdf5ID_construct() {
    cout << "\ntest hdf5ID construct" << endl;
    // construct
    hid_t test = GOOD_ID1;
    {
      TestHdf5ID uid(test);
      TS_ASSERT_EQUALS(uid.get(), test);
      TS_ASSERT_DIFFERS(uid.get(), INVALID_HID);
      TS_ASSERT(uid.isValid());
    }
    // deleter not called on exit
    TS_ASSERT_EQUALS(call_count, 0);
  }

  void test_file_is_closed() {
    cout << "\ntest closing files" << std::endl;
    H5Eset_auto(H5E_DEFAULT, nullptr, nullptr);

    FileResource resource("test_file_is_closed_fixture.nxs");
    std::string filename{resource.fullPath()};
    ParameterID fapl = H5Pcreate(H5P_FILE_ACCESS);
    H5Pset_fclose_degree(fapl, H5F_CLOSE_STRONG);
    UniqueFileID file = H5Fcreate(filename.c_str(), H5F_ACC_TRUNC, H5P_DEFAULT, fapl);
    TS_ASSERT(!hdf_file_is_closed(filename));
    file.reset();
    TS_ASSERT(hdf_file_is_closed(filename));
    { // scope the file to check deconstructor
      UniqueFileID file2 = H5Fopen(filename.c_str(), H5F_ACC_RDONLY, fapl);
      TS_ASSERT(!hdf_file_is_closed(filename));
    }
    TS_ASSERT(hdf_file_is_closed(filename));
  }

  void test_hdf5ID_comparators() {
    cout << "\ntest hdf5ID comparator operations" << endl;

    TestHdf5ID uid1(GOOD_ID1);
    TestHdf5ID uid2(GOOD_ID2);
    TestHdf5ID uid3(INVALID_HID);

    // Test equality operators
    TS_ASSERT(uid1 == GOOD_ID1);
    TS_ASSERT(!(uid1 == GOOD_ID2));
    TS_ASSERT(uid1 != GOOD_ID2);
    TS_ASSERT(!(uid1 != GOOD_ID1));

    // Test inequality operators
    TS_ASSERT(uid3 <= INVALID_HID);
    TS_ASSERT(uid3 == INVALID_HID);
    TS_ASSERT(!(uid3 < INVALID_HID));

    // Test with different IDs
    if (GOOD_ID1 < GOOD_ID2) {
      TS_ASSERT(uid1 < GOOD_ID2);
      TS_ASSERT(uid1 <= GOOD_ID2);
    } else {
      TS_ASSERT(uid2 < GOOD_ID1);
      TS_ASSERT(uid2 <= GOOD_ID1);
    }
  }
  void test_hdf5ID_implicit_conversion() {
    cout << "\ntest hdf5ID implicit conversion to hid_t" << endl;

    TestHdf5ID uid(GOOD_ID1);
    hid_t raw_id = uid; // implicit conversion
    TS_ASSERT_EQUALS(raw_id, GOOD_ID1);

    // Can be used in HDF5 functions expecting hid_t
    TS_ASSERT(H5Iis_valid(uid) > 0);
  }

  void test_hdf5ID_zero_is_invalid() {
    cout << "\ntest hdf5ID zero is invalid" << endl;

    TestHdf5ID uid(0);
    TS_ASSERT(!uid.isValid());
    TS_ASSERT_EQUALS(uid.get(), 0);
  }

  void test_hdf5ID_negative_values() {
    cout << "\ntest hdf5ID negative values are invalid" << endl;

    TestHdf5ID uid1(-1);
    TestHdf5ID uid2(-999);

    TS_ASSERT(!uid1.isValid());
    TS_ASSERT(!uid2.isValid());
    TS_ASSERT_EQUALS(uid1.get(), -1);
    TS_ASSERT_EQUALS(uid2.get(), -999);
  }

  // ******************************************************************
  // UNIQUE ID
  // ******************************************************************

  void test_uniqueID_close_on_exit() {
    cout << "\ntest uniqueID close on exit" << endl;

    // if set with the invalid ID, no close on exit
    {
      TestUniqueID uid(INVALID_HID);
      TS_ASSERT(!uid.isValid());
    }
    // the deleter was not called on exit
    TS_ASSERT_EQUALS(call_count, 0);

    // if set with a valid ID, will close on exit
    hid_t good = GOOD_ID1;
    TS_ASSERT(H5Iis_valid(good));
    {
      TestUniqueID uid(good);
      TS_ASSERT(uid.isValid());
    }
    // the deleter was called on exit
    TS_ASSERT_EQUALS(call_count, 1);
  }

  void test_uniqueID_construct_empty() {
    cout << "\ntest uniqueID constructor empty" << endl;
    // construct empty
    {
      TestUniqueID uid;
      TS_ASSERT_EQUALS(uid.get(), INVALID_HID);
      TS_ASSERT(!uid.isValid());
    }
    // no deleter called on invalid ID
    TS_ASSERT_EQUALS(call_count, 0);
  }

  void test_uniqueID_construct() {
    cout << "\ntest uniqueID construct" << endl;
    // construct
    hid_t test = GOOD_ID1;
    {
      TestUniqueID uid(test);
      TS_ASSERT_EQUALS(uid.get(), test);
      TS_ASSERT_DIFFERS(uid.get(), INVALID_HID);
      TS_ASSERT(uid.isValid());
    }
    // deleter called on valid ID
    TS_ASSERT_EQUALS(call_count, 1);
  }

  void test_uniqueID_move_construct() {
    cout << "\ntest uniqueID move construct\n";
    // move construct
    hid_t test = GOOD_ID1;
    {
      TestUniqueID uid(test);
      {
        TestUniqueID uid2(std::move(uid));
        // move constructor invalidates uid, sets uid2 to value
        TS_ASSERT(!uid.isValid());
        TS_ASSERT_EQUALS(uid2.get(), test);
      }
      // uid2 out of scope, deleter called once, uid still invalid
      TS_ASSERT_EQUALS(uid.get(), INVALID_HID);
      TS_ASSERT(!uid.isValid());
      TS_ASSERT_EQUALS(call_count, 1);
    }
    // ensure no double-delete
    TS_ASSERT_EQUALS(call_count, 1);
  }

  void test_uniqueID_assign_hid() {
    // assign from hid_t
    hid_t val1 = GOOD_ID1, val2 = GOOD_ID2;
    {
      TestUniqueID uid(val1);
      TS_ASSERT_EQUALS(uid.get(), val1);
      TS_ASSERT(uid.isValid());
      // ASSIGN THE VALUE
      uid = val2;
      TS_ASSERT_EQUALS(uid.get(), val2);
      TS_ASSERT(uid.isValid());
      TS_ASSERT_EQUALS(call_count, 1);
    }
    TS_ASSERT_EQUALS(call_count, 2);
  }

  void test_uniqueID_assign_other() {
    cout << "\ntest uniqueID assign" << endl;
    // assign from uid
    hid_t val1 = GOOD_ID1, val2 = GOOD_ID2;
    {
      TestUniqueID uid1(val1), uid2(val2);
      TS_ASSERT_EQUALS(uid1.get(), val1);
      TS_ASSERT_EQUALS(uid2.get(), val2);
      // ASSIGN FROM MOVE
      uid1 = std::move(uid2);
      TS_ASSERT_EQUALS(uid1.get(), val2);
      TS_ASSERT_EQUALS(uid2.get(), INVALID_HID);
      TS_ASSERT_EQUALS(call_count, 1);
    }
    TS_ASSERT_EQUALS(call_count, 2);
  }

  void test_uniqueID_assign_move() {
    cout << "\ntest uniqueID move assign" << endl;

    hid_t test1 = GOOD_ID1;
    hid_t test2 = GOOD_ID2;

    TestUniqueID uid1(test1);
    TestUniqueID uid2;

    uid2 = std::move(uid1);
    TS_ASSERT_EQUALS(uid2.get(), test1);
    TS_ASSERT_EQUALS(uid1.get(), INVALID_HID);

    uid2 = test2;
    TS_ASSERT_EQUALS(uid2.get(), test2);
    TS_ASSERT_EQUALS(call_count, 1);
  }

// NOTE this test performs self-move on purpose
#if defined(__GNUC__) || defined(__clang__)
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wself-move"
#endif
  void test_uniqueID_assign_self() {
    cout << "\ntest uniqueID self assignment" << endl;

    hid_t test = GOOD_ID1;
    TestUniqueID uid(test);
    TS_ASSERT_EQUALS(uid.get(), test);

    // Self-assignment via move
    uid = std::move(uid);
    TS_ASSERT_EQUALS(uid.get(), test);
    TS_ASSERT(uid.isValid());
    // closer not called on self-move
    TS_ASSERT_EQUALS(call_count, 0);
  }
#if defined(__GNUC__) || defined(__clang__)
#pragma GCC diagnostic pop
#endif

  void test_uniqueID_release() {
    cout << "\ntest uniqueID release" << endl;
    // release
    hid_t test = GOOD_ID1;
    hid_t res;
    {
      TestUniqueID uid(test);
      TS_ASSERT_THROWS_NOTHING(res = uid.release());
      TS_ASSERT_EQUALS(uid.get(), INVALID_HID);
      TS_ASSERT(!uid.isValid());
      TS_ASSERT_EQUALS(res, test);
    }
    TS_ASSERT_EQUALS(call_count, 0);
    TS_ASSERT_EQUALS(res, test);
  }

  void test_uniqueID_reset_same() {
    cout << "\ntest uniqueID same" << endl;
    // reset
    hid_t test = GOOD_ID1;
    {
      TestUniqueID uid(test);
      TS_ASSERT(uid.isValid());
      TS_ASSERT_EQUALS(uid.get(), test);
      TS_ASSERT_THROWS_NOTHING(uid.reset(test));
      TS_ASSERT_EQUALS(uid.get(), test);
      TS_ASSERT_EQUALS(call_count, 0);
    }
    // deleter called once when uid goes out of scope
    TS_ASSERT_EQUALS(call_count, 1);
  }

  void test_uniqueID_reset_other() {
    cout << "\ntest uniqueID reset" << endl;
    // reset
    hid_t test = GOOD_ID1, other = BAD_ID;
    {
      TestUniqueID uid(test);
      TS_ASSERT(uid.isValid());
      TS_ASSERT_EQUALS(uid.get(), test);
      TS_ASSERT_THROWS_NOTHING(uid.reset(other));
      TS_ASSERT_EQUALS(uid.get(), other);
      TS_ASSERT_EQUALS(call_count, 1);
    }
    // deleter not called when invalid ID (BAD_ID) exits scope
    TS_ASSERT_EQUALS(call_count, 1);
  }

  void test_uniqueID_reset_none() {
    cout << "\ntest uniqueID none" << endl;
    // reset
    hid_t test = GOOD_ID1;
    {
      TestUniqueID uid(test);
      TS_ASSERT(uid.isValid());
      TS_ASSERT_EQUALS(uid.get(), test);
      TS_ASSERT_THROWS_NOTHING(uid.reset());
      TS_ASSERT_EQUALS(uid.get(), INVALID_HID);
      TS_ASSERT_EQUALS(call_count, 1);
    }
  }

  void test_uniqueID_reset_move() {
    cout << "\ntest uniqueID move reset" << endl;

    hid_t test1 = GOOD_ID1;
    hid_t test2 = GOOD_ID2;

    TestUniqueID uid1(test1);
    TestUniqueID uid2(test2);

    // Move reset
    uid1.reset(std::move(uid2));
    TS_ASSERT_EQUALS(uid1.get(), test2);
    TS_ASSERT_EQUALS(uid2.get(), INVALID_HID);
    TS_ASSERT(uid1.isValid());
    TS_ASSERT(!uid2.isValid());
    TS_ASSERT_EQUALS(call_count, 1); // test1 was closed
  }

  void test_uniqueID_release_then_reset() {
    cout << "\ntest uniqueID release then reset" << endl;

    hid_t test1 = GOOD_ID1;
    hid_t test2 = GOOD_ID2;
    TestUniqueID uid(test1);

    hid_t released = uid.release();
    TS_ASSERT_EQUALS(released, test1);
    TS_ASSERT_EQUALS(uid.get(), INVALID_HID);
    TS_ASSERT(!uid.isValid());

    // Reset after release
    uid.reset(test2);
    TS_ASSERT_EQUALS(uid.get(), test2);
    TS_ASSERT(uid.isValid());
    // the closer is not called on released id
    TS_ASSERT_EQUALS(call_count, 0);
  }

  void test_uniqueID_multiple_resets() {
    cout << "\ntest uniqueID multiple sequential resets" << endl;

    TestUniqueID uid(GOOD_ID1);
    TS_ASSERT_EQUALS(call_count, 0);

    uid.reset(GOOD_ID2);
    TS_ASSERT_EQUALS(call_count, 1);
    TS_ASSERT_EQUALS(uid.get(), GOOD_ID2);

    uid.reset(BAD_ID);
    TS_ASSERT_EQUALS(call_count, 2);
    TS_ASSERT_EQUALS(uid.get(), BAD_ID);

    uid.reset();
    TS_ASSERT_EQUALS(call_count, 2); // BAD_ID was invalid, no close
    TS_ASSERT_EQUALS(uid.get(), INVALID_HID);
  }

  void test_uniqueID_groups_close() {
    cout << "\ntest uniqueID groups close" << endl;

    FileResource resource("test_uniqueid_close_groups.h5");
    std::string filename(resource.fullPath());
    {
      // create a file held by a unique ID
      ParameterID fapl = H5Pcreate(H5P_FILE_ACCESS);
      H5Pset_fclose_degree(fapl, H5F_CLOSE_STRONG);
      UniqueFileID fid = H5Fcreate(filename.c_str(), H5F_ACC_TRUNC, H5P_DEFAULT, fapl);
      TS_ASSERT(!hdf_file_is_closed(filename));
      TS_ASSERT_EQUALS(H5Fget_obj_count(fid, H5F_OBJ_GROUP), 0);
      {
        // create a group within that file
        GroupID gid = H5Gcreate1(fid.get(), "a_group", 1);
        TS_ASSERT_EQUALS(H5Fget_obj_count(fid, H5F_OBJ_GROUP), 1);
        TS_ASSERT(gid.isValid());
        GroupID gid2 = H5Gopen1(fid.get(), "a_group");
        TS_ASSERT(gid2.isValid());
      }
      TS_ASSERT(!hdf_file_is_closed(filename));
      TS_ASSERT_EQUALS(H5Fget_obj_count(fid, H5F_OBJ_GROUP), 0);
    }
    TS_ASSERT(hdf_file_is_closed(filename));
  }

  void test_uniqueID_no_double_close() {
    cout << "\ntest uniqueID no double close" << endl;

    // setup the error handler to count errors
    int err_count = 0;
    auto err = [](hid_t, void *count) {
      (*static_cast<int *>(count))++;
      return 0;
    };
    H5Eset_auto2(H5E_DEFAULT, err, &err_count);

    FileResource resource("test_uniqueid_no_double.h5");
    std::string filename(resource.fullPath());
    {
      // create a file to hold a group
      UniqueFileID fid = H5Fcreate(filename.c_str(), H5F_ACC_TRUNC, H5P_DEFAULT, H5P_DEFAULT);
      TS_ASSERT_EQUALS(H5Fget_obj_count(fid, H5F_OBJ_GROUP), 0);
      // now create a group in the file with a group ID
      hid_t gid = H5Gcreate1(fid.get(), "a_group", 1);
      TS_ASSERT_EQUALS(H5Fget_obj_count(fid, H5F_OBJ_GROUP), 1);
      {
        // hold that group ID in a UniqueID
        GroupID uid(gid);
        // this is a valid id
        TS_ASSERT_EQUALS(H5Fget_obj_count(fid, H5F_OBJ_GROUP), 1);
        TS_ASSERT(H5Iis_valid(gid));
        TS_ASSERT(uid.isValid());
        // now close it by some other process
        TS_ASSERT(H5Gclose(gid) >= 0);
        TS_ASSERT_EQUALS(H5Fget_obj_count(fid, H5F_OBJ_GROUP), 0);
        // once gid closed, the unique id is no longer valid
        TS_ASSERT(!H5Iis_valid(gid));
        TS_ASSERT(!uid.isValid());
        // note, double-closing would cause an error if improperly handled
        TS_ASSERT_EQUALS(err_count, 0); // no errors so far
        TS_ASSERT(H5Gclose(gid) < 0);
        TS_ASSERT_EQUALS(err_count, 1); // now there's one
      }
      // after exit, the deleter was not called because gid is no longer valid
      TS_ASSERT_EQUALS(err_count, 1); // no further errors registered
      // close the file by some other process
      H5Fclose(fid.get());
    }
    // after exit, the deleter was not called because fid is no longer valid
    TS_ASSERT_EQUALS(err_count, 1); // no further errors registered
  }

  void test_unique_file_id() {
    cout << "\ntest the file id" << endl;

    UniqueFileID fid;
    TS_ASSERT(!fid.isValid());

    // create a file
    FileResource resource("test_nexus_unique_fid.nxs");
    std::string filename(resource.fullPath());
    ParameterID fapl = H5Pcreate(H5P_FILE_ACCESS);
    H5Pset_fclose_degree(fapl, H5F_CLOSE_STRONG);
    { // scoped file creation
      UniqueFileID fid = H5Fcreate(filename.c_str(), H5F_ACC_TRUNC, H5P_DEFAULT, fapl);
      TS_ASSERT(fid.isValid());
      TS_ASSERT(!hdf_file_is_closed(filename));
      H5Fclose(fid.release());
      H5garbage_collect();
    }
    TS_ASSERT(hdf_file_is_closed(filename));

    { // scoped fid
      UniqueFileID fid = H5Fopen(filename.c_str(), H5F_ACC_RDONLY, fapl);
      TS_ASSERT(!hdf_file_is_closed(filename));
      TS_ASSERT(fid.isValid());
    } // fid goes out of scope and deconstructor is called
    // the file is now closed
    TS_ASSERT(hdf_file_is_closed(filename));
  }

  // ******************************************************************
  // SHARED ID
  // ******************************************************************

  void test_sharedID_close_on_exit() {
    cout << "\ntest sharedID close on exit" << endl;

    // if unset, no close on exit
    {
      TestSharedID uid;
      TS_ASSERT(!uid.isValid());
      TS_ASSERT_EQUALS(uid.get(), INVALID_HID);
      TS_ASSERT_EQUALS(uid.use_count(), 0);
    }
    TS_ASSERT_EQUALS(call_count, 0);

    // if set with the invalid ID, no close on exit
    {
      TestSharedID uid(INVALID_HID);
      TS_ASSERT(!uid.isValid());
      TS_ASSERT_EQUALS(uid.get(), INVALID_HID);
      TS_ASSERT_EQUALS(uid.use_count(), 0);
    }
    // the deleter was not called on exit
    TS_ASSERT_EQUALS(call_count, 0);

    // if set with a valid ID, will close on exit
    hid_t good = GOOD_ID1;
    TS_ASSERT(H5Iis_valid(good));
    {
      TestSharedID uid(good);
      TS_ASSERT(uid.isValid());
      TS_ASSERT_EQUALS(uid.get(), good);
      TS_ASSERT_EQUALS(uid.use_count(), 1);
    }
    // the deleter was called on exit
    TS_ASSERT_EQUALS(call_count, 1);
  }

  void test_sharedID_copy_construct() {
    cout << "\ntest sharedID copy constructor from invalid" << endl;

    TestSharedID uid1(GOOD_ID1);
    TS_ASSERT_EQUALS(uid1.use_count(), 1);

    TestSharedID uid2(uid1);
    TS_ASSERT_EQUALS(uid1.use_count(), 2);
    TS_ASSERT_EQUALS(uid2.use_count(), 2);
    TS_ASSERT_EQUALS(uid1.get(), uid2.get());
    TS_ASSERT_EQUALS(uid1, uid2);
  }

  void test_sharedID_copy_construct_from_invalid() {
    cout << "\ntest sharedID copy constructor from invalid" << endl;

    TestSharedID uid1;
    TS_ASSERT_EQUALS(uid1.use_count(), 0);

    TestSharedID uid2(uid1);
    TS_ASSERT_EQUALS(uid1.use_count(), 0);
    TS_ASSERT_EQUALS(uid2.use_count(), 0);
    TS_ASSERT(!uid1.isValid());
    TS_ASSERT(!uid2.isValid());
  }

  void test_sharedID_move_construct() {
    cout << "\ntest sharedID move construct\n";
    // move construct
    hid_t test = GOOD_ID1;
    {
      TestSharedID uid1(test);
      TS_ASSERT_EQUALS(uid1.use_count(), 1);
      {
        TestSharedID uid2(std::move(uid1));
        TS_ASSERT_EQUALS(uid1.use_count(), 0);
        TS_ASSERT_EQUALS(uid2.use_count(), 1);
        TS_ASSERT_EQUALS(uid2.get(), test);
      }
      // uid2 out of scope, deleter called once
      TS_ASSERT_EQUALS(call_count, 1);
    }
    // ensure no double-delete
    TS_ASSERT_EQUALS(call_count, 1);
  }

  void test_sharedID_assign_hid() {
    // assign from hid_t
    hid_t val1 = GOOD_ID1, val2 = GOOD_ID2;
    {
      TestSharedID uid(val1);
      TS_ASSERT_EQUALS(uid.use_count(), 1);
      TS_ASSERT_EQUALS(uid.get(), val1);
      TS_ASSERT(uid.isValid());
      // ASSIGN THE VALUE
      uid = val2;
      TS_ASSERT_EQUALS(uid.use_count(), 1);
      TS_ASSERT_EQUALS(uid.get(), val2);
      TS_ASSERT(uid.isValid());
      TS_ASSERT_EQUALS(call_count, 1);
    }
    TS_ASSERT_EQUALS(call_count, 2);
  }

  void test_sharedID_assign_other() {
    cout << "\ntest sharedID copy assignment" << endl;

    TestSharedID uid1(GOOD_ID1);
    TestSharedID uid2(GOOD_ID2);

    TS_ASSERT_EQUALS(uid1.use_count(), 1);
    TS_ASSERT_EQUALS(uid2.use_count(), 1);

    // Copy assignment
    uid1 = uid2;
    TS_ASSERT_EQUALS(uid1.get(), GOOD_ID2);
    TS_ASSERT_EQUALS(uid2.get(), GOOD_ID2);
    TS_ASSERT_EQUALS(uid1.use_count(), 2);
    TS_ASSERT_EQUALS(uid2.use_count(), 2);
    TS_ASSERT_EQUALS(call_count, 1); // GOOD_ID1 closed
    TS_ASSERT(uid1 == uid2);
  }

// NOTE this test performs self-assign on purpose
#if defined(__clang__)
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wself-assign-overloaded"
#endif
  void test_sharedID_assign_self() {
    cout << "\ntest sharedID self assignment copy" << endl;

    TestSharedID uid(GOOD_ID1);
    TS_ASSERT_EQUALS(uid.use_count(), 1);

    // Self-assignment via copy
    uid = uid;
    TS_ASSERT_EQUALS(uid.get(), GOOD_ID1);
    TS_ASSERT_EQUALS(uid.use_count(), 1);
    TS_ASSERT_EQUALS(call_count, 0);
  }
#if defined(__clang__)
#pragma GCC diagnostic pop
#endif

  void test_sharedID_move_assign() {
    cout << "\ntest sharedID move assignment" << endl;

    TestSharedID uid1(GOOD_ID1);
    TestSharedID uid2(GOOD_ID2);

    TS_ASSERT_EQUALS(uid1.use_count(), 1);
    TS_ASSERT_EQUALS(uid2.use_count(), 1);

    // Move assignment
    uid1 = std::move(uid2);
    TS_ASSERT_EQUALS(uid1.get(), GOOD_ID2);
    TS_ASSERT_EQUALS(uid2.get(), INVALID_HID);
    TS_ASSERT_EQUALS(uid1.use_count(), 1);
    TS_ASSERT_EQUALS(uid2.use_count(), 0);
    TS_ASSERT_EQUALS(call_count, 1); // GOOD_ID1 closed
    TS_ASSERT(!(uid1 == uid2));
  }

// NOTE this test performs self-move on purpose
#if defined(__GNUC__) || defined(__clang__)
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wself-move"
#endif
  void test_sharedID_move_assign_self() {
    cout << "\ntest sharedID self assignment move" << endl;

    TestSharedID uid(GOOD_ID1);
    TS_ASSERT_EQUALS(uid.use_count(), 1);

    // Self-assignment via move
    uid = std::move(uid);
    TS_ASSERT_EQUALS(uid.get(), GOOD_ID1);
    TS_ASSERT_EQUALS(uid.use_count(), 1);
    TS_ASSERT_EQUALS(call_count, 0);
  }
#if defined(__GNUC__) || defined(__clang__)
#pragma GCC diagnostic pop
#endif

  void test_sharedID_increment_and_decrement() {
    cout << "\ntest sharedID increment and decrement" << endl;

    TestSharedID uid(GOOD_ID1);
    std::size_t counts = uid.use_count();
    constexpr std::size_t N = 10;
    {
      TestSharedID uids[N];
      // up
      for (std::size_t i = 0; i < N; i++) {
        uids[i].reset(uid);
        counts++;
        TS_ASSERT_EQUALS(uid.use_count(), counts);
      }

      // down
      for (std::size_t i = 0; i < N; i++) {
        uids[i].reset();
        counts--;
        TS_ASSERT_EQUALS(uid.use_count(), counts);
      }
    }
    TS_ASSERT_EQUALS(uid.use_count(), 1);
  }

  void test_sharedID_no_close_when_shared() {
    cout << "\ntest sharedID no close when shared" << endl;

    {
      TestSharedID uid1;
      TS_ASSERT_EQUALS(uid1.use_count(), 0);
      {
        TestSharedID uid2(GOOD_ID1);
        TS_ASSERT(uid2.isValid());
        TS_ASSERT_EQUALS(uid2.use_count(), 1);
        uid1 = uid2;
        TS_ASSERT(uid1.isValid());
        TS_ASSERT_EQUALS(uid1.use_count(), 2);
        TS_ASSERT_EQUALS(uid2.use_count(), 2);
        TS_ASSERT_EQUALS(uid1.get(), GOOD_ID1);
        TS_ASSERT_EQUALS(uid2.get(), GOOD_ID1);
        // close uid2; uid2 is invaldiated, but uid1 is unchanged
        uid2.reset();
        TS_ASSERT_EQUALS(uid2.use_count(), 0);
        TS_ASSERT_EQUALS(uid1.use_count(), 1);
        TS_ASSERT(!uid2.isValid());
        TS_ASSERT(uid1.isValid());
        TS_ASSERT_EQUALS(uid1.get(), GOOD_ID1);
        // deleter was not called on reset
        TS_ASSERT_EQUALS(call_count, 0);
      }
      // the deleter was not called on exit
      TS_ASSERT_EQUALS(call_count, 0);
      // uid1 is still valid
      TS_ASSERT_EQUALS(uid1.use_count(), 1);
      TS_ASSERT(uid1.isValid());
      TS_ASSERT_EQUALS(uid1.use_count(), 1);
      TS_ASSERT_EQUALS(uid1.get(), GOOD_ID1);
      {
        // make another ID
        TestSharedID uid3(uid1);
        TS_ASSERT(uid3.isValid());
        TS_ASSERT_EQUALS(uid1.use_count(), 2);
        TS_ASSERT_EQUALS(uid3.use_count(), 2);
        TS_ASSERT_EQUALS(uid1.get(), GOOD_ID1);
        TS_ASSERT_EQUALS(uid3.get(), GOOD_ID1);
        // close uid1
        uid1.reset();
        TS_ASSERT_EQUALS(uid1.use_count(), 0);
        TS_ASSERT_EQUALS(uid3.use_count(), 1);
        TS_ASSERT(!uid1.isValid());
        TS_ASSERT(uid3.isValid());
        TS_ASSERT_EQUALS(uid3.get(), GOOD_ID1);
        // deleter was not called on reset
        TS_ASSERT_EQUALS(call_count, 0);
      }
      // deleter called on exit (uid3)
      TS_ASSERT_EQUALS(call_count, 1);
    }
    // the deleter was not called on exit (uid1 already invalid)
    TS_ASSERT_EQUALS(call_count, 1); // counts still 1
  }

  void test_sharedID_no_segfaults() {
    cout << "\ntest sharedID no segfaults" << endl;
    // Make sure calling the public API methods never generates a segfault.
    // If handled improperly, there could be a nullptr dereference.
    TestSharedID uid; // default init, leash counts is nullptr
    // make sure none of these dereference the nullptr
    TS_ASSERT_EQUALS(uid.get(), INVALID_HID);
    TS_ASSERT(!uid.isValid());
    TS_ASSERT_EQUALS(uid.use_count(), 0);
    TS_ASSERT_THROWS_NOTHING(uid.reset());
    // set to a valid value, try again
    uid = GOOD_ID1;
    TS_ASSERT_EQUALS(uid.get(), GOOD_ID1);
    TS_ASSERT(uid.isValid());
    TS_ASSERT_EQUALS(uid.use_count(), 1);
    // set to an invalid id, try again
    TS_ASSERT_THROWS_NOTHING(uid.reset(BAD_ID));
    TS_ASSERT_EQUALS(uid.get(), BAD_ID);
    TS_ASSERT(!uid.isValid());
    TS_ASSERT_EQUALS(uid.use_count(), 0);
    // close the ID and then check
    TS_ASSERT_THROWS_NOTHING(uid.reset());
    TS_ASSERT_EQUALS(uid.get(), INVALID_HID);
    TS_ASSERT(!uid.isValid());
    TS_ASSERT_EQUALS(uid.use_count(), 0);
  }

  void test_sharedID_reset_close_no_segfault() {
    cout << "\ntest sharedID no segfault when reset" << endl;
    /** if handled improperly, then attempts to reset an invalidated ID can
     * result in a segmentation fault, from dereferencing a null leash counter.
     */
    {
      TestSharedID uid1(GOOD_ID1);
      TS_ASSERT_EQUALS(uid1.use_count(), 1);
      {
        TestSharedID uid3(uid1);
        uid1.reset();
        // deleter not called on reset
        TS_ASSERT_EQUALS(call_count, 0);
      }
      // deleter called on exit (uid3)
      TS_ASSERT_EQUALS(call_count, 1);
      TS_ASSERT(!uid1.isValid());
      uid1.reset(); // should be a no-op
    }
    // the deleter was not called on exit (uid1 already invalid)
    TS_ASSERT_EQUALS(call_count, 1);
  }

  void test_sharedID_reset_copy() {
    cout << "\ntest sharedID reset with copy" << endl;

    TestSharedID uid1(GOOD_ID1);
    TestSharedID uid2;

    TS_ASSERT_EQUALS(uid1.use_count(), 1);
    TS_ASSERT_EQUALS(uid2.use_count(), 0);

    uid2.reset(uid1);
    TS_ASSERT_EQUALS(uid1.use_count(), 2);
    TS_ASSERT_EQUALS(uid2.use_count(), 2);
    TS_ASSERT_EQUALS(uid1.get(), GOOD_ID1);
    TS_ASSERT_EQUALS(uid2.get(), GOOD_ID1);
    TS_ASSERT(uid1 == uid2);
  }

  void test_sharedID_reset_move() {
    cout << "\ntest sharedID reset with move" << endl;

    TestSharedID uid1(GOOD_ID1);
    TestSharedID uid2;

    TS_ASSERT_EQUALS(uid1.use_count(), 1);
    TS_ASSERT_EQUALS(uid2.use_count(), 0);

    uid2.reset(std::move(uid1));
    TS_ASSERT_EQUALS(uid1.use_count(), 0);
    TS_ASSERT_EQUALS(uid2.use_count(), 1);
    TS_ASSERT_EQUALS(uid1.get(), INVALID_HID);
    TS_ASSERT_EQUALS(uid2.get(), GOOD_ID1);
  }

  void test_sharedID_reset_with_same_hid() {
    cout << "\ntest sharedID reset with same hid_t value" << endl;

    TestSharedID uid(GOOD_ID1);
    TS_ASSERT_EQUALS(uid.use_count(), 1);
    TS_ASSERT_EQUALS(call_count, 0);

    // Reset with same value should not close
    uid.reset(GOOD_ID1);
    TS_ASSERT_EQUALS(uid.get(), GOOD_ID1);
    TS_ASSERT_EQUALS(uid.use_count(), 1);
    TS_ASSERT_EQUALS(call_count, 0);
  }

  void test_sharedID_equality_operators() {
    cout << "\ntest sharedID equality between SharedIDs" << endl;

    TestSharedID uid1(GOOD_ID1);
    TestSharedID uid2(uid1);
    TestSharedID uid3(GOOD_ID1); // Same value, different tracking
    TestSharedID uid4;

    // Same tracking
    TS_ASSERT(uid1 == uid2);
    TS_ASSERT(uid2 == uid1);

    // Different tracking even though same hid_t value
    TS_ASSERT(!(uid1 == uid3));
    TS_ASSERT(!(uid3 == uid1));

    // Invalid IDs
    TS_ASSERT(!(uid1 == uid4));
    TS_ASSERT(!(uid4 == uid1));
  }

  void test_sharedID_close_vector_on_exit() {
    cout << "\ntest sharedID in vector operations" << endl;

    TestSharedID original(GOOD_ID1);
    std::size_t counts = original.use_count();
    constexpr std::size_t N = 10;
    {
      std::vector<TestSharedID> vec;
      for (std::size_t i = 0; i < N; ++i) {
        vec.push_back(original);
        counts++;
        TS_ASSERT_EQUALS(original.use_count(), counts);
      }
    }
    TS_ASSERT_EQUALS(original.use_count(), 1);
    TS_ASSERT_EQUALS(call_count, 0);
  }

  void test_sharedID_close_all_on_exit() {
    cout << "\ntest sharedID all array elements closed on exit" << endl;

    TestSharedID uid(GOOD_ID1);
    std::size_t counts = uid.use_count();
    constexpr std::size_t N = 10;
    TestSharedID *puid; // this used to delete the array later
    {
      TestSharedID *uids = new TestSharedID[N];
      for (std::size_t i = 0; i < N; i++) {
        uids[i].reset(uid);
        counts++;
        TS_ASSERT_EQUALS(uid.use_count(), counts);
      }
      puid = uids; // save the pointer, to deleter it
    }
    // exiting scope does not clear the allocated array
    TS_ASSERT_EQUALS(uid.use_count(), counts);
    delete[] puid;
    // deleting the allocated array clears those counts
    TS_ASSERT_EQUALS(uid.use_count(), 1);
  }

  void test_sharedID_thrice() {
    cout << "\ntest a sharedId, thrice" << std::endl;

    { // scoped fid
      TestSharedID id1 = GOOD_ID1;
      TestSharedID id2(id1);
      TestSharedID id3(id2);
      TS_ASSERT(id1.isValid());
      TS_ASSERT(id2.isValid());
      TS_ASSERT(id3.isValid());
      TS_ASSERT_EQUALS(id1.use_count(), 3);
      TS_ASSERT_EQUALS(id2.use_count(), 3);
      TS_ASSERT_EQUALS(id3.use_count(), 3);
      TS_ASSERT_EQUALS(id2.get(), id1.get());
      TS_ASSERT_EQUALS(id3.get(), id1.get());
      TS_ASSERT(id1 == id2);
      TS_ASSERT(id1 == id3);
      TS_ASSERT(id2 == id3);
      // close fid1
      id1.reset();
      TS_ASSERT_EQUALS(call_count, 0);
      TS_ASSERT(!id1.isValid());
      TS_ASSERT(id2.isValid());
      TS_ASSERT(id3.isValid());
      TS_ASSERT_EQUALS(id1.use_count(), 0);
      TS_ASSERT_EQUALS(id2.use_count(), 2);
      TS_ASSERT_EQUALS(id3.use_count(), 2);
      TS_ASSERT_EQUALS(id2.get(), id3.get());
      TS_ASSERT(!(id1 == id2));
      TS_ASSERT(!(id1 == id3));
      TS_ASSERT(id2 == id3);
      // close fid3
      id3.reset();
      TS_ASSERT_EQUALS(call_count, 0);
      TS_ASSERT(!id1.isValid());
      TS_ASSERT(id2.isValid());
      TS_ASSERT(!id3.isValid());
      TS_ASSERT_EQUALS(id1.use_count(), 0);
      TS_ASSERT_EQUALS(id2.use_count(), 1);
      TS_ASSERT_EQUALS(id3.use_count(), 0);
      TS_ASSERT_DIFFERS(id2.get(), id3.get());
      TS_ASSERT(!(id1 == id2));
      TS_ASSERT(!(id2 == id3));
    } // last id goes out of scope and deconstructor is called
    // the destructor is now called
    TS_ASSERT_EQUALS(call_count, 1);
  }

  void test_sharedID_circular_sharing() {
    cout << "\ntest sharedID circular sharing pattern" << endl;

    TestSharedID uid1(GOOD_ID1);
    TestSharedID uid2(uid1);
    TestSharedID uid3(uid2);

    TS_ASSERT_EQUALS(uid1.use_count(), 3);
    TS_ASSERT_EQUALS(uid2.use_count(), 3);
    TS_ASSERT_EQUALS(uid3.use_count(), 3);

    // Now reassign in a circle
    uid1 = uid3;
    TS_ASSERT_EQUALS(uid1.use_count(), 3);
    TS_ASSERT_EQUALS(uid2.use_count(), 3);
    TS_ASSERT_EQUALS(uid3.use_count(), 3);
    TS_ASSERT_EQUALS(call_count, 0);
  }

  void test_sharedID_assign_invalid_to_valid() {
    cout << "\ntest sharedID assign invalid to valid" << endl;

    TestSharedID uid1(GOOD_ID1);
    TestSharedID uid2;

    TS_ASSERT_EQUALS(uid1.use_count(), 1);
    TS_ASSERT_EQUALS(uid2.use_count(), 0);
    TS_ASSERT_EQUALS(call_count, 0);

    uid1 = uid2; // Assign invalid to valid
    TS_ASSERT_EQUALS(uid1.use_count(), 0);
    TS_ASSERT_EQUALS(uid2.use_count(), 0);
    TS_ASSERT(!uid1.isValid());
    TS_ASSERT(!uid2.isValid());
    TS_ASSERT_EQUALS(call_count, 1); // GOOD_ID1 was closed
  }

  void test_sharedID_multiple_resets() {
    cout << "\ntest sharedID multiple sequential resets" << endl;

    TestSharedID uid(GOOD_ID1);
    TS_ASSERT_EQUALS(uid.use_count(), 1);
    TS_ASSERT_EQUALS(call_count, 0);

    uid.reset(GOOD_ID2);
    TS_ASSERT_EQUALS(uid.use_count(), 1);
    TS_ASSERT_EQUALS(uid.get(), GOOD_ID2);
    TS_ASSERT_EQUALS(call_count, 1); // GOOD_ID1 closed

    uid.reset(BAD_ID);
    TS_ASSERT_EQUALS(uid.use_count(), 0);
    TS_ASSERT_EQUALS(uid.get(), BAD_ID);
    TS_ASSERT_EQUALS(call_count, 2); // GOOD_ID2 closed

    uid.reset();
    TS_ASSERT_EQUALS(uid.use_count(), 0);
    TS_ASSERT_EQUALS(uid.get(), INVALID_HID);
    TS_ASSERT_EQUALS(call_count, 2); // BAD_ID was invalid, no close
  }

  void test_sharedID_close_on_pointer_delete() {
    cout << "\ntest sharedID complex lifetime management" << endl;

    TestSharedID *uid1 = new TestSharedID(GOOD_ID1);
    TS_ASSERT_EQUALS(uid1->use_count(), 1);

    TestSharedID *uid2 = new TestSharedID(*uid1);
    TS_ASSERT_EQUALS(uid1->use_count(), 2);
    TS_ASSERT_EQUALS(uid2->use_count(), 2);

    TestSharedID *uid3 = new TestSharedID(*uid2);
    TS_ASSERT_EQUALS(uid1->use_count(), 3);
    TS_ASSERT_EQUALS(uid2->use_count(), 3);
    TS_ASSERT_EQUALS(uid3->use_count(), 3);

    delete uid1;
    TS_ASSERT_EQUALS(uid2->use_count(), 2);
    TS_ASSERT_EQUALS(uid3->use_count(), 2);
    TS_ASSERT_EQUALS(call_count, 0);

    delete uid3;
    TS_ASSERT_EQUALS(uid2->use_count(), 1);
    TS_ASSERT_EQUALS(call_count, 0);

    delete uid2;
    TS_ASSERT_EQUALS(call_count, 1); // Finally closed
  }

  void test_sharedID_swap_pattern() {
    cout << "\ntest sharedID swap-like pattern" << endl;

    TestSharedID uid1(GOOD_ID1);
    TestSharedID uid2(GOOD_ID2);

    TS_ASSERT_EQUALS(uid1.use_count(), 1);
    TS_ASSERT_EQUALS(uid2.use_count(), 1);

    // Swap using temporaries
    {
      TestSharedID temp(std::move(uid1));
      uid1 = std::move(uid2);
      uid2 = std::move(temp);
    }

    TS_ASSERT_EQUALS(uid1.get(), GOOD_ID2);
    TS_ASSERT_EQUALS(uid2.get(), GOOD_ID1);
    TS_ASSERT_EQUALS(uid1.use_count(), 1);
    TS_ASSERT_EQUALS(uid2.use_count(), 1);
    TS_ASSERT_EQUALS(call_count, 0);
  }

  void test_sharedID_zero_use_count_edge_cases() {
    cout << "\ntest sharedID zero use_count edge cases" << endl;

    TestSharedID uid1;
    TS_ASSERT_EQUALS(uid1.use_count(), 0);

    // Operations on zero-count ID
    uid1.reset();
    TS_ASSERT_EQUALS(uid1.use_count(), 0);

    TestSharedID uid2(uid1);
    TS_ASSERT_EQUALS(uid1.use_count(), 0);
    TS_ASSERT_EQUALS(uid2.use_count(), 0);

    TestSharedID uid3;
    uid3 = uid1;
    TS_ASSERT_EQUALS(uid3.use_count(), 0);
  }

  void test_sharedID_files() {
    cout << "\ntest the shared file id" << std::endl;

    // create a file
    FileResource resource("test_nexus_fid.nxs");
    std::string filename(resource.fullPath());
    ParameterID fapl = H5Pcreate(H5P_FILE_ACCESS);
    H5Pset_fclose_degree(fapl, H5F_CLOSE_STRONG);
    // create and then close the file
    {
      UniqueFileID fid_temp = H5Fcreate(filename.c_str(), H5F_ACC_TRUNC, H5P_DEFAULT, fapl);
      TS_ASSERT(!hdf_file_is_closed(filename));
    }
    TS_ASSERT(hdf_file_is_closed(filename));
    // now check sharing a file id
    { // scoped fid
      SharedFileID fid1 = H5Fopen(filename.c_str(), H5F_ACC_RDONLY | H5F_ACC_SWMR_READ, fapl);
      SharedFileID fid2(fid1);
      SharedFileID fid3(fid2);
      TS_ASSERT_EQUALS(fid1.use_count(), 3);
      TS_ASSERT_EQUALS(fid2.use_count(), 3);
      TS_ASSERT_EQUALS(fid3.use_count(), 3);
      TS_ASSERT_EQUALS(fid2.get(), fid1.get());
      TS_ASSERT_EQUALS(fid3.get(), fid1.get());
      TS_ASSERT(fid1 == fid2);
      TS_ASSERT(fid2 == fid3);
      TS_ASSERT(fid3 == fid1);
      TS_ASSERT(!hdf_file_is_closed(filename));
      // close fid1
      fid1.reset();
      TS_ASSERT(!hdf_file_is_closed(filename));
      TS_ASSERT(fid2.isValid());
      TS_ASSERT(fid3.isValid());
      TS_ASSERT(!(fid1 == fid2));
      TS_ASSERT(!(fid1 == fid3));
      TS_ASSERT(fid2 == fid3);
      TS_ASSERT_EQUALS(fid3.use_count(), 2);
      // close fid3
      fid3.reset();
      TS_ASSERT(!hdf_file_is_closed(filename));
      TS_ASSERT(fid2.isValid());
    } // last fid goes out of scope and destructor is called
    // the file is now closed
    TS_ASSERT(hdf_file_is_closed(filename));
  }

  void test_sharedId_thread_safety() {
// NOTE this warning causes build failures; suppress for now
// the issue is caused by capturing variables of type TestSharedID,
// which have a deleter method defined at global scope
#if defined(__GNUC__) && !(defined(__clang__))
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wsubobject-linkage"
#endif
    constexpr int N{10};
    TestSharedID id(GOOD_ID1);
    std::vector<TestSharedID> ids(N);
    std::vector<std::thread> threads;

    // create some ids and increment count
    std::function<void(int)> make_another = [&ids, &id](int i) {
      ids[i] = id;
      int dart = (53 * i + 122) % 9;
      std::this_thread::sleep_for(std::chrono::milliseconds(dart));
      TS_ASSERT_EQUALS(ids[i].get(), id.get());
      dart = (53 * dart + 122) % 9;
      std::this_thread::sleep_for(std::chrono::milliseconds(dart));
    };
    for (int i = 0; i < N; i++) {
      threads.emplace_back(make_another, i);
    }
    for (int i = 0; i < N; i++) {
      threads[i].join();
    }
    // now check
    TS_ASSERT_EQUALS(id.use_count(), N + 1);

    // delete some ids and verify counts
    threads.clear();
    std::function<void(int)> remove_more = [&ids](int i) {
      int dart = (27 * i + 122) % 7;
      std::this_thread::sleep_for(std::chrono::milliseconds(dart));
      ids[i].reset();
    };
    for (int i = 0; i < N; ++i) {
      threads.emplace_back(remove_more, i);
    }
    for (int i = 0; i < N; i++) {
      threads[i].join();
    }
    // now check
    TS_ASSERT_EQUALS(id.use_count(), 1);
#if defined(__GNUC__) && !(defined(__clang__))
#pragma GCC diagnostic pop
#endif
  }
};
