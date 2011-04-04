#ifndef RUNPARAMTEST_H_
#define RUNPARAMTEST_H_

// ONLY BARE BONES OF POSSIBLE TEST FILE, MAINLY STILL TOBYFITTEST

#include <cxxtest/TestSuite.h>
#include <cmath>
#include <iostream>
#include <boost/scoped_ptr.hpp> 
#include "MantidMDAlgorithms/RunParam.h"

using namespace Mantid::API;
using namespace Mantid::Kernel;
using namespace Mantid::Geometry;



class RunParamTest : public CxxTest::TestSuite
{
private:
  
  boost::shared_ptr<Mantid::MDAlgorithms::RunParam> rParam;
  boost::shared_ptr<Mantid::MDAlgorithms::RunParam> rParam2;


public:
  
  RunParamTest() {};

  // set up a simple RunParam object and test
  void testInit()
  {
    using namespace Mantid::MDAlgorithms;
    rParam = boost::shared_ptr<RunParam> (new RunParam() ) ;
    rParam->setEi(10.);
    TS_ASSERT_EQUALS(rParam->getEi(),10.);
    rParam2 = boost::shared_ptr<RunParam> (new RunParam(
        45., 45., 5., 42.,
        0.5, 10., 7.19, 1.82,
        66.67, 66.67, 13.55314, 50.,
        0., 0., 0., 26.7,
        1, 2.28, 49., 1300.,
        150., 0., 3.87, 3.87,
        3.87, 90., 90., 90.,
        1., 0., 0., 0.,
        1., 0., 0., 0.,
        0., 0., 1., 1.,
        0., -1., 1., 0.,
        10., 14., 18., 1,
        10., 0.5
        ));
    TS_ASSERT_EQUALS(rParam2->getEi(),45.);
    TS_ASSERT_EQUALS(rParam2->getTemp(),10.);


  }

  void testTidyUp()
  {
  }

};

#endif /*RUNPARAMTEST_H_*/
