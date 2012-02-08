#ifndef MANTID_MDWS_DESCRIPTION_H_
#define MANTID_MDWS_DESCRIPTION_H_

#include <cxxtest/TestSuite.h>
#include "MantidMDEvents/MDWSDescription.h"


using namespace Mantid::MDEvents;
using namespace Mantid::Kernel;

class MDWSDescriptionTest : public CxxTest::TestSuite
{
    MDWSDescription descr;
public:
  // This pair of boilerplate methods prevent the suite being created statically
  // This means the constructor isn't called when running other tests
  static MDWSDescriptionTest *createSuite() { return new MDWSDescriptionTest(); }
  static void destroySuite( MDWSDescriptionTest *suite ) { delete suite; }

  void test_name()
  {
      V3D dir1(1,0,0);
      std::string name;

      TS_ASSERT_THROWS_NOTHING(name=makeAxisName(dir1,descr.defailt_qNames));
      TS_ASSERT_EQUALS("[Qh,0,0]",name);

      TS_ASSERT_THROWS_NOTHING(name=makeAxisName(V3D(-1,0.99,-1.001),descr.defailt_qNames));
      TS_ASSERT_EQUALS("[-Qh,0.99Qk,-Ql]",name);

      TS_ASSERT_THROWS_NOTHING(name=makeAxisName(V3D(-1,0.9999,-1.001),descr.defailt_qNames));
      TS_ASSERT_EQUALS("[-Qh,Qk,-Ql]",name);

      TS_ASSERT_THROWS_NOTHING(name=makeAxisName(V3D(-1,0.999,-1.01),descr.defailt_qNames));
      TS_ASSERT_EQUALS("[-Qh,0.999Qk,-1.01Ql]",name);

      TS_ASSERT_THROWS_NOTHING(name=makeAxisName(V3D(2.01,0.9,-1.01),descr.defailt_qNames));
      TS_ASSERT_EQUALS("[2.01Qh,0.9Qk,-1.01Ql]",name);



  }

MDWSDescriptionTest():descr(5)
{
}

};


#endif