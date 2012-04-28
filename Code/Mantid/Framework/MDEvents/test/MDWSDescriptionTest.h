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
      TS_ASSERT_EQUALS("[-Q3,0.99Q3,-Q3]",name);

      TS_ASSERT_THROWS_NOTHING(name=makeAxisName(V3D(-1,0.9999,-1.001),Names));
      TS_ASSERT_EQUALS("[-Q3,Q3,-Q3]",name);

      TS_ASSERT_THROWS_NOTHING(name=makeAxisName(V3D(-1,0.999,-1.01),Names));
      TS_ASSERT_EQUALS("[-Q3,0.999Q3,-1.01Q3]",name);

      TS_ASSERT_THROWS_NOTHING(name=makeAxisName(V3D(2.01,0.9,-1.01),Names));
      TS_ASSERT_EQUALS("[2.01Q1,0.9Q1,-1.01Q1]",name);

      TS_ASSERT_THROWS_NOTHING(name=makeAxisName(V3D(0.2,0.9,-1),Names));
      TS_ASSERT_EQUALS("[0.2Q3,0.9Q3,-Q3]",name);


  }

MDWSDescriptionTest():descr(5)
{
}

};


#endif
