#ifndef ASSOCIATIVECOMPOSITEFUNCTIONTEST_H_
#define ASSOCIATIVECOMPOSITEFUNCTIONTEST_H_

#include <cxxtest/TestSuite.h>

#include "MantidAPI/AssociativeCompositeFunction.h"
#include "MantidAPI/IFunction1D.h"
#include "MantidAPI/FunctionFactory.h"


using namespace Mantid;
using namespace Mantid::API;


/// Moc implementation of AssociativeCompositeFunction
class AssociativeMoc : public AssociativeCompositeFunction {
public:
    std::string name() const override { return "AssociativeMoc"; }
    bool isAssociative(API::IFunction_sptr f) const override {
        if (boost::dynamic_pointer_cast<ProductFunction>(f)) {
            return true;
        }
        return false;
    }
protected:
    /// overwrite IFunction base class method, which declare function parameters
    void init() override {};
};


class AssociativeCompositeFunctionTest : public CxxTest::TestSuite {

public:

    static AssociativeCompositeFunctionTest *createSuite() {
        return new AssociativeCompositeFunctionTest();
    }

    static void destroySuite(AssociativeCompositeFunctionTest *suite) { delete suite; }

    AssociativeCompositeFunctionTest(){
        initializeFunctions();
    };

    testInitialization() {
        // prod3 is a product within a product. It should become a product with three components
        TS_ASSERT_EQUALS(m_f["prod3"]->asString(), "");
    }

private:
    // Initialize some functions that we'll use in the tests
    void initializeFunctions() {
        // aliases to certain function
        std::map<std::string, std::string> aliases = {
            {"exp", "name=ExpDecay"},
            {"lin", "name=LinearBackground"},
            {"comp", "name=Lorentzian;name=InelasticDiffSphere"},
            {"conv", "(composite=Convolution;name=Resolution,X=(3.0,3.1),Y=(3.2,3.3);"
                             "name=DeltaFunction)"},
            {"prod1", "(name=AssociativeMoc)"},
            {"prod2", "(composite=AssociativeMoc;name=ExpDecay;name=FlatBackground)"},
            {"prod3", "(composite=AssociativeMoc;name=ExpDecay;"
                              "(composite=ProductFunction;name=Gaussian;name=Quadratic))"}
        };
        // Create the function objects
        for (auto& kv : aliases) {
            m_f[kv.first] = FunctionFactory::Instance().createInitialized(kv.second);
        }
    }
    // a map containing aliases to function pointers
    std::map<std::string, std::string> m_f;
};
#endif /*ASSOCIATIVECOMPOSITEFUNCTIONTEST_H_*/

