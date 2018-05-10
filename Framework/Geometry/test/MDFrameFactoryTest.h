#ifndef MANTID_KERNEL_MDFRAMEFACTORYTEST_H_
#define MANTID_KERNEL_MDFRAMEFACTORYTEST_H_

#include <cxxtest/TestSuite.h>

#include "MantidGeometry/MDGeometry/GeneralFrame.h"
#include "MantidGeometry/MDGeometry/HKL.h"
#include "MantidGeometry/MDGeometry/MDFrame.h"
#include "MantidGeometry/MDGeometry/MDFrameFactory.h"
#include "MantidGeometry/MDGeometry/QLab.h"
#include "MantidGeometry/MDGeometry/QSample.h"
#include "MantidGeometry/MDGeometry/UnknownFrame.h"
#include "MantidKernel/MDUnit.h"
#include "MantidKernel/UnitLabelTypes.h"

using namespace Mantid::Geometry;
using namespace Mantid::Kernel;

class MDFrameFactoryTest : public CxxTest::TestSuite {
public:
  // This pair of boilerplate methods prevent the suite being created statically
  // This means the constructor isn't called when running other tests
  static MDFrameFactoryTest *createSuite() { return new MDFrameFactoryTest(); }
  static void destroySuite(MDFrameFactoryTest *suite) { delete suite; }

  void test_GeneralFrameFactory_all_general() {
    GeneralFrameFactory factory;
    const MDFrameArgument argument("any_frame", "any_unit");
    TS_ASSERT(factory.canInterpret(argument));

    std::unique_ptr<MDFrame> product = factory.create(argument);
    TSM_ASSERT("Should be creating a GeneralFrame",
               dynamic_cast<GeneralFrame *>(product.get()));

    const auto &heldMDUnit = product->getMDUnit();
    TSM_ASSERT("Expect that we have a label unit",
               dynamic_cast<const Mantid::Kernel::LabelUnit *>(&heldMDUnit));
  }

  void test_GeneralFrameFactory_known_units() {

    GeneralFrameFactory factory;

    const MDFrameArgument argument("any_frame", Units::Symbol::InverseAngstrom);

    std::unique_ptr<MDFrame> product = factory.create(argument);
    TSM_ASSERT("Should be creating a GeneralFrame",
               dynamic_cast<GeneralFrame *>(product.get()));

    const auto &heldMDUnit = product->getMDUnit();
    TSM_ASSERT("Expect that we have a label unit",
               dynamic_cast<const Mantid::Kernel::InverseAngstromsUnit *>(
                   &heldMDUnit));
  }

  void test_QLabFrameFactory() {
    QLabFrameFactory factory;

    const MDFrameArgument badArgument("any_frame");
    TSM_ASSERT("Should NOT offer to produce QLab products for this frame key",
               !factory.canInterpret(badArgument));

    const MDFrameArgument argument = MDFrameArgument(QLab::QLabName);
    TSM_ASSERT("Should offer to produce QLab products for this frame key",
               factory.canInterpret(argument));

    std::unique_ptr<MDFrame> product = factory.create(argument);
    TSM_ASSERT("Should be creating a QLab frame",
               dynamic_cast<Mantid::Geometry::QLab *>(product.get()));
  }

  void test_QSampleFrameFactory() {
    QSampleFrameFactory factory;

    const MDFrameArgument badArgument("any_frame");
    TSM_ASSERT(
        "Should NOT offer to produce QSample products for this frame key",
        !factory.canInterpret(badArgument));

    const MDFrameArgument argument = MDFrameArgument(QSample::QSampleName);
    TSM_ASSERT("Should offer to produce QSample products for this frame key",
               factory.canInterpret(argument));

    std::unique_ptr<MDFrame> product = factory.create(argument);
    TSM_ASSERT("Should be creating a QSample frame",
               dynamic_cast<Mantid::Geometry::QSample *>(product.get()));
  }

  void test_HKLFrameFactory_interpretation() {
    HKLFrameFactory factory;

    TSM_ASSERT("Should NOT offer to produce HKL products for this frame key",
               !factory.canInterpret(MDFrameArgument("any_frame")));

    TSM_ASSERT(
        "Should NOT offer to produce HKL products as units are incompatible",
        !factory.canInterpret(
            MDFrameArgument(HKL::HKLName, Units::Symbol::Metre)));

    TSM_ASSERT("Should offer to produce HKL products",
               factory.canInterpret(MDFrameArgument(
                   HKL::HKLName, Units::Symbol::InverseAngstrom)));

    TSM_ASSERT("Should offer to produce HKL products",
               factory.canInterpret(
                   MDFrameArgument(HKL::HKLName, Units::Symbol::RLU)));

    TSM_ASSERT(
        "Should offer to produce HKL products",
        factory.canInterpret(MDFrameArgument(HKL::HKLName, "in 1.684 A^-1")));
  }

  void test_HKLFrameFactory_create_inverse_angstroms() {
    HKLFrameFactory factory;

    std::unique_ptr<MDFrame> product = factory.create(
        MDFrameArgument(HKL::HKLName, Units::Symbol::InverseAngstrom));

    TSM_ASSERT("Should be creating a HKL frame, in inverse angstroms",
               dynamic_cast<Mantid::Geometry::HKL *>(product.get()));

    TSM_ASSERT_EQUALS("Units carried across incorrectly",
                      Units::Symbol::InverseAngstrom, product->getUnitLabel());
  }

  void test_HKLFrameFactory_create_rlu() {
    HKLFrameFactory factory;

    std::unique_ptr<MDFrame> product =
        factory.create(MDFrameArgument(HKL::HKLName, Units::Symbol::RLU));
    TSM_ASSERT("Should be creating a HKL frame, in rlu",
               dynamic_cast<Mantid::Geometry::HKL *>(product.get()));

    TSM_ASSERT_EQUALS("Units carried across incorrectly", Units::Symbol::RLU,
                      product->getUnitLabel());
  }

  void test_make_standard_chain() {
    MDFrameFactory_uptr chain = makeMDFrameFactoryChain();
    // Now lets try the chain of factories out
    TS_ASSERT(dynamic_cast<UnknownFrame *>(
        chain->create(MDFrameArgument(UnknownFrame::UnknownFrameName)).get()));
    TS_ASSERT(dynamic_cast<GeneralFrame *>(
        chain->create(MDFrameArgument("any_frame")).get()));
    TS_ASSERT(dynamic_cast<Mantid::Geometry::QLab *>(
        chain->create(MDFrameArgument(QLab::QLabName)).get()));
    TS_ASSERT(dynamic_cast<Mantid::Geometry::QSample *>(
        chain->create(MDFrameArgument(QSample::QSampleName)).get()));
    TS_ASSERT(dynamic_cast<Mantid::Geometry::HKL *>(
        chain->create(MDFrameArgument(HKL::HKLName, Units::Symbol::RLU))
            .get()));
  }
};

#endif /* MANTID_KERNEL_MDFRAMEFACTORYTEST_H_ */
