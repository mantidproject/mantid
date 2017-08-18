#ifndef ASSOCIATIVECOMPOSITEFUNCTIONTEST_H_
#define ASSOCIATIVECOMPOSITEFUNCTIONTEST_H_

#include <string>
#include <cxxtest/TestSuite.h>
#include <boost/shared_array.hpp>

#include "MantidAPI/IFunction.h"
#include "MantidAPI/AssociativeCompositeFunction.h"
#include "MantidAPI/IFunction1D.h"
#include "MantidAPI/FunctionFactory.h"
#include "MantidTestHelpers/FunctionTestHelper.h"

using namespace Mantid;
using namespace Mantid::API;


/// Moc implementation of AssociativeCompositeFunction
class AssociativeMoc : public AssociativeCompositeFunction {

public:

    std::string name() const override { return "AssociativeMoc"; }

    bool isAssociative(API::IFunction_sptr f) const override {
        if (boost::dynamic_pointer_cast<AssociativeMoc>(f)) {
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

    /*
     * Boilerplate
     */
    static AssociativeCompositeFunctionTest *createSuite() {
        return new AssociativeCompositeFunctionTest();
    }

    static void destroySuite(AssociativeCompositeFunctionTest *suite) { delete suite; }

    /// Set up
    AssociativeCompositeFunctionTest() : m_f() {
        FunctionFactory::Instance().subscribe<Gauss>("Gauss");
        FunctionFactory::Instance().subscribe<Linear>("Linear");
        FunctionFactory::Instance().subscribe<Cubic>("Cubic");
        FunctionFactory::Instance().subscribe<AssociativeMoc>("AssociativeMoc");
        initializeMocFunctions();
    }

    /// Tear down
    ~AssociativeCompositeFunctionTest() override {
        FunctionFactory::Instance().unsubscribe("Gauss");
        FunctionFactory::Instance().unsubscribe("Linear");
        FunctionFactory::Instance().unsubscribe("Cubic");
        FunctionFactory::Instance().unsubscribe("AssociativeMoc");
    }

    /**
     * Tests begin here
     */

    void testInitialization() {
        std::string s("composite=AssociativeMoc,NumDeriv=false;" \
                      "name=Linear,a=0,b=0;" \
                      "name=Gauss,c=0,h=1,s=1;" \
                      "name=Cubic,c0=0,c1=0,c2=0,c3=0");
        TS_ASSERT_EQUALS(m_f["moc4"]->asString(), s);
        s.assign("composite=AssociativeMoc,NumDeriv=false;" \
                 "name=Linear,a=0,b=0;" \
                 "name=Gauss,c=0,h=1,s=1;" \
                 "name=Cubic,c0=0,c1=0,c2=0,c3=0;" \
                 "name=Linear,a=0,b=0");
        TS_ASSERT_EQUALS(m_f["moc5"]->asString(), s);
    }

    void testAddFunction() {
        auto f = functionAssociativeInitialized("(name=AssociativeMoc)");
        f->addFunction(m_f["line"]);
        TS_ASSERT_EQUALS(f->asString(), "composite=AssociativeMoc,NumDeriv=false;name=Linear,a=0,b=0");
        f->addFunction(m_f["comp"]);
        // "comp" does not unroll, it's not associative
        std::string s("composite=AssociativeMoc,NumDeriv=false;" \
                      "name=Linear,a=0,b=0;" \
                      "(name=Gauss,c=0,h=1,s=1;name=Cubic,c0=0,c1=0,c2=0,c3=0)");
        TS_ASSERT_EQUALS(f->asString(), s);
        f->addFunction(m_f["moc2"]);
        s.append(";name=Linear,a=0,b=0");
        TS_ASSERT_EQUALS(f->asString(), s);
        f->addFunction(m_f["moc3"]);
        s.append(";name=Gauss,c=0,h=1,s=1;" \
                 "name=Cubic,c0=0,c1=0,c2=0,c3=0");  // moc3 is unrolled
        TS_ASSERT_EQUALS(f->asString(), s);
    }

    void testInsertFunction() {
        std::string s("(composite=AssociativeMoc;name=Gauss;name=Cubic)");
        auto f = functionAssociativeInitialized(s);
        f->insertFunction(1, m_f["line"]);  // insert Linear before Cubic
        s.assign("composite=AssociativeMoc,NumDeriv=false;" \
                 "name=Gauss,c=0,h=1,s=1;" \
                 "name=Linear,a=0,b=0;" \
                 "name=Cubic,c0=0,c1=0,c2=0,c3=0");
        TS_ASSERT_EQUALS(f->asString(), s);
        f->insertFunction(2, m_f["moc2"]);  // insert moc2 (Linear) before Cubic
        s.assign("composite=AssociativeMoc,NumDeriv=false;" \
                 "name=Gauss,c=0,h=1,s=1;" \
                 "name=Linear,a=0,b=0;" \
                 "name=Linear,a=0,b=0;" \
                 "name=Cubic,c0=0,c1=0,c2=0,c3=0");
        TS_ASSERT_EQUALS(f->asString(), s);
        f->insertFunction(3, m_f["moc3"]);  // insert moc3 (Gauss * Cubic) before Cubic
        s.assign("composite=AssociativeMoc,NumDeriv=false;" \
                 "name=Gauss,c=0,h=1,s=1;" \
                 "name=Linear,a=0,b=0;" \
                 "name=Linear,a=0,b=0;" \
                 "name=Gauss,c=0,h=1,s=1;" \
                 "name=Cubic,c0=0,c1=0,c2=0,c3=0;" \
                 "name=Cubic,c0=0,c1=0,c2=0,c3=0");
        TS_ASSERT_EQUALS(f->asString(), s);
    }

    void testReplaceFunction() {
        std::string s("(composite=AssociativeMoc;name=Gauss;name=Cubic)");
        auto f = functionAssociativeInitialized(s);
        f->replaceFunction(1, m_f["line"]);  // Replace Cubic with Linear
        s.assign("composite=AssociativeMoc,NumDeriv=false;" \
                 "name=Gauss,c=0,h=1,s=1;" \
                 "name=Linear,a=0,b=0");
        TS_ASSERT_EQUALS(f->asString(), s);
        f->replaceFunction(0, m_f["moc2"]);  // Replace Gauss with moc2 (Linear)
        s.assign("composite=AssociativeMoc,NumDeriv=false;" \
                  "name=Linear,a=0,b=0;" \
                  "name=Linear,a=0,b=0");
        TS_ASSERT_EQUALS(f->asString(), s);
        f->insertFunction(1, m_f["moc3"]);  // Replace second Linear with moc3 (Gauss * Cubic)
        s.assign("composite=AssociativeMoc,NumDeriv=false;" \
                 "name=Linear,a=0,b=0;" \
                 "name=Gauss,c=0,h=1,s=1;" \
                 "name=Cubic,c0=0,c1=0,c2=0,c3=0;" \
                 "name=Linear,a=0,b=0");
        TS_ASSERT_EQUALS(f->asString(), s);
    }

private:
    // Initialize some functions that we'll use in the tests
    void initializeMocFunctions() {
        // aliases to certain functions
        std::map<std::string, std::string> aliases = {
            {"line", "name=Linear"},
            {"comp", "name=Gauss;name=Cubic"},
            {"moc1", "(name=AssociativeMoc)"},
            {"moc2", "(composite=AssociativeMoc;name=Linear)"},
            // moc3 = Gauss * Linear
            {"moc3", "(composite=AssociativeMoc;name=Gauss;name=Cubic)"},
            // moc4 = Gauss * (Linear * Cubic)
            {"moc4", "(composite=AssociativeMoc;name=Linear;"
                        "(composite=AssociativeMoc;name=Gauss;name=Cubic))"},
            // moc5 = (Gauss * Linear) * (Cubic * Gauss)
            {"moc5", "(composite=AssociativeMoc;" \
                     "(composite=AssociativeMoc;name=Linear;name=Gauss);" \
                     "(composite=AssociativeMoc;name=Cubic;name=Linear))"},
        };
        for (auto& kv : aliases) {
            //m_f[kv.first] = FunctionFactory::Instance().createInitialized(kv.second);
            m_f.insert(std::pair<std::string, IFunction_sptr>(kv.first,
               FunctionFactory::Instance().createInitialized(kv.second)));
        }
    }

    boost::shared_ptr<AssociativeMoc> functionAssociativeInitialized(const std::string &s) {
        auto f = FunctionFactory::Instance().createInitialized(s);
        return boost::dynamic_pointer_cast<AssociativeMoc>(f);
    }

    // a map containing aliases to function pointers
    std::map<std::string, IFunction_sptr> m_f;

};  /* class AssociativeCompositeFunctionTest */

#endif /*ASSOCIATIVECOMPOSITEFUNCTIONTEST_H_*/

