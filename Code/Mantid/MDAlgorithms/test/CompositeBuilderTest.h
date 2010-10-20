#ifndef COMPOSITE_FUNCTION_BUILDER_TEST_H_
#define COMPOSITE_FUNCTION_BUILDER_TEST_H_

#include <cxxtest/TestSuite.h>
#include <vector>
#include <memory>
#include "CompositeFunctionBuilder.h"
#include "CompositeImplicitFunction.h"
#include "IImplicitFunction.h"
#include "IFunctionBuilder.h"
#include "IParameter.h"
#include "MDDataObjects/point3D.h"

using namespace Mantid::MDAlgorithms;
using namespace Mantid::MDDataObjects;

class CompositeFunctionBuilderTest : public CxxTest::TestSuite
{
 
 private:
    
	class FakeParameter : public IParameter
    {
	    public:
	        std::string getName() const
			{
			    return "FakeParameter";
			}
			bool isValid() const
			{
			    return false;
			}

			~FakeParameter(){;} 
			
		    protected:
			virtual IParameter* cloneImp() const
			{
			    return new FakeParameter;
			}
	};	
	
	class FakeImplicitFunction : public IImplicitFunction
	{
	  public:
	  bool evaluate(point3D const * const pPoint3D) const
	  {    
	        return false;
	  }
	};
	
	class FakeFunctionBuilder : public IFunctionBuilder
	{
	   public:
	    mutable bool isInvoked;
	    void addParameter(IParameter& parameter)
		{
		}
		std::auto_ptr<IImplicitFunction> create() const
		{
		    isInvoked = true;
		    return std::auto_ptr<IImplicitFunction>(new FakeImplicitFunction);
		}
	};

 public:
	
	void testCreate()
	{
	  FakeFunctionBuilder* builderA = new FakeFunctionBuilder;
	  FakeFunctionBuilder* builderB = new FakeFunctionBuilder;
	  CompositeFunctionBuilder* innerCompBuilder = new CompositeFunctionBuilder;
	  innerCompBuilder->addFunctionBuilder(builderA);
	  innerCompBuilder->addFunctionBuilder(builderB);
	  std::auto_ptr<CompositeFunctionBuilder> outterCompBuilder = std::auto_ptr<CompositeFunctionBuilder>(new CompositeFunctionBuilder);
	  outterCompBuilder->addFunctionBuilder(innerCompBuilder);
	  std::auto_ptr<IImplicitFunction> topFunc = outterCompBuilder->create();
	  //CompositeImplicitFunction* topCompFunc = dynamic_cast<CompositeImplicitFunction*>(topFunc.get());
	  
	  TSM_ASSERT("Nested builder not called by composite", builderA->isInvoked);
	  TSM_ASSERT("Nested builder not called by composite", builderB->isInvoked);
	  //TSM_ASSERT("Top level function generated, should have been a composite function instance.", topCompFunc != NULL);
	  
	}
	
	void testAddParameter()
	{
	  FakeParameter param;
	  CompositeFunctionBuilder compFuncBuilder; 
	  TSM_ASSERT_THROWS("Should have thrown invalid_argument exception as no parameters allowed on composite function builders.", compFuncBuilder.addParameter(param), std::invalid_argument);
	}
	
};

#endif