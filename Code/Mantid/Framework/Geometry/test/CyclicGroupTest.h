#ifndef MANTID_GEOMETRY_CYCLICGROUPTEST_H_
#define MANTID_GEOMETRY_CYCLICGROUPTEST_H_

#include <cxxtest/TestSuite.h>

#include "MantidGeometry/Crystal/CyclicGroup.h"
#include "MantidGeometry/Crystal/SymmetryOperationFactory.h"

#include <boost/make_shared.hpp>

using namespace Mantid::Geometry;
using namespace Mantid::Kernel;

class CyclicGroupTest : public CxxTest::TestSuite
{
public:
  // This pair of boilerplate methods prevent the suite being created statically
  // This means the constructor isn't called when running other tests
  static CyclicGroupTest *createSuite() { return new CyclicGroupTest(); }
  static void destroySuite( CyclicGroupTest *suite ) { delete suite; }


  void testConstructor()
  {
      CyclicGroup_const_sptr group = boost::make_shared<const CyclicGroup>(SymmetryOperationFactory::Instance().createSymOp("-x,-y,-z"));

      TS_ASSERT_EQUALS(group->order(), 2);
  }

  void testCreate()
  {
      Group_const_sptr group = GroupFactory::create<CyclicGroup>("-x,-y,-z");
      TS_ASSERT(boost::dynamic_pointer_cast<const CyclicGroup>(group));

      TS_ASSERT_EQUALS(group->order(), 2);
  }

  void testMultiplication()
  {
      /* Even though this is part of Group, it's a good example and basically
       * the method used to generate point groups.
       */

      Group_const_sptr groupOne = GroupFactory::create<CyclicGroup>("-x,-y,z");
      Group_const_sptr groupTwo = GroupFactory::create<CyclicGroup>("x,-y,-z");

      // This is in fact point group 222
      Group_const_sptr groupThree = groupOne * groupTwo;

      TS_ASSERT_EQUALS(groupThree->order(), 4);

      Group_const_sptr groupFour = GroupFactory::create<CyclicGroup>("-x,-y,-z");

      // Which becomes mmm, if inversion is added.
      Group_const_sptr groupFive = groupFour * groupThree;
      TS_ASSERT_EQUALS(groupFive->order(), 8);
  }
};


#endif /* MANTID_GEOMETRY_CYCLICGROUPTEST_H_ */
