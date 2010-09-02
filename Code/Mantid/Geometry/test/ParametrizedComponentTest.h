#ifndef MANTID_TESTPARCOMPONENT__
#define MANTID_TESTPARCOMPONENT__

#include <cxxtest/TestSuite.h>
#include <cmath>
#include <iostream>
#include <string>
#include "MantidGeometry/Instrument/ParametrizedComponent.h"
#include "MantidGeometry/V3D.h"
#include "MantidGeometry/Quat.h"
#include "MantidKernel/Exception.h"

using namespace Mantid::Geometry;

class testParComponent : public CxxTest::TestSuite
{
public:

  testParComponent::testParComponent() : m_parentComp(NULL), m_childOneComp(NULL), m_childTwoComp(NULL), m_paramMap(NULL),
  m_strName("StringParam"), m_strValue("test-string"), m_dblName("DblParam"), m_dblValue(10.0),
  m_posName("PosParam"), m_posValue(1,1,1), m_quatName("QuatParam"),
  m_quatValue(2,3,4,5)
  {
  }

  void testEmptyConstructor()
  {
    Component q;

    ParameterMap pmap;
    ParametrizedComponent pq(&q,pmap);

    TS_ASSERT_EQUALS(pq.getName(),"");
    TS_ASSERT(!pq.getParent());
    TS_ASSERT_EQUALS(pq.getRelativePos(),V3D(0,0,0));
    TS_ASSERT_EQUALS(pq.getRelativeRot(),Quat(1,0,0,0));
    //as there is no parent GetPos should equal getRelativePos
    TS_ASSERT_EQUALS(pq.getRelativePos(),pq.getPos());
  }

  void testNameLocationOrientationParentValueConstructor()
  {
    Component parent("Parent",V3D(1,1,1));
    //name and parent
    Component q("Child",V3D(5,6,7),Quat(1,1,1,1),&parent);
    ParameterMap pmap;
    ParametrizedComponent pq(&q,pmap);

    TS_ASSERT_EQUALS(pq.getName(),"Child");
    //check the parent
    TS_ASSERT(pq.getParent());
    TS_ASSERT_EQUALS(pq.getParent()->getName(),parent.getName());

    TS_ASSERT_EQUALS(pq.getRelativePos(),V3D(5,6,7));
    TS_ASSERT_EQUALS(pq.getPos(),V3D(6,7,8));
    TS_ASSERT_EQUALS(pq.getRelativeRot(),Quat(1,1,1,1));
  }

  void testGetParameter()
  {
    ParametrizedComponent * paramComp = createSingleParameterizedComponent();

    TS_ASSERT_EQUALS(paramComp->getStringParameter(m_strName)[0], m_strValue);
    TS_ASSERT_EQUALS(paramComp->getNumberParameter(m_dblName)[0], m_dblValue);
    TS_ASSERT_EQUALS(paramComp->getPositionParameter(m_posName)[0], m_posValue);
    TS_ASSERT_EQUALS(paramComp->getRotationParameter(m_quatName)[0], m_quatValue);
    
    cleanUpParametrizedComponent();
  }

  void testThatCorrectParametersAreListed()
  {
    ParametrizedComponent * paramComp = createSingleParameterizedComponent();
    std::set<std::string> paramNames = paramComp->getParameterNames();

    TS_ASSERT_EQUALS(paramNames.size(), 4);
    checkBaseParameterNames(paramNames);
    cleanUpParametrizedComponent();
  }

  void testThatRecursiveParameterSearchReturnsNamesOfAllParentParameters()
  {
    createParameterizedTree();
    ParametrizedComponent *parent = new ParametrizedComponent(m_parentComp, *m_paramMap);
    ParametrizedComponent *child = new ParametrizedComponent(m_childOneComp, *m_paramMap);
    ParametrizedComponent *grandchild = new ParametrizedComponent(m_childTwoComp, *m_paramMap);
    
    //Parent
    std::set<std::string> paramNames = parent->getParameterNames();
    TS_ASSERT_EQUALS(paramNames.size(), 4);
    checkBaseParameterNames(paramNames);
    //Child
    paramNames = child->getParameterNames();
    TS_ASSERT_EQUALS(paramNames.size(), 5);
    checkBaseParameterNames(paramNames);
    TS_ASSERT_DIFFERS(paramNames.find(m_strName + "_child1"), paramNames.end());
    // Grandchild
    paramNames = grandchild->getParameterNames();
    TS_ASSERT_EQUALS(paramNames.size(), 6);
    checkBaseParameterNames(paramNames);
    TS_ASSERT_DIFFERS(paramNames.find(m_strName + "_child1"), paramNames.end());
    TS_ASSERT_DIFFERS(paramNames.find(m_strName + "_child2"), paramNames.end());

    delete parent;
    delete child;
    delete grandchild;
    cleanUpParametrizedComponent();
  }

  void testThatNonRecursiveParameterSearchReturnsOnlyComponentParameters()
  {
    createParameterizedTree();
    ParametrizedComponent *child = new ParametrizedComponent(m_childOneComp, *m_paramMap);
    std::set<std::string> paramNames = child->getParameterNames(false);
    TS_ASSERT_EQUALS(paramNames.size(), 1);
    TS_ASSERT_DIFFERS(paramNames.find(m_strName + "_child1"), paramNames.end());

    ParametrizedComponent *grandchild = new ParametrizedComponent(m_childTwoComp, *m_paramMap);
    paramNames = grandchild->getParameterNames(false);
    TS_ASSERT_EQUALS(paramNames.size(), 1);
    TS_ASSERT_DIFFERS(paramNames.find(m_strName + "_child2"), paramNames.end());

    delete child;
    delete grandchild;
    cleanUpParametrizedComponent();
  }

  void testThatParComponentHasDefinedParameter()
  {
    createParameterizedTree();
    ParametrizedComponent *child = new ParametrizedComponent(m_childOneComp, *m_paramMap);
    ParametrizedComponent *grandchild = new ParametrizedComponent(m_childTwoComp, *m_paramMap);

    TS_ASSERT_EQUALS(child->hasParameter(m_strName + "_child1"), true);
    TS_ASSERT_EQUALS(grandchild->hasParameter(m_strName + "_child2"), true);

    //Non-recursive
    TS_ASSERT_EQUALS(grandchild->hasParameter(m_strName + "_child2", false), true);
    TS_ASSERT_EQUALS(grandchild->hasParameter(m_strName, false), false);

    delete child;
    delete grandchild;
    cleanUpParametrizedComponent();
  }

private:
  
  void checkBaseParameterNames(const std::set<std::string> & paramNames)
  {
    TS_ASSERT_DIFFERS(paramNames.find(m_strName), paramNames.end());
    TS_ASSERT_DIFFERS(paramNames.find(m_dblName), paramNames.end());
    TS_ASSERT_DIFFERS(paramNames.find(m_posName), paramNames.end());
    TS_ASSERT_DIFFERS(paramNames.find(m_quatName), paramNames.end());
  }

  ParametrizedComponent * createSingleParameterizedComponent()
  {
    m_parentComp = new Component("Parent",V3D(1,1,1));
    m_paramMap = new ParameterMap;

    m_paramMap->add("string", m_parentComp, m_strName, m_strValue);
    m_paramMap->add("double", m_parentComp, m_dblName, m_dblValue);
    m_paramMap->add("V3D", m_parentComp, m_posName, m_posValue);
    m_paramMap->add("Quat", m_parentComp, m_quatName, m_quatValue);

    return new ParametrizedComponent(m_parentComp, *m_paramMap);
  }

  void createParameterizedTree()
  {
    m_parentComp = new Component("Parent",V3D(1,1,1));
    m_paramMap = new ParameterMap;

    m_paramMap->add("string", m_parentComp, m_strName, m_strValue);
    m_paramMap->add("double", m_parentComp, m_dblName, m_dblValue);
    m_paramMap->add("V3D", m_parentComp, m_posName, m_posValue);
    m_paramMap->add("Quat", m_parentComp, m_quatName, m_quatValue);

    m_childOneComp = new Component("Child1", V3D(1,2,3), m_parentComp);
    m_paramMap->add("string", m_childOneComp, m_strName + "_child1", m_strValue + "_child1");
    m_childTwoComp = new Component("Child2", V3D(3,2,1), m_childOneComp);
    m_paramMap->add("string", m_childTwoComp, m_strName + "_child2", m_strValue + "_child2");
  }


  void cleanUpParametrizedComponent()
  {
    delete m_parentComp;
    delete m_childOneComp;
    delete m_childTwoComp;
    delete m_paramMap;
  }

  Component *m_parentComp,*m_childOneComp,*m_childTwoComp;
  ParameterMap *m_paramMap;

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
