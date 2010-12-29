#ifndef COMPOSITE_FUNCTION_BUILDER_TEST_H_
#define COMPOSITE_FUNCTION_BUILDER_TEST_H_

#include <gmock/gmock.h>
#include <gtest/gtest.h>
#include <cxxtest/TestSuite.h>
#include <vector>
#include <boost/scoped_ptr.hpp>
#include "MantidMDAlgorithms/CompositeFunctionBuilder.h"
#include "MantidMDAlgorithms/CompositeImplicitFunction.h"
#include "MantidAPI/ImplicitFunction.h"
#include "MantidAPI/ImplicitFunctionBuilder.h"
#include "MantidAPI/ImplicitFunctionParameter.h"

class CompositeBuilderTest : public CxxTest::TestSuite
{

private:

    class FakeParameter : public Mantid::API::ImplicitFunctionParameter
    {
    public:
        bool isValid() const
        {
            return false;
        }
		 MOCK_CONST_METHOD0(getName, std::string());
         MOCK_CONST_METHOD0(toXMLString, std::string());
        ~FakeParameter(){;} 

    protected:
        virtual Mantid::API::ImplicitFunctionParameter* clone() const
        {
            return new FakeParameter;
        }
    };	



    class FakeImplicitFunction : public Mantid::API::ImplicitFunction
    {
    public:
        bool evaluate(const Mantid::API::Point3D* pPoint3D) const
        {    
            return false;
        }
		MOCK_CONST_METHOD0(getName, std::string());
		MOCK_CONST_METHOD0(toXMLString, std::string());
    };

    class FakeFunctionBuilder : public Mantid::API::ImplicitFunctionBuilder
    {
    public:
        mutable bool isInvoked;
        void addParameter(std::auto_ptr<Mantid::API::ImplicitFunctionParameter> parameter)
        {
        }
        Mantid::API::ImplicitFunction* create() const
        {
            isInvoked = true;
            return new FakeImplicitFunction;
        }
    };

public:

    void testCreate()
    {

        using namespace Mantid::MDAlgorithms;

        FakeFunctionBuilder* builderA = new FakeFunctionBuilder;
        FakeFunctionBuilder* builderB = new FakeFunctionBuilder;
        CompositeFunctionBuilder* innerCompBuilder = new CompositeFunctionBuilder;
        innerCompBuilder->addFunctionBuilder(builderA);
        innerCompBuilder->addFunctionBuilder(builderB);
        boost::scoped_ptr<CompositeFunctionBuilder> outterCompBuilder(new CompositeFunctionBuilder);
        outterCompBuilder->addFunctionBuilder(innerCompBuilder);
        boost::scoped_ptr<Mantid::API::ImplicitFunction> topFunc(outterCompBuilder->create());
        //CompositeImplicitFunction* topCompFunc = dynamic_cast<CompositeImplicitFunction*>(topFunc.get());

        TSM_ASSERT("Nested builder not called by composite", builderA->isInvoked);
        TSM_ASSERT("Nested builder not called by composite", builderB->isInvoked);
        //TSM_ASSERT("Top level function generated, should have been a composite function instance.", topCompFunc != NULL);

    }

};



#endif
