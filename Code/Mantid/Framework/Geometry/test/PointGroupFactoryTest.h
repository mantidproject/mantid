#ifndef MANTID_GEOMETRY_POINTGROUPFACTORYTEST_H_
#define MANTID_GEOMETRY_POINTGROUPFACTORYTEST_H_

#include <cxxtest/TestSuite.h>

#include "MantidGeometry/Crystal/PointGroupFactory.h"
#include "MantidGeometry/Crystal/PointGroup.h"
#include "MantidKernel/Exception.h"


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
};

DECLARE_POINTGROUP(TestPointGroupCubicA);
DECLARE_POINTGROUP(TestPointGroupCubicB);
DECLARE_POINTGROUP(TestPointGroupTriclinic);


class PointGroupFactoryTest : public CxxTest::TestSuite
{
public:
  // This pair of boilerplate methods prevent the suite being created statically
  // This means the constructor isn't called when running other tests
  static PointGroupFactoryTest *createSuite() { return new PointGroupFactoryTest(); }
  static void destroySuite( PointGroupFactoryTest *suite ) { delete suite; }

  ~PointGroupFactoryTest()
  {
      // Unsubscribing the fake point groups
      PointGroupFactory::Instance().unsubscribePointgroup("cubicA");
      PointGroupFactory::Instance().unsubscribePointgroup("cubicB");
      PointGroupFactory::Instance().unsubscribePointgroup("triclinic");
  }

  void testCreatePointGroup()
  {
      TS_ASSERT_THROWS_NOTHING(PointGroupFactory::Instance().createPointgroup("cubicA"));
      TS_ASSERT_THROWS_NOTHING(PointGroupFactory::Instance().createPointgroup("cubicB"));
      TS_ASSERT_THROWS_NOTHING(PointGroupFactory::Instance().createPointgroup("triclinic"));

      TS_ASSERT_THROWS(PointGroupFactory::Instance().createPointgroup("cubicC"), Mantid::Kernel::Exception::NotFoundError);
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
      std::vector<std::string> cubic = PointGroupFactory::Instance().getAllPointGroupSymbols(PointGroup::Cubic);
      TS_ASSERT_DIFFERS(findString(cubic, "cubicA"), cubic.end());
      TS_ASSERT_DIFFERS(findString(cubic, "cubicB"), cubic.end());

      std::vector<std::string> triclinic = PointGroupFactory::Instance().getAllPointGroupSymbols(PointGroup::Triclinic);
      TS_ASSERT_DIFFERS(findString(triclinic, "triclinic"), triclinic.end());
  }

  void testUnsubscribePointGroup()
  {
      TS_ASSERT_THROWS_NOTHING(PointGroupFactory::Instance().createPointgroup("cubicA"));

      PointGroupFactory::Instance().unsubscribePointgroup("cubicA");
      TS_ASSERT_THROWS(PointGroupFactory::Instance().create("cubicA"), Mantid::Kernel::Exception::NotFoundError);

      PointGroupFactory::Instance().subscribePointgroup<TestPointGroupCubicA>();
      TS_ASSERT_THROWS_NOTHING(PointGroupFactory::Instance().createPointgroup("cubicA"));
  }

private:
  std::vector<std::string>::const_iterator findString(const std::vector<std::string> &vector, const std::string &searchString)
  {
      return std::find(vector.begin(), vector.end(), searchString);
  }
};


#endif /* MANTID_GEOMETRY_POINTGROUPFACTORYTEST_H_ */
