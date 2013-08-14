#ifndef MULTIDOMAINCREATORTEST_H_
#define MULTIDOMAINCREATORTEST_H_

#include "MantidAPI/FunctionDomain1D.h"
#include "MantidAPI/FunctionValues.h"
#include "MantidAPI/JointDomain.h"
#include "MantidAPI/FrameworkManager.h"
#include "MantidAPI/WorkspaceProperty.h"
#include "MantidCurveFitting/MultiDomainCreator.h"
#include "MantidCurveFitting/FitMW.h"
#include "MantidKernel/PropertyManager.h"

#include "MantidTestHelpers/FakeObjects.h"

#include <cxxtest/TestSuite.h>
#include <boost/make_shared.hpp>
#include <algorithm>
#include <iostream>

using namespace Mantid;
using namespace Mantid::API;
using namespace Mantid::CurveFitting;

class MultiDomainCreatorTest : public CxxTest::TestSuite
{
public:
  // This pair of boilerplate methods prevent the suite being created statically
  // This means the constructor isn't called when running other tests
  static MultiDomainCreatorTest *createSuite() { return new MultiDomainCreatorTest(); }
  static void destroySuite( MultiDomainCreatorTest *suite ) { delete suite; }
  
  MultiDomainCreatorTest()
  {
    //FrameworkManager::Instance();
    ws1.reset(new WorkspaceTester);
    ws1->initialize(1,10,10);
    {
      Mantid::MantidVec& x = ws1->dataX(0);
      Mantid::MantidVec& y = ws1->dataY(0);
      //Mantid::MantidVec& e = ws1->dataE(0);
      for(size_t i = 0; i < ws1->blocksize(); ++i)
      {
        x[i] =  0.1 * double(i);
        y[i] =  1.0;
      }
    }

    ws2.reset(new WorkspaceTester);
    ws2->initialize(1,10,10);
    {
      Mantid::MantidVec& x = ws2->dataX(0);
      Mantid::MantidVec& y = ws2->dataY(0);
      //Mantid::MantidVec& e = ws2->dataE(0);
      for(size_t i = 0; i < ws2->blocksize(); ++i)
      {
        x[i] = 1 + 0.1 * double(i);
        y[i] = 2.0;
      }
    }

    ws3.reset(new WorkspaceTester);
    ws3->initialize(1,10,10);
    {
      Mantid::MantidVec& x = ws3->dataX(0);
      Mantid::MantidVec& y = ws3->dataY(0);
      //Mantid::MantidVec& e = ws3->dataE(0);
      for(size_t i = 0; i < ws3->blocksize(); ++i)
      {
        x[i] = 2 + 0.1 * double(i);
        y[i] = 3.0;
      }
    }
  }

  void test_creator()
  {
    Mantid::Kernel::PropertyManager manager;
    manager.declareProperty(new WorkspaceProperty<Workspace>("WS1","",Direction::Input));
    manager.declareProperty(new WorkspaceProperty<Workspace>("WS2","",Direction::Input));
    manager.declareProperty(new WorkspaceProperty<Workspace>("WS3","",Direction::Input));

    std::vector<std::string> propNames;
    propNames.push_back("WS1");
    propNames.push_back("WS2");
    propNames.push_back("WS3");
    MultiDomainCreator multi( &manager, propNames );

    TS_ASSERT_EQUALS(multi.getNCreators(), 3);
    TS_ASSERT( !multi.hasCreator(0) );
    TS_ASSERT( !multi.hasCreator(1) );
    TS_ASSERT( !multi.hasCreator(2) );

    manager.setProperty("WS1", ws1);
    manager.setProperty("WS2", ws2);
    manager.setProperty("WS3", ws3);

    FitMW* creator = new FitMW( &manager, "WS1" );
    creator->declareDatasetProperties("1");
    multi.setCreator(0, creator);
    creator = new FitMW( &manager, "WS2" );
    creator->declareDatasetProperties("2");
    multi.setCreator(1, creator);
    creator = new FitMW( &manager, "WS3" );
    creator->declareDatasetProperties("3");
    multi.setCreator(2, creator);

    TS_ASSERT( multi.hasCreator(0) );
    TS_ASSERT( multi.hasCreator(1) );
    TS_ASSERT( multi.hasCreator(2) );

    manager.setProperty("WorkspaceIndex1", 0);
    manager.setProperty("WorkspaceIndex2", 0);
    manager.setProperty("WorkspaceIndex3", 0);

    FunctionDomain_sptr domain;
    IFunctionValues_sptr values;

    multi.createDomain( domain, values );
    
    TS_ASSERT( domain );
    TS_ASSERT( values );

    auto jointDomain = boost::dynamic_pointer_cast<JointDomain>( domain );
    TS_ASSERT( jointDomain );
    TS_ASSERT_EQUALS(jointDomain->getNParts(), 3);

    auto d1 = dynamic_cast<const FunctionDomain1D*>( &jointDomain->getDomain( 0 ) );
    auto d2 = dynamic_cast<const FunctionDomain1D*>( &jointDomain->getDomain( 1 ) );
    auto d3 = dynamic_cast<const FunctionDomain1D*>( &jointDomain->getDomain( 2 ) );

    TS_ASSERT( d1 );
    TS_ASSERT( d2 );
    TS_ASSERT( d3 );
    
    TS_ASSERT_EQUALS(d1->size() , 10);
    TS_ASSERT_EQUALS(d2->size() , 10);
    TS_ASSERT_EQUALS(d3->size() , 10);

    TS_ASSERT_EQUALS(values->size() , 30);
  }


private:
  MatrixWorkspace_sptr ws1,ws2,ws3;
};

#endif /*MULTIDOMAINCREATORTEST_H_*/
