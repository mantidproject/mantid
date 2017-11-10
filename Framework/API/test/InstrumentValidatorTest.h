#ifndef MANTID_API_INSTRUMENTVALIDATORTEST_H_
#define MANTID_API_INSTRUMENTVALIDATORTEST_H_

#include <cxxtest/TestSuite.h>

#include "MantidAPI/InstrumentValidator.h"
#include "MantidGeometry/Instrument/ObjComponent.h"
#include "MantidTestHelpers/FakeObjects.h"
#include "MantidTestHelpers/ComponentCreationHelper.h"

using namespace Mantid::Kernel;
using Mantid::API::InstrumentValidator;

class InstrumentValidatorTest : public CxxTest::TestSuite {
public:
  // This pair of boilerplate methods prevent the suite being created statically
  // This means the constructor isn't called when running other tests
  static InstrumentValidatorTest *createSuite() {
    return new InstrumentValidatorTest();
  }
  static void destroySuite(InstrumentValidatorTest *suite) { delete suite; }

  void test_success() {
    auto ws = boost::make_shared<WorkspaceTester>();
    auto instr =
        boost::make_shared<Mantid::Geometry::Instrument>("TestInstrument");
    ws->setInstrument(instr);
    // Define a sample as a simple sphere
    auto sample = new Mantid::Geometry::ObjComponent(
        "samplePos",
        ComponentCreationHelper::createSphere(0.1, V3D(0, 0, 0), "1"),
        instr.get());
    instr->setPos(0.0, 0.0, 0.0);
    instr->add(sample);
    instr->markAsSamplePos(sample);

    InstrumentValidator validator;
    TS_ASSERT_EQUALS(validator.checkValidity(ws), "");
  }

  void test_fail() {
    auto ws = boost::make_shared<WorkspaceTester>();
    InstrumentValidator validator;
    TS_ASSERT_EQUALS(
        validator.checkValidity(ws),
        "The instrument is missing the following components: sample holder");
  }
};

#endif /* MANTID_API_INSTRUMENTVALIDATORTEST_H_ */
