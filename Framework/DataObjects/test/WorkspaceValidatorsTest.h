// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#ifndef WORKSPACEVALIDATORSTEST_H_
#define WORKSPACEVALIDATORSTEST_H_

#include "MantidAPI/Axis.h"
#include "MantidAPI/CommonBinsValidator.h"
#include "MantidAPI/HistogramValidator.h"
#include "MantidAPI/InstrumentValidator.h"
#include "MantidAPI/OrientedLatticeValidator.h"
#include "MantidAPI/RawCountValidator.h"
#include "MantidAPI/Sample.h"
#include "MantidAPI/SampleValidator.h"
#include "MantidAPI/WorkspaceProperty.h"
#include "MantidAPI/WorkspaceUnitValidator.h"
#include "MantidDataObjects/Workspace2D.h"
#include "MantidGeometry/Crystal/OrientedLattice.h"
#include "MantidKernel/Material.h"
#include "MantidKernel/NeutronAtom.h"
#include "MantidKernel/UnitFactory.h"
#include "MantidTestHelpers/ComponentCreationHelper.h"
#include <cxxtest/TestSuite.h>

using namespace Mantid::Kernel;
using namespace Mantid::API;

class WorkspaceValidatorsTest : public CxxTest::TestSuite {
private:
  WorkspaceUnitValidator *wavUnitVal;
  WorkspaceUnitValidator *anyUnitVal;
  HistogramValidator *histVal;
  RawCountValidator *rawVal;
  RawCountValidator *nonRawVal;
  CommonBinsValidator *binVal;

  MatrixWorkspace_sptr ws1;
  MatrixWorkspace_sptr ws2;

public:
  // This pair of boilerplate methods prevent the suite being created statically
  // This means the constructor isn't called when running other tests
  static WorkspaceValidatorsTest *createSuite() {
    return new WorkspaceValidatorsTest();
  }
  static void destroySuite(WorkspaceValidatorsTest *suite) { delete suite; }

  WorkspaceValidatorsTest() {
    wavUnitVal = new WorkspaceUnitValidator("Wavelength");
    anyUnitVal = new WorkspaceUnitValidator("");
    histVal = new HistogramValidator();
    rawVal = new RawCountValidator();
    nonRawVal = new RawCountValidator(false);
    binVal = new CommonBinsValidator();

    ws1 = MatrixWorkspace_sptr(new Mantid::DataObjects::Workspace2D);
    ws1->initialize(2, 10, 9);

    ws2 = MatrixWorkspace_sptr(new Mantid::DataObjects::Workspace2D);
    ws2->initialize(2, 10, 10);
    ws2->getAxis(0)->unit() = UnitFactory::Instance().create("Wavelength");
    ws2->setDistribution(true);
  }

  ~WorkspaceValidatorsTest() override {
    delete wavUnitVal;
    delete anyUnitVal;
    delete histVal;
    delete rawVal;
    delete nonRawVal;
    delete binVal;
  }

  void testCast() {
    TS_ASSERT(dynamic_cast<IValidator *>(wavUnitVal));
    TS_ASSERT(dynamic_cast<IValidator *>(anyUnitVal));
    TS_ASSERT(dynamic_cast<IValidator *>(histVal));
    TS_ASSERT(dynamic_cast<IValidator *>(rawVal));
    TS_ASSERT(dynamic_cast<IValidator *>(nonRawVal));
    TS_ASSERT(dynamic_cast<IValidator *>(binVal));
  }

  void testWorkspaceUnitValidator() {
    TS_ASSERT_THROWS_NOTHING(WorkspaceUnitValidator());
  }

  void testWorkspaceUnitValidator_getType() {
    TS_ASSERT_EQUALS(wavUnitVal->getType(), "workspaceunit");
    TS_ASSERT_EQUALS(anyUnitVal->getType(), "workspaceunit");
  }

  void testWorkspaceUnitValidator_isValid() {
    TS_ASSERT_EQUALS(wavUnitVal->isValid(ws1),
                     "The workspace must have units of Wavelength");
    TS_ASSERT_EQUALS(wavUnitVal->isValid(ws2), "");
    TS_ASSERT_EQUALS(anyUnitVal->isValid(ws1), "The workspace must have units");
    TS_ASSERT_EQUALS(anyUnitVal->isValid(ws2), "");
  }

  void testHistogramValidator() {
    TS_ASSERT_THROWS_NOTHING(HistogramValidator(false));
  }

  void testHistogramValidator_getType() {
    TS_ASSERT_EQUALS(histVal->getType(), "histogram");
  }

  void testHistogramValidator_isValid() {
    TS_ASSERT_EQUALS(histVal->isValid(ws1), "");
    TS_ASSERT_EQUALS(histVal->isValid(ws2),
                     "The workspace must contain histogram data");
    HistogramValidator reverse(false);
    TS_ASSERT_EQUALS(reverse.isValid(ws1),
                     "The workspace must not contain histogram data");
    TS_ASSERT_EQUALS(reverse.isValid(ws2), "");
  }

  void testRawCountValidator_getType() {
    TS_ASSERT_EQUALS(nonRawVal->getType(), "rawcount");
  }

  void testRawCountValidator_isValid() {
    TS_ASSERT_EQUALS(rawVal->isValid(ws1), "");
    TS_ASSERT_EQUALS(
        rawVal->isValid(ws2),
        "A workspace containing numbers of counts is required here");
    TS_ASSERT_EQUALS(nonRawVal->isValid(ws1),
                     "A workspace of numbers of counts is not allowed here");
    TS_ASSERT_EQUALS(nonRawVal->isValid(ws2), "");
  }

  void testCommonBinsValidator_getType() {
    TS_ASSERT_EQUALS(binVal->getType(), "commonbins");
  }

  void testCommonBinsValidator_isValid() {
    TS_ASSERT_EQUALS(binVal->isValid(ws1), "");
    TS_ASSERT_EQUALS(binVal->isValid(ws2), "");
    ws1->dataX(0)[5] = 1.1;
    TS_ASSERT_EQUALS(
        binVal->isValid(ws1),
        "The workspace must have common bin boundaries for all histograms");
  }

  void testWSPropertyandValidator() {
    auto wavUnitValidator =
        boost::make_shared<WorkspaceUnitValidator>("Wavelength");
    WorkspaceProperty<MatrixWorkspace> wsp1("workspace1", "ws1",
                                            Direction::Input, wavUnitValidator);
    // test property validation
    TS_ASSERT_EQUALS(
        wsp1.isValid(),
        "Workspace \"ws1\" was not found in the Analysis Data Service");
    ;

    TS_ASSERT_EQUALS(wsp1.setValue(""),
                     "Enter a name for the Input/InOut workspace");

    // fine and correct unit
    wsp1 = ws2;
    TS_ASSERT_EQUALS(wsp1.isValid(), "");
    ;

    // fine and no unit
    TS_ASSERT_THROWS(wsp1 = ws1, const std::invalid_argument &);

    TS_ASSERT_EQUALS(wsp1.setValue(""),
                     "Enter a name for the Input/InOut workspace");
    TS_ASSERT_EQUALS(wsp1.isValid(),
                     "Enter a name for the Input/InOut workspace");
  }

  void testInstrumentValidator() {
    { // default validator
      auto ws = boost::make_shared<Mantid::DataObjects::Workspace2D>();
      auto inst = boost::make_shared<Mantid::Geometry::Instrument>();
      auto *sample = new Mantid::Geometry::ObjComponent("Sample");
      inst->add(sample);
      inst->markAsSamplePos(sample);

      auto instVal = boost::make_shared<InstrumentValidator>();
      TS_ASSERT_EQUALS(
          instVal->isValid(ws),
          "The instrument is missing the following components: sample holder");
      ws->setInstrument(inst);
      TS_ASSERT_EQUALS(instVal->isValid(ws), "");
    }

    { // requires just a source
      auto ws = boost::make_shared<Mantid::DataObjects::Workspace2D>();
      auto inst = boost::make_shared<Mantid::Geometry::Instrument>();

      auto instVal = boost::make_shared<InstrumentValidator>(
          InstrumentValidator::SourcePosition);
      TS_ASSERT_EQUALS(
          instVal->isValid(ws),
          "The instrument is missing the following components: source");
      auto *src = new Mantid::Geometry::ObjComponent("Source");
      inst->add(src);
      inst->markAsSource(src);
      ws->setInstrument(inst);
      TS_ASSERT_EQUALS(instVal->isValid(ws), "");
    }

    { // requires source & sample position
      auto ws = boost::make_shared<Mantid::DataObjects::Workspace2D>();
      auto inst = boost::make_shared<Mantid::Geometry::Instrument>();

      auto instVal = boost::make_shared<InstrumentValidator>(
          InstrumentValidator::SourcePosition |
          InstrumentValidator::SamplePosition);
      TS_ASSERT_EQUALS(instVal->isValid(ws), "The instrument is missing the "
                                             "following components: "
                                             "source,sample holder");
      auto *sample = new Mantid::Geometry::ObjComponent("Sample");
      inst->add(sample);
      inst->markAsSamplePos(sample);
      auto *src = new Mantid::Geometry::ObjComponent("Source");
      inst->add(src);
      inst->markAsSource(src);
      ws->setInstrument(inst);
      TS_ASSERT_EQUALS(instVal->isValid(ws), "");
    }
  }

  void testOrientedLatticeValidator() {
    using Mantid::API::OrientedLatticeValidator;
    using Mantid::DataObjects::Workspace2D;
    using Mantid::Geometry::OrientedLattice;
    OrientedLatticeValidator validator;
    auto ws = boost::make_shared<Workspace2D>();
    TS_ASSERT_EQUALS(
        validator.isValid(ws),
        "Workspace must have a sample with an orientation matrix defined.");

    OrientedLattice lattice;
    ws->mutableSample().setOrientedLattice(&lattice);

    TS_ASSERT_EQUALS(validator.isValid(ws), "");
  }

  void testSampleValidator() {
    using Mantid::Geometry::CSGObject;
    using Mantid::Kernel::Material;
    using Mantid::PhysicalConstants::NeutronAtom;
    // These should be separate tests when they are refactored out
    { // requires just shape
      auto ws = boost::make_shared<Mantid::DataObjects::Workspace2D>();
      auto sampleVal =
          boost::make_shared<SampleValidator>(SampleValidator::Shape);

      TS_ASSERT_EQUALS(sampleVal->isValid(ws),
                       "The sample is missing the following properties: shape");
      auto shape = ComponentCreationHelper::createSphere(0.01);
      ws->mutableSample().setShape(shape);
      TS_ASSERT_EQUALS(sampleVal->isValid(ws), "");
    }

    { // requires just material
      auto ws = boost::make_shared<Mantid::DataObjects::Workspace2D>();
      auto sampleVal =
          boost::make_shared<SampleValidator>(SampleValidator::Material);

      TS_ASSERT_EQUALS(
          sampleVal->isValid(ws),
          "The sample is missing the following properties: material");
      auto noShape = boost::make_shared<CSGObject>();
      noShape->setMaterial(Material("V", NeutronAtom(), 0.072));
      ws->mutableSample().setShape(noShape);
      TS_ASSERT_EQUALS(sampleVal->isValid(ws), "");
    }

    { // requires both
      auto ws = boost::make_shared<Mantid::DataObjects::Workspace2D>();
      auto sampleVal = boost::make_shared<SampleValidator>(
          SampleValidator::Shape | SampleValidator::Material);

      TS_ASSERT_EQUALS(
          sampleVal->isValid(ws),
          "The sample is missing the following properties: shape,material");
      auto shape = ComponentCreationHelper::createSphere(0.01);
      shape->setMaterial(Material("V", NeutronAtom(), 0.072));
      ws->mutableSample().setShape(shape);
      TS_ASSERT_EQUALS(sampleVal->isValid(ws), "");
    }
  }
};

#endif /*WORKSPACEVALIDATORSTEST_H_*/
