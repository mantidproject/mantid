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
class TestPointGroupCubicA : public PointGroup
{
public:
    TestPointGroupCubicA() : PointGroup("cubicA")
    { }
    ~TestPointGroupCubicA() { }

    std::string getName() const { return "cubicA (test)"; }
    bool isEquivalent(const Mantid::Kernel::V3D &hkl, const Mantid::Kernel::V3D &hkl2) const
    {
        UNUSED_ARG(hkl);
        UNUSED_ARG(hkl2);

        return false;
    }

    PointGroup::CrystalSystem crystalSystem() const { return PointGroup::Cubic; }

    void init() { }
};

class TestPointGroupCubicB : public PointGroup
{
public:
    TestPointGroupCubicB() : PointGroup("cubicB")
    { }
    ~TestPointGroupCubicB() { }

    std::string getName() const { return "cubicB (test)"; }
    bool isEquivalent(const Mantid::Kernel::V3D &hkl, const Mantid::Kernel::V3D &hkl2) const
    {
        UNUSED_ARG(hkl);
        UNUSED_ARG(hkl2);

        return false;
    }

    PointGroup::CrystalSystem crystalSystem() const { return PointGroup::Cubic; }

    void init() { }
};

class TestPointGroupTriclinic : public PointGroup
{
public:
    TestPointGroupTriclinic() : PointGroup("triclinic")
    { }
    ~TestPointGroupTriclinic() { }

    std::string getName() const { return "triclinic (test)"; }
    bool isEquivalent(const Mantid::Kernel::V3D &hkl, const Mantid::Kernel::V3D &hkl2) const
    {
        UNUSED_ARG(hkl);
        UNUSED_ARG(hkl2);

        return false;
    }

    PointGroup::CrystalSystem crystalSystem() const { return PointGroup::Triclinic; }

    void init() { }
};

class PointGroupFactoryTest : public CxxTest::TestSuite
{
public:
  // This pair of boilerplate methods prevent the suite being created statically
  // This means the constructor isn't called when running other tests
  static PointGroupFactoryTest *createSuite() { return new PointGroupFactoryTest(); }
  static void destroySuite( PointGroupFactoryTest *suite ) { delete suite; }

  PointGroupFactoryTest()
  {
      PointGroupFactory::Instance().subscribePointGroup<TestPointGroupCubicA>();
      PointGroupFactory::Instance().subscribePointGroup<TestPointGroupCubicB>();
      PointGroupFactory::Instance().subscribePointGroup<TestPointGroupTriclinic>();
  }

  ~PointGroupFactoryTest()
  {
      // Unsubscribing the fake point groups
      PointGroupFactory::Instance().unsubscribePointGroup("cubicA");
      PointGroupFactory::Instance().unsubscribePointGroup("cubicB");
      PointGroupFactory::Instance().unsubscribePointGroup("triclinic");
  }

  void testCreatePointGroup()
  {
      TS_ASSERT_THROWS_NOTHING(PointGroupFactory::Instance().createPointGroup("cubicA"));
      TS_ASSERT_THROWS_NOTHING(PointGroupFactory::Instance().createPointGroup("cubicB"));
      TS_ASSERT_THROWS_NOTHING(PointGroupFactory::Instance().createPointGroup("triclinic"));

      TS_ASSERT_THROWS(PointGroupFactory::Instance().createPointGroup("cubicC"), Mantid::Kernel::Exception::NotFoundError);
  }

  void testGetAllPointGroupSymbols()
  {
      std::vector<std::string> symbols = PointGroupFactory::Instance().getAllPointGroupSymbols();

      TS_ASSERT_DIFFERS(findString(symbols, "cubicA"), symbols.end());
      TS_ASSERT_DIFFERS(findString(symbols, "cubicB"), symbols.end());
      TS_ASSERT_DIFFERS(findString(symbols, "triclinic"), symbols.end());
  }

  void testGetAllPointGroupSymbolsCrystalSystems()
  {
      std::vector<std::string> cubic = PointGroupFactory::Instance().getPointGroupSymbols(PointGroup::Cubic);
      TS_ASSERT_DIFFERS(findString(cubic, "cubicA"), cubic.end());
      TS_ASSERT_DIFFERS(findString(cubic, "cubicB"), cubic.end());

      std::vector<std::string> triclinic = PointGroupFactory::Instance().getPointGroupSymbols(PointGroup::Triclinic);
      TS_ASSERT_DIFFERS(findString(triclinic, "triclinic"), triclinic.end());
  }

  void testUnsubscribePointGroup()
  {
      TS_ASSERT_THROWS_NOTHING(PointGroupFactory::Instance().createPointGroup("cubicA"));

      PointGroupFactory::Instance().unsubscribePointGroup("cubicA");

      std::vector<std::string> allSymbols = PointGroupFactory::Instance().getAllPointGroupSymbols();
      TS_ASSERT_EQUALS(findString(allSymbols, "cubicA"), allSymbols.end());

      TS_ASSERT_THROWS(PointGroupFactory::Instance().create("cubicA"), Mantid::Kernel::Exception::NotFoundError);

      PointGroupFactory::Instance().subscribePointGroup<TestPointGroupCubicA>();
      TS_ASSERT_THROWS_NOTHING(PointGroupFactory::Instance().createPointGroup("cubicA"));
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
