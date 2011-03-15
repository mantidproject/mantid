#ifndef TIMESTEP_TO_TIMESTEP_TEST_H_
#define TIMESTEP_TO_TIMESTEP_TEST_H_

#include <cxxtest/TestSuite.h>
#include <vtkRectilinearGrid.h>
#include "MantidVatesAPI/TimeStepToTimeStep.h"

class TimeStepToTimeStepTest: public CxxTest::TestSuite
{
public:

  void testArgumentEqualsProduct()
  {
    //Check that this type works as a compile-time proxy. Should do nothing with argument other than return it.
    Mantid::VATES::TimeStepToTimeStep proxy;
    double argument = 1;
    TSM_ASSERT_EQUALS("The TimeStepToTimeStep proxy should return its own argument", argument, proxy(argument));
  }

};

#endif
