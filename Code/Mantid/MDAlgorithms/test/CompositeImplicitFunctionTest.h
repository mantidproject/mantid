#ifndef COMPOSITE_IMPLICIT_FUNCTION_TEST_H_
#define COMPOSITE_IMPLICIT_FUNCTION_TEST_H_

#include <cxxtest/TestSuite.h>
#include <cmath>


#include "CompositeImplicitFunction.h"
#include "Hexahedron.h"

using namespace Mantid::MDAlgorithms;
using namespace Mantid::MDDataObjects;

class CompositeImplicitFunctionTest : public CxxTest::TestSuite
{
private:

	//Fake ImplicitFunction to verify abstract treatement of nested functions by composite.
	class FakeIImplicitFunction : public IImplicitFunction
	{
	private:
		int  m_evaluateCount;
		bool m_setOutput;
	public:
		FakeIImplicitFunction(bool setOutput=false) : m_setOutput(setOutput), m_evaluateCount(0)
		{
		}
		bool Evaluate(Hexahedron* hexahedron)
		{
			m_evaluateCount += 1;
			return m_setOutput;
		}
		int getEvaluateCount()
		{
			return m_evaluateCount;
		}
		~FakeIImplicitFunction(){}
	};

	//Fake CompositeImplicitFunction. Minimal decoration to expose the number of contained functions for testability purposes.
	class FakeCompositeImplicitFunction : public CompositeImplicitFunction
	{
	public:
		int getFunctionsCount()
		{
			return this->m_Functions.size();
		}
	};
  
public:

  void testFunctionAddition()
  {

	  FakeIImplicitFunction* a = new FakeIImplicitFunction();
	  FakeIImplicitFunction* b = new FakeIImplicitFunction();

	  FakeCompositeImplicitFunction composite;
	  composite.AddFunction(a);
	  composite.AddFunction(b);
	  TSM_ASSERT_EQUALS("Two functions should have been added to composite", 2, composite.getFunctionsCount());
  }


  void testEvaluateCount()
  {
	  CompositeImplicitFunction composite;
	  bool dummyOutCome = true;
	  FakeIImplicitFunction* a = new FakeIImplicitFunction(dummyOutCome);
	  FakeIImplicitFunction* b = new FakeIImplicitFunction(dummyOutCome);
	  composite.AddFunction(a);
	  composite.AddFunction(b);
	  composite.Evaluate(new Mantid::MDDataObjects::Hexahedron());

	  int callResult = a->getEvaluateCount() + b->getEvaluateCount();
	  TSM_ASSERT_EQUALS("Two Functions should have been executed",2, callResult);
  }

  void testAbortEvaluation()
  {
	  CompositeImplicitFunction composite;
	  FakeIImplicitFunction* a = new FakeIImplicitFunction(false);
	  FakeIImplicitFunction* b = new FakeIImplicitFunction(false);
	  composite.AddFunction(a);
	  composite.AddFunction(b);
	  composite.Evaluate(new Mantid::MDDataObjects::Hexahedron());

	  int callResult = a->getEvaluateCount() + b->getEvaluateCount();
	  TSM_ASSERT_EQUALS("Should have aborted after first function evaluation", 1, callResult);
  }


};

#endif 
