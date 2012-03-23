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

      std::vector<std::string> Names(3,"");
      Names[0]="Q1";
      Names[1]="Q2";
      Names[2]="Q3";

      TS_ASSERT_THROWS_NOTHING(name=makeAxisName(dir1,Names));
      TS_ASSERT_EQUALS("[Q1,0,0]",name);

      TS_ASSERT_THROWS_NOTHING(name=makeAxisName(V3D(-1,0.99,-1.001),Names));
      TS_ASSERT_EQUALS("[-1011Q1,1001Q2,-1012Q3]",name);

      TS_ASSERT_THROWS_NOTHING(name=makeAxisName(V3D(-1,0.9999,-1.001),Names));
      TS_ASSERT_EQUALS("[-1001Q1,1001Q2,-1002Q3]",name);

      TS_ASSERT_THROWS_NOTHING(name=makeAxisName(V3D(-1,0.999,-1.01),Names));
      TS_ASSERT_EQUALS("[-1011Q1,1010Q2,-1021Q3]",name);

      TS_ASSERT_THROWS_NOTHING(name=makeAxisName(V3D(2.01,0.9,-1.01),Names));
      TS_ASSERT_EQUALS("[4445Q1,1990Q2,-2233Q3]",name);

      TS_ASSERT_THROWS_NOTHING(name=makeAxisName(V3D(0.2,0.9,-1),Names));
      TS_ASSERT_EQUALS("[1111Q1,5000Q2,-5556Q3]",name);


  }

MDWSDescriptionTest():descr(5)
{
}

};


#endif