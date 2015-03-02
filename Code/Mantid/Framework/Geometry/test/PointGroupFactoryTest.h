#ifndef MANTID_GEOMETRY_POINTGROUPFACTORYTEST_H_
#define MANTID_GEOMETRY_POINTGROUPFACTORYTEST_H_

#include <cxxtest/TestSuite.h>

#include "MantidGeometry/Crystal/PointGroupFactory.h"
#include "MantidGeometry/Crystal/PointGroup.h"
#include "MantidKernel/Exception.h"

#include "MantidGeometry/Crystal/SpaceGroupFactory.h"

using Mantid::Geometry::PointGroupFactoryImpl;
using namespace Mantid::Geometry;


/* For testing the factory, three fake point groups are defined
 * and registered in the factory (see below).
 *
 * When the test is destroyed, these are explicitly unregistered,
 * so they don't interfere with other tests.
 */
class PointGroupFactoryTest : public CxxTest::TestSuite
{
public:
  // This pair of boilerplate methods prevent the suite being created statically
  // This means the constructor isn't called when running other tests
  static PointGroupFactoryTest *createSuite() { return new PointGroupFactoryTest(); }
  static void destroySuite( PointGroupFactoryTest *suite ) { delete suite; }

  PointGroupFactoryTest()
  {
      PointGroupFactory::Instance().subscribePointGroup("monoclinicA", "x,y,-z", "test");
      PointGroupFactory::Instance().subscribePointGroup("monoclinicB", "x,-y,-z", "test");
      PointGroupFactory::Instance().subscribePointGroup("triclinic", "-x,-y,-z", "test");
  }

  ~PointGroupFactoryTest()
  {
      // Unsubscribing the fake point groups
      PointGroupFactory::Instance().unsubscribePointGroup("monoclinicA");
      PointGroupFactory::Instance().unsubscribePointGroup("monoclinicB");
      PointGroupFactory::Instance().unsubscribePointGroup("triclinic");
  }

  void testCreatePointGroup()
  {
      TS_ASSERT_THROWS_NOTHING(PointGroupFactory::Instance().createPointGroup("monoclinicA"));
      TS_ASSERT_THROWS_NOTHING(PointGroupFactory::Instance().createPointGroup("monoclinicB"));
      TS_ASSERT_THROWS_NOTHING(PointGroupFactory::Instance().createPointGroup("triclinic"));

      TS_ASSERT_THROWS(PointGroupFactory::Instance().createPointGroup("cubicC"), std::invalid_argument);
  }

  void testGetAllPointGroupSymbols()
  {
      std::vector<std::string> symbols = PointGroupFactory::Instance().getAllPointGroupSymbols();

      TS_ASSERT_DIFFERS(findString(symbols, "monoclinicA"), symbols.end());
      TS_ASSERT_DIFFERS(findString(symbols, "monoclinicB"), symbols.end());
      TS_ASSERT_DIFFERS(findString(symbols, "triclinic"), symbols.end());
  }

  void testGetAllPointGroupSymbolsCrystalSystems()
  {
      std::vector<std::string> cubic = PointGroupFactory::Instance().getPointGroupSymbols(PointGroup::Monoclinic);

      TS_ASSERT_DIFFERS(findString(cubic, "monoclinicA"), cubic.end());
      TS_ASSERT_DIFFERS(findString(cubic, "monoclinicB"), cubic.end());

      std::vector<std::string> triclinic = PointGroupFactory::Instance().getPointGroupSymbols(PointGroup::Triclinic);
      TS_ASSERT_DIFFERS(findString(triclinic, "triclinic"), triclinic.end());
  }

  void testUnsubscribePointGroup()
  {
      TS_ASSERT_THROWS_NOTHING(PointGroupFactory::Instance().createPointGroup("monoclinicA"));

      PointGroupFactory::Instance().unsubscribePointGroup("monoclinicA");

      std::vector<std::string> allSymbols = PointGroupFactory::Instance().getAllPointGroupSymbols();
      TS_ASSERT_EQUALS(findString(allSymbols, "monoclinicA"), allSymbols.end());

      TS_ASSERT_THROWS(PointGroupFactory::Instance().createPointGroup("monoclinicA"), std::invalid_argument);

      PointGroupFactory::Instance().subscribePointGroup("monoclinicA", "x,y,-z", "test");
      TS_ASSERT_THROWS_NOTHING(PointGroupFactory::Instance().createPointGroup("monoclinicA"));
  }

  void testPointGroupSymbolCreation()
  {
      TS_ASSERT_THROWS_NOTHING(checkSpaceGroupSymbol("P -1"));
      TS_ASSERT_THROWS_NOTHING(checkSpaceGroupSymbol("P 1 2/m 1"));
      TS_ASSERT_THROWS_NOTHING(checkSpaceGroupSymbol("P 1 1 2/m"));
      TS_ASSERT_THROWS_NOTHING(checkSpaceGroupSymbol("F d d d"));
      TS_ASSERT_THROWS_NOTHING(checkSpaceGroupSymbol("C m c a"));
      TS_ASSERT_THROWS_NOTHING(checkSpaceGroupSymbol("P 43/a m d"));
      TS_ASSERT_THROWS_NOTHING(checkSpaceGroupSymbol("I 41/c c n"));
      TS_ASSERT_THROWS_NOTHING(checkSpaceGroupSymbol("P 63/m m c"));
      TS_ASSERT_THROWS_NOTHING(checkSpaceGroupSymbol("F d -3 m"));
      TS_ASSERT_THROWS_NOTHING(checkSpaceGroupSymbol("P -3 c 1"));
      TS_ASSERT_THROWS_NOTHING(checkSpaceGroupSymbol("P -3 1 d"));
      TS_ASSERT_THROWS_NOTHING(checkSpaceGroupSymbol("P 4/a"));
      TS_ASSERT_THROWS_NOTHING(checkSpaceGroupSymbol("P 62/d"));
      TS_ASSERT_THROWS_NOTHING(checkSpaceGroupSymbol("I d -3"));
      TS_ASSERT_THROWS_NOTHING(checkSpaceGroupSymbol("I 4/c"));
  }


private:
  std::vector<std::string>::const_iterator findString(const std::vector<std::string> &vector, const std::string &searchString)
  {
      return std::find(vector.begin(), vector.end(), searchString);
  }

  void checkSpaceGroupSymbol(const std::string &symbol)
  {
      PointGroupFactory::Instance().createPointGroupFromSpaceGroupSymbol(symbol);
  }
};


#endif /* MANTID_GEOMETRY_POINTGROUPFACTORYTEST_H_ */
