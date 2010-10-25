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

class CompositeFunctionBuilderTest : public CxxTest::TestSuite
{

private:

    class FakeParameter : public Mantid::MDAlgorithms::IParameter
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
        std::string toXML() const
        {
            return "";
        }
        ~FakeParameter(){;} 

    protected:
        virtual Mantid::MDAlgorithms::IParameter* cloneImp() const
        {
            return new FakeParameter;
        }
    };	

    class FakeImplicitFunction : Mantid::API::IImplicitFunction
    {
    public:
        bool evaluate(Mantid::MDDataObjects::point3D const * const pPoint3D) const
        {    
            return false;
        }
    };

    class FakeFunctionBuilder : public Mantid::MDAlgorithms::IFunctionBuilder
    {
    public:
        mutable bool isInvoked;
        void addParameter(std::auto_ptr<Mantid::MDAlgorithms::IParameter> parameter)
        {
        }
        std::auto_ptr<Mantid::API::IImplicitFunction> create() const
        {
            isInvoked = true;
            return std::auto_ptr<Mantid::API::IImplicitFunction>(new FakeImplicitFunction);
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
        std::auto_ptr<CompositeFunctionBuilder> outterCompBuilder = std::auto_ptr<CompositeFunctionBuilder>(new CompositeFunctionBuilder);
        outterCompBuilder->addFunctionBuilder(innerCompBuilder);
        std::auto_ptr<Mantid::API::IImplicitFunction> topFunc = outterCompBuilder->create();
        //CompositeImplicitFunction* topCompFunc = dynamic_cast<CompositeImplicitFunction*>(topFunc.get());

        TSM_ASSERT("Nested builder not called by composite", builderA->isInvoked);
        TSM_ASSERT("Nested builder not called by composite", builderB->isInvoked);
        //TSM_ASSERT("Top level function generated, should have been a composite function instance.", topCompFunc != NULL);

    }

};



#endif