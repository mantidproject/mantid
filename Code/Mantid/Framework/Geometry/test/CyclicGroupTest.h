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
      Group_const_sptr group = CyclicGroup::create("-x,-y,-z");
      TS_ASSERT(boost::dynamic_pointer_cast<const CyclicGroup>(group));

      TS_ASSERT_EQUALS(group->order(), 2);
  }

  void testMultiplication()
  {
      /* Even though this is part of Group, it's a good example and basically
       * the method used to generate point groups.
       */

      Group_const_sptr groupOne = CyclicGroup::create("-x,-y,z");
      Group_const_sptr groupTwo = CyclicGroup::create("x,-y,-z");

      // This is in fact point group 222
      Group_const_sptr groupThree = groupOne * groupTwo;

      TS_ASSERT_EQUALS(groupThree->order(), 4);

      Group_const_sptr groupFour = CyclicGroup::create("-x,-y,-z");

      // Which becomes mmm, if inversion is added.
      Group_const_sptr groupFive = groupFour * groupThree;
      TS_ASSERT_EQUALS(groupFive->order(), 8);
  }

  void testSpaceGroup()
  {
      // Small test, constructing Fm-3m (225) from the generators listed in ITA
      Group_const_sptr group1 = CyclicGroup::create("-x,-y,z");
      Group_const_sptr group2 = CyclicGroup::create("-x,y,-z");
      Group_const_sptr group3 = CyclicGroup::create("z,x,y");
      Group_const_sptr group4 = CyclicGroup::create("y,x,-z");
      Group_const_sptr group5 = CyclicGroup::create("-x,-y,-z");

      // Make a translation group "F"
      std::vector<SymmetryOperation> lops;
      lops.push_back(SymmetryOperation("x,y,z"));
      lops.push_back(SymmetryOperation("x,y+1/2,z+1/2"));
      lops.push_back(SymmetryOperation("x+1/2,y+1/2,z"));
      lops.push_back(SymmetryOperation("x+1/2,y,z+1/2"));

      Group_const_sptr translationGroup = boost::make_shared<const Group>(lops);

      // Generate space group by multiplying the generating groups
      Group_const_sptr fm3barm = group1 * group2 * group3 * group4 * group5 * translationGroup;

      // Output the symmetry operations including Centering in x,y,z-form
      std::cout << std::endl;
      std::cout << "Order: " << fm3barm->order() << std::endl;
      std::vector<SymmetryOperation> ops = fm3barm->getSymmetryOperations();
      for(auto it = ops.begin(); it != ops.end(); ++it) {
          std::cout << (*it).identifier() << std::endl;
      }

      V3D w2a(0, 0, 0);
      std::vector<V3D> w2at = fm3barm * w2a;
      std::cout << "Equivalents of " << w2a << ":" << std::endl;
      for(auto it = w2at.begin(); it != w2at.end(); ++it) {
          std::cout << "   " << (*it) << std::endl;
      }
  }



};


#endif /* MANTID_GEOMETRY_CYCLICGROUPTEST_H_ */
