#ifndef MANTID_TESTPARCOMPONENT__
#define MANTID_TESTPARCOMPONENT__

#include "MantidGeometry/IComponent.h"
#include "MantidGeometry/Instrument/Component.h"
#include "MantidKernel/Exception.h"
#include "MantidKernel/Quat.h"
#include "MantidKernel/V3D.h"
#include <boost/make_shared.hpp>
#include <cmath>
#include <cxxtest/TestSuite.h>
#include <string>

using namespace Mantid;
using namespace Mantid::Geometry;
using Mantid::Kernel::Quat;
using Mantid::Kernel::V3D;

/** This test used to refer to ParametrizedComponent, a
 * class that has (as of Nov 2010) been folded back into
 * Component.
 */
class ParametrizedComponentTest : public CxxTest::TestSuite {
public:
  // This pair of boilerplate methods prevent the suite being created statically
  // This means the constructor isn't called when running other tests
  static ParametrizedComponentTest *createSuite() {
    return new ParametrizedComponentTest();
  }
  static void destroySuite(ParametrizedComponentTest *suite) { delete suite; }

  ParametrizedComponentTest()
      : m_parentComp(nullptr), m_childOneComp(nullptr), m_childTwoComp(nullptr),
        m_paramMap(), m_strName("StringParam"), m_strValue("test-string"),
        m_dblName("DblParam"), m_dblValue(10.0), m_posName("PosParam"),
        m_posValue(1, 1, 1), m_quatName("QuatParam"), m_quatValue(2, 3, 4, 5) {}

  void testEmptyConstructor() {
    Component q;

    ParameterMap_const_sptr pmap(new ParameterMap());
    Component pq(&q, pmap.get());

    TS_ASSERT_EQUALS(pq.getName(), "");
    TS_ASSERT(!pq.getParent());
    TS_ASSERT_EQUALS(pq.getRelativePos(), V3D(0, 0, 0));
    TS_ASSERT_EQUALS(pq.getRelativeRot(), Quat(1, 0, 0, 0));
    // as there is no parent GetPos should equal getRelativePos
    TS_ASSERT_EQUALS(pq.getRelativePos(), pq.getPos());
  }

  void testIsParametrized() {
    Component q;
    ParameterMap_const_sptr pmap(new ParameterMap());
    Component pq(&q, pmap.get());

    TS_ASSERT(!q.isParametrized());
    TS_ASSERT(pq.isParametrized());
  }

  void testNameLocationOrientationParentValueConstructor() {
    Component parent("Parent", V3D(1, 1, 1));
    // name and parent
    Component q("Child", V3D(5, 6, 7), Quat(1, 1, 1, 1), &parent);
    ParameterMap_const_sptr pmap(new ParameterMap());
    Component pq(&q, pmap.get());

    TS_ASSERT_EQUALS(pq.getName(), "Child");
    // check the parent
    TS_ASSERT(pq.getParent());
    TS_ASSERT_EQUALS(pq.getParent()->getName(), parent.getName());

    TS_ASSERT_EQUALS(pq.getRelativePos(), V3D(5, 6, 7));
    TS_ASSERT_EQUALS(pq.getPos(), V3D(6, 7, 8));
    TS_ASSERT_EQUALS(pq.getRelativeRot(), Quat(1, 1, 1, 1));
  }

  void testGetParameterAndItsType() {
    Component *paramComp = createSingleParameterizedComponent();
    TS_ASSERT_EQUALS(paramComp->getStringParameter(m_strName).size(), 1);
    TS_ASSERT_EQUALS(paramComp->getStringParameter(m_strName)[0], m_strValue);
    TS_ASSERT_EQUALS(paramComp->getNumberParameter(m_dblName)[0], m_dblValue);
    TS_ASSERT_EQUALS(paramComp->getPositionParameter(m_posName)[0], m_posValue);
    TS_ASSERT_EQUALS(paramComp->getRotationParameter(m_quatName)[0],
                     m_quatValue);

    std::string typeName;
    TS_ASSERT_THROWS_NOTHING(typeName = paramComp->getParameterType(m_strName));
    TS_ASSERT_EQUALS(ParameterMap::pString(), typeName);
    TS_ASSERT_THROWS_NOTHING(typeName = paramComp->getParameterType(m_dblName));
    TS_ASSERT_EQUALS(ParameterMap::pDouble(), typeName);
    TS_ASSERT_THROWS_NOTHING(typeName = paramComp->getParameterType(m_posName));
    TS_ASSERT_EQUALS(ParameterMap::pV3D(), typeName);
    TS_ASSERT_THROWS_NOTHING(typeName =
                                 paramComp->getParameterType(m_quatName));
    TS_ASSERT_EQUALS(ParameterMap::pQuat(), typeName);

    delete paramComp;
    cleanUpComponent();
  }

  void testThatNonRecursiveGetParameterOnlySearchesCurrentComponent() {
    createParameterizedTree();
    Component *grandchild = new Component(m_childTwoComp, m_paramMap.get());

    TS_ASSERT_EQUALS(grandchild->getStringParameter(m_strName, false).size(),
                     0);
    TS_ASSERT_EQUALS(grandchild->getNumberParameter(m_dblName, false).size(),
                     0);
    TS_ASSERT_EQUALS(grandchild->getPositionParameter(m_posName, false).size(),
                     0);
    TS_ASSERT_EQUALS(grandchild->getRotationParameter(m_quatName, false).size(),
                     0);

    std::vector<std::string> params =
        grandchild->getStringParameter(m_strName + "_child2", false);
    const size_t nparams = params.size();
    TS_ASSERT_EQUALS(nparams, 1);
    if (nparams > 0) {
      TS_ASSERT_EQUALS(params[0], m_strValue + "_child2");
    }

    delete grandchild;
    cleanUpComponent();
  }

  void testThatCorrectParametersAreListed() {
    Component *paramComp = createSingleParameterizedComponent();
    auto paramNames = paramComp->getParameterNames();

    TS_ASSERT_EQUALS(paramNames.size(), 4);
    checkBaseParameterNamesExist(paramNames);
    delete paramComp;
    cleanUpComponent();
  }

  void testThatRecursiveParameterSearchReturnsNamesOfAllParentParameters() {
    createParameterizedTree();
    Component *parent = new Component(m_parentComp, m_paramMap.get());
    Component *child = new Component(m_childOneComp, m_paramMap.get());
    Component *grandchild = new Component(m_childTwoComp, m_paramMap.get());

    // Parent
    auto paramNames = parent->getParameterNames();
    TS_ASSERT_EQUALS(paramNames.size(), 4);
    checkBaseParameterNamesExist(paramNames);
    // Child
    paramNames = child->getParameterNames();
    TS_ASSERT_EQUALS(paramNames.size(), 5);
    checkBaseParameterNamesExist(paramNames);
    TS_ASSERT_DIFFERS(paramNames.find(m_strName + "_child1"), paramNames.end());
    // Grandchild
    paramNames = grandchild->getParameterNames();
    TS_ASSERT_EQUALS(paramNames.size(), 6);
    checkBaseParameterNamesExist(paramNames);
    TS_ASSERT_DIFFERS(paramNames.find(m_strName + "_child1"), paramNames.end());
    TS_ASSERT_DIFFERS(paramNames.find(m_strName + "_child2"), paramNames.end());

    delete parent;
    delete child;
    delete grandchild;
    cleanUpComponent();
  }

  void testThatNonRecursiveParameterSearchReturnsOnlyComponentParameters() {
    createParameterizedTree();
    Component *child = new Component(m_childOneComp, m_paramMap.get());
    auto paramNames = child->getParameterNames(false);
    TS_ASSERT_EQUALS(paramNames.size(), 1);
    TS_ASSERT_DIFFERS(paramNames.find(m_strName + "_child1"), paramNames.end());

    Component *grandchild = new Component(m_childTwoComp, m_paramMap.get());
    paramNames = grandchild->getParameterNames(false);
    TS_ASSERT_EQUALS(paramNames.size(), 1);
    TS_ASSERT_DIFFERS(paramNames.find(m_strName + "_child2"), paramNames.end());

    delete child;
    delete grandchild;
    cleanUpComponent();
  }

  void testThatParComponentHasDefinedParameter() {
    createParameterizedTree();
    Component *child = new Component(m_childOneComp, m_paramMap.get());
    Component *grandchild = new Component(m_childTwoComp, m_paramMap.get());

    TS_ASSERT_EQUALS(child->hasParameter(m_strName + "_child1"), true);
    TS_ASSERT_EQUALS(grandchild->hasParameter(m_strName + "_child2"), true);

    // Non-recursive
    TS_ASSERT_EQUALS(grandchild->hasParameter(m_strName + "_child2", false),
                     true);
    TS_ASSERT_EQUALS(grandchild->hasParameter(m_strName, false), false);

    delete child;
    delete grandchild;
    cleanUpComponent();
  }

private:
  void checkBaseParameterNamesExist(const std::set<std::string> &paramNames) {
    TS_ASSERT_DIFFERS(paramNames.find(m_strName), paramNames.end());
    TS_ASSERT_DIFFERS(paramNames.find(m_dblName), paramNames.end());
    TS_ASSERT_DIFFERS(paramNames.find(m_posName), paramNames.end());
    TS_ASSERT_DIFFERS(paramNames.find(m_quatName), paramNames.end());
  }

  Component *createSingleParameterizedComponent() {
    m_parentComp = new Component("Parent", V3D(1, 1, 1));
    m_paramMap = boost::make_shared<ParameterMap>();

    m_paramMap->add("string", m_parentComp, m_strName, m_strValue);
    m_paramMap->add("double", m_parentComp, m_dblName, m_dblValue);
    m_paramMap->add("V3D", m_parentComp, m_posName, m_posValue);
    m_paramMap->add("Quat", m_parentComp, m_quatName, m_quatValue);

    ParameterMap_const_sptr const_pmap =
        boost::dynamic_pointer_cast<const ParameterMap>(m_paramMap);
    return new Component(m_parentComp, const_pmap.get());
  }

  void createParameterizedTree() {
    m_parentComp = new Component("Parent", V3D(1, 1, 1));
    m_paramMap = boost::make_shared<ParameterMap>();

    m_paramMap->add("string", m_parentComp, m_strName, m_strValue);
    m_paramMap->add("double", m_parentComp, m_dblName, m_dblValue);
    m_paramMap->add("V3D", m_parentComp, m_posName, m_posValue);
    m_paramMap->add("Quat", m_parentComp, m_quatName, m_quatValue);

    m_childOneComp = new Component("Child1", V3D(1, 2, 3), m_parentComp);
    m_paramMap->add("string", m_childOneComp, m_strName + "_child1",
                    m_strValue + "_child1");
    m_childTwoComp = new Component("Child2", V3D(3, 2, 1), m_childOneComp);
    m_paramMap->add("string", m_childTwoComp, m_strName + "_child2",
                    m_strValue + "_child2");
  }

  void cleanUpComponent() {
    delete m_parentComp;
    m_parentComp = nullptr;
    delete m_childOneComp;
    m_childOneComp = nullptr;
    delete m_childTwoComp;
    m_childTwoComp = nullptr;
    // delete m_paramMap;    m_paramMap = NULL;
  }

  Component *m_parentComp, *m_childOneComp, *m_childTwoComp;
  ParameterMap_sptr m_paramMap;

  const std::string m_strName;
  const std::string m_strValue;
  const std::string m_dblName;
  const double m_dblValue;
  const std::string m_posName;
  const V3D m_posValue;
  const std::string m_quatName;
  const Quat m_quatValue;
};

#endif
