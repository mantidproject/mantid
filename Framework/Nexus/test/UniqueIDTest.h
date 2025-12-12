// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2025 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include <cxxtest/TestSuite.h>

#include "MantidNexus/NexusException.h"
#include "MantidNexus/NexusFile.h"
#include "MantidTypes/Core/DateAndTime.h"
#include "test_helper.h"
#include <H5Cpp.h>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <map>
#include <regex>
#include <sstream>
#include <string>
#include <vector>

using namespace NexusTest;
using std::cout;
using std::endl;
using std::map;
using std::multimap;
using std::string;
using std::vector;

namespace {
// an ID which will always return as valid
const hid_t GOOD_ID = H5T_NATIVE_INT;
static int call_count = 0;
int blank_deleter(hid_t) { return ++call_count; }
} // namespace

using TestUniqueID = Mantid::Nexus::UniqueID<blank_deleter>;

class UniqueIDTest : public CxxTest::TestSuite {

public:
  // // This pair of boilerplate methods prevent the suite being created statically
  // // This means the constructor isn't called when running other tests
  static UniqueIDTest *createSuite() { return new UniqueIDTest(); }
  static void destroySuite(UniqueIDTest *suite) { delete suite; }

  void test_uniqueId_isValid() {
    cout << "\ntest uniqueID isValid" << endl;
    call_count = 0;

    // if set with the invalid ID, will be invalid
    {
      TestUniqueID uid(TestUniqueID::INVALID_ID);
      TS_ASSERT(!uid.isValid());
    }
    // the deleter was not called on exit
    TS_ASSERT_EQUALS(call_count, 0);

    // if set with a positive integer that is not actually an hdf5 object, will be invalid
    hid_t test = 101;
    TS_ASSERT(!H5Iis_valid(test));
    {
      TestUniqueID uid(test);
      TS_ASSERT(!uid.isValid());
    }
    // the deleter was not called on exit
    TS_ASSERT_EQUALS(call_count, 0);

    // if set with a valid identifier, returns valid
    hid_t good = GOOD_ID;
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
    call_count = 0;
    {
      TestUniqueID uid;
      TS_ASSERT_EQUALS(uid.get(), TestUniqueID::INVALID_ID);
      TS_ASSERT(!uid.isValid());
    }
    // no deleter called on invalid ID
    TS_ASSERT_EQUALS(call_count, 0);
  }

  void test_uniqueID_construct() {
    cout << "\ntest uniqueID construct" << endl;
    // construct
    call_count = 0;
    hid_t test = GOOD_ID;
    {
      TestUniqueID uid(test);
      TS_ASSERT_EQUALS(uid.get(), test);
      TS_ASSERT_DIFFERS(uid.get(), TestUniqueID::INVALID_ID);
      TS_ASSERT(uid.isValid());
    }
    // deleter called on valid ID
    TS_ASSERT_EQUALS(call_count, 1);
  }

  void test_uniqueID_move_construct() {
    cout << "\ntest uniqueID move construct\n";
    // move construct
    call_count = 0;
    hid_t test = GOOD_ID;
    {
      TestUniqueID uid(test);
      {
        TestUniqueID uid2(std::move(uid));
        // move constructor invalidates uid, sets uid2 to value
        TS_ASSERT(!uid.isValid());
        TS_ASSERT_EQUALS(uid2.get(), test);
      }
      // uid2 out of scope, deleter called once, uid still invalid
      TS_ASSERT_EQUALS(uid.get(), TestUniqueID::INVALID_ID);
      TS_ASSERT(!uid.isValid());
      TS_ASSERT_EQUALS(call_count, 1);
    }
    // ensure no double-delete
    TS_ASSERT_EQUALS(call_count, 1);
  }

  void test_uniqueID_assign_hid() {
    // assign from hid_t
    call_count = 0;
    hid_t val1 = GOOD_ID, val2 = H5T_NATIVE_CHAR;
    {
      TestUniqueID uid(val1);
      TS_ASSERT_EQUALS(uid.get(), val1);
      TS_ASSERT(uid.isValid());
      uid = val2;
      TS_ASSERT_EQUALS(uid.get(), val2);
      TS_ASSERT(uid.isValid());
      TS_ASSERT_EQUALS(call_count, 1);
    }
    TS_ASSERT_EQUALS(call_count, 2);
  }

  void test_uniqueID_assign_uniqueID() {
    cout << "\ntest uniqueID assign" << endl;
    // assign from uid
    call_count = 0;
    hid_t val1 = GOOD_ID, val2 = H5T_NATIVE_CHAR;
    {
      Mantid::Nexus::UniqueID<blank_deleter> uid1(val1), uid2(val2);
      TS_ASSERT_EQUALS(uid1.get(), val1);
      TS_ASSERT_EQUALS(uid2.get(), val2);
      uid1 = std::move(uid2);
      TS_ASSERT_EQUALS(uid1.get(), val2);
      TS_ASSERT_EQUALS(uid2.get(), TestUniqueID::INVALID_ID);
      TS_ASSERT_EQUALS(call_count, 1);
    }
    TS_ASSERT_EQUALS(call_count, 2);
  }

  void test_uniqueID_release() {
    cout << "\ntest uniqueID release" << endl;
    // release
    call_count = 0;
    hid_t test = GOOD_ID;
    hid_t res;
    {
      TestUniqueID uid(test);
      TS_ASSERT_THROWS_NOTHING(res = uid.release());
      TS_ASSERT_EQUALS(uid.get(), TestUniqueID::INVALID_ID);
      TS_ASSERT(!uid.isValid());
      TS_ASSERT_EQUALS(res, test);
    }
    TS_ASSERT_EQUALS(call_count, 0);
    TS_ASSERT_EQUALS(res, test);
  }

  void test_uniqueID_reset_same() {
    cout << "\ntest uniqueID same" << endl;
    // reset
    call_count = 0;
    hid_t test = GOOD_ID;
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
    call_count = 0;
    hid_t test = GOOD_ID, other = 101;
    {
      TestUniqueID uid(test);
      TS_ASSERT(uid.isValid());
      TS_ASSERT_EQUALS(uid.get(), test);
      TS_ASSERT_THROWS_NOTHING(uid.reset(other));
      TS_ASSERT_EQUALS(uid.get(), other);
      TS_ASSERT_EQUALS(call_count, 1);
    }
    // deleter not called when invalid ID (101) exits scope
    TS_ASSERT_EQUALS(call_count, 1);
  }

  void test_uniqueID_reset_none() {
    cout << "\ntest uniqueID none" << endl;
    // reset
    call_count = 0;
    hid_t test = GOOD_ID;
    {
      TestUniqueID uid(test);
      TS_ASSERT(uid.isValid());
      TS_ASSERT_EQUALS(uid.get(), test);
      TS_ASSERT_THROWS_NOTHING(uid.reset());
      TS_ASSERT_EQUALS(uid.get(), TestUniqueID::INVALID_ID);
      TS_ASSERT_EQUALS(call_count, 1);
    }
  }

  void test_uniqueID_groups_close() {
    cout << "\ntest uniqueID groups close" << endl;

    std::string filename{"test_uniqueid.h5"};
    {
      // create a file held by a unique ID
      Mantid::Nexus::UniqueID<&H5Fclose> fid = H5Fcreate(filename.c_str(), H5F_ACC_TRUNC, H5P_DEFAULT, H5P_DEFAULT);
      TS_ASSERT_EQUALS(H5Fget_obj_count(fid, H5F_OBJ_GROUP), 0);
      {
        // create a group within that file
        Mantid::Nexus::UniqueID<&H5Gclose> gid = H5Gcreate1(fid.get(), "a_group", 1);
        TS_ASSERT_EQUALS(H5Fget_obj_count(fid, H5F_OBJ_GROUP), 1);
        TS_ASSERT(gid.isValid());
        Mantid::Nexus::UniqueID<&H5Gclose> gid2 = H5Gopen1(fid.get(), "a_group");
        TS_ASSERT(gid2.isValid());
      }
      TS_ASSERT_EQUALS(H5Fget_obj_count(fid, H5F_OBJ_GROUP), 0);
    }
    // cleanup
    std::filesystem::remove(filename);
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
    std::string filename{"test_uniqueid.h5"};
    {
      // create a file to hold a group
      Mantid::Nexus::UniqueID<&H5Fclose> fid = H5Fcreate(filename.c_str(), H5F_ACC_TRUNC, H5P_DEFAULT, H5P_DEFAULT);
      TS_ASSERT_EQUALS(H5Fget_obj_count(fid, H5F_OBJ_GROUP), 0);
      // now create a group in the file with a group ID
      hid_t gid = H5Gcreate1(fid.get(), "a_group", 1);
      TS_ASSERT_EQUALS(H5Fget_obj_count(fid, H5F_OBJ_GROUP), 1);
      {
        // hold that group ID in a UniqueID
        Mantid::Nexus::UniqueID<&H5Gclose> uid(gid);
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
      // after exit, the deleter was not called because fid is no longer valid
      TS_ASSERT_EQUALS(err_count, 1); // no further errors registered
      // close the file by some other process
      H5Fclose(fid.get());
    }
    // after exit, the deleter was not called because fid is no longer valid
    TS_ASSERT_EQUALS(err_count, 1); // no further errors registered
    // cleanup
    std::filesystem::remove(filename);
  }

  void test_uniqueID_zero() {
    TestUniqueID uid(0);
    TS_ASSERT(!uid.isValid());
  }
};
