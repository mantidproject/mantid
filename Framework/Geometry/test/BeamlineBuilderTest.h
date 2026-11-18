// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2026 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include <cxxtest/TestSuite.h>

#include "MantidFrameworkTestHelpers/ComponentCreationHelper.h"
#include "MantidGeometry/Instrument.h"
#include "MantidGeometry/Instrument/BeamlineBuilder.h"
#include "MantidGeometry/Instrument/CompAssembly.h"
#include "MantidGeometry/Instrument/ComponentInfo.h"
#include "MantidGeometry/Instrument/Detector.h"
#include "MantidGeometry/Instrument/DetectorInfo.h"
#include "MantidGeometry/Instrument/GridDetector.h"
#include "MantidGeometry/Instrument/InstrumentMetadata.h"
#include "MantidGeometry/Instrument/InstrumentVisitor.h"
#include "MantidGeometry/Instrument/ObjComponent.h"
#include "MantidGeometry/Instrument/Parameter.h"
#include "MantidGeometry/Instrument/ParameterFactory.h"
#include "MantidGeometry/Instrument/RectangularDetector.h"
#include "MantidGeometry/Instrument/ReferenceFrame.h"
#include "MantidGeometry/Instrument/StructuredDetector.h"
#include "MantidGeometry/Instrument/XMLInstrumentParameter.h"
#include "MantidGeometry/Objects/BoundingBox.h"
#include "MantidGeometry/Objects/IObject.h"
#include "MantidKernel/Exception.h"
#include "MantidKernel/Interpolation.h"

using namespace Mantid::Geometry;
using Mantid::detid_t;
using Mantid::Kernel::Quat;
using Mantid::Kernel::V2D;
using Mantid::Kernel::V3D;
using Node = BeamlineBuilder::Node;

namespace {
/// Positions, rotations and side-by-side view positions that make the composition non-trivial.
struct Placement {
  V3D instrumentPos{0, 0, 1};
  Quat instrumentRot{90, V3D(0, 0, 1)};
  V3D sourcePos{0, 0, -10};
  V3D monitorPos{0, 0, -1};
  V3D bankPos{1, 0, 0};
  Quat bankRot{45, V3D(0, 1, 0)};
  V2D bankSideBySide{1, 2};
  V2D pixelSideBySide{0.5, 0.25};
  std::vector<std::pair<detid_t, V3D>> pixels{{7, V3D(0, 0, 0)}, {5, V3D(0, 0.1, 0)}, {6, V3D(0, 0.2, 0)}};
};

/// A legacy tree: a source, a sample, a monitor, a shapeless ObjComponent and a bank of pixels whose IDs are not
/// in tree order.
Instrument_sptr makeLegacyInstrument(Placement const &placement) {
  auto const instrument = std::make_shared<Instrument>("inst");
  instrument->setPos(placement.instrumentPos);
  instrument->setRot(placement.instrumentRot);
  auto *const source = new ObjComponent("source", ComponentCreationHelper::createSphere(0.01));
  source->setPos(placement.sourcePos);
  instrument->add(source);
  instrument->markAsSource(source);
  auto *const sample = new Component("sample");
  instrument->add(sample);
  instrument->markAsSamplePos(sample);
  auto *const monitor = new Detector("monitor", 1, ComponentCreationHelper::createSphere(0.01), nullptr);
  monitor->setPos(placement.monitorPos);
  instrument->add(monitor);
  instrument->markAsMonitor(monitor);
  instrument->add(new ObjComponent("slit"));
  auto *const bank = new CompAssembly("bank");
  bank->setPos(placement.bankPos);
  bank->setRot(placement.bankRot);
  bank->setSideBySideViewPos(placement.bankSideBySide);
  instrument->add(bank);
  for (auto const &[id, position] : placement.pixels) {
    auto *const pixel =
        new Detector("pixel" + std::to_string(id), id, ComponentCreationHelper::createSphere(0.01), nullptr);
    pixel->setPos(position);
    if (id == 5) {
      pixel->setSideBySideViewPos(placement.pixelSideBySide);
    }
    bank->add(pixel);
    instrument->markAsDetector(pixel);
  }
  return instrument;
}

/// The same instrument as makeLegacyInstrument, through the builder.
BeamlineBuilder makeBuilder(Placement const &placement) {
  BeamlineBuilder builder;
  auto const root =
      builder.addAssembly(BeamlineBuilder::noParent, "inst", placement.instrumentPos, placement.instrumentRot);
  builder.setSource(builder.addObjComponent(root, "source", placement.sourcePos, Quat(),
                                            ComponentCreationHelper::createSphere(0.01)));
  builder.setSample(builder.addComponent(root, "sample"));
  builder.addDetector(root, "monitor", 1, placement.monitorPos, Quat(), ComponentCreationHelper::createSphere(0.01),
                      true);
  builder.addObjComponent(root, "slit", V3D(), Quat(), nullptr);
  auto const bank = builder.addAssembly(root, "bank", placement.bankPos, placement.bankRot);
  builder.setSideBySideViewPosition(bank, placement.bankSideBySide);
  for (auto const &[id, position] : placement.pixels) {
    auto const pixel = builder.addDetector(bank, "pixel" + std::to_string(id), id, position, Quat(),
                                           ComponentCreationHelper::createSphere(0.01));
    if (id == 5) {
      builder.setSideBySideViewPosition(pixel, placement.pixelSideBySide);
    }
  }
  return builder;
}

/// The beamline of a legacy tree, as InstrumentVisitor makes it.
BeamlineRef walk(Instrument_sptr const &instrument) {
  InstrumentVisitor visitor(instrument);
  visitor.walkInstrument();
  auto [componentInfo, detectorInfo] = visitor.makeWrappers();
  return makeBeamlineRef(std::move(componentInfo), std::move(detectorInfo));
}

/// A legacy tree holding a 3D grid bank and a structured bank, as InstrumentDefinitionParser makes them.
Instrument_sptr makeBankInstrument() {
  auto const instrument = std::make_shared<Instrument>("banks");
  instrument->setReferenceFrame(std::make_shared<ReferenceFrame>(Y, Z, Right, ""));
  auto *const grid = new GridDetector("grid");
  grid->setPos(V3D(0, 0, 2));
  instrument->add(grid);
  grid->initialize(ComponentCreationHelper::createCuboid(0.5), 2, -1.0, 1.0, 3, -1.5, 1.0, 2, -1.0, 1.0, 100, "zyx", 6,
                   1);
  auto *const structured = new StructuredDetector("structured");
  structured->setPos(V3D(1, 0, 2));
  instrument->add(structured);
  structured->initialize(2, 2, {0, 1, 2, 0, 1, 2, 0, 1, 2}, {0, 0, 0, 1, 1, 1, 2, 2, 2}, true, 200, true, 2, 1);
  std::vector<IDetector_const_sptr> detectors;
  for (auto const *bank : std::initializer_list<ICompAssembly const *>{grid, structured}) {
    std::vector<IComponent_const_sptr> children;
    bank->getChildren(children, true);
    for (auto const &child : children) {
      if (auto const detector = std::dynamic_pointer_cast<IDetector const>(child)) {
        instrument->markAsDetectorIncomplete(detector.get());
      }
    }
  }
  instrument->markAsDetectorFinalize();
  return instrument;
}

/// A column holding one pixel, as a bank's grid of pixels needs.
void addPixelColumn(BeamlineBuilder &builder, Node const bank, detid_t const id) {
  auto const column = builder.addAssembly(bank, "column" + std::to_string(id));
  builder.addDetector(column, "pixel" + std::to_string(id), id, V3D(), Quat(), nullptr);
}

/// Asserts that two beamlines hold the same instrument, field by field.
void assertSameBeamline(ComponentInfo const &componentInfo, DetectorInfo const &detectorInfo,
                        ComponentInfo const &expectedComponentInfo, DetectorInfo const &expectedDetectorInfo) {
  TS_ASSERT_EQUALS(componentInfo.size(), expectedComponentInfo.size());
  TS_ASSERT_EQUALS(componentInfo.root(), expectedComponentInfo.root());
  TS_ASSERT_EQUALS(componentInfo.hasSource(), expectedComponentInfo.hasSource());
  TS_ASSERT_EQUALS(componentInfo.hasSample(), expectedComponentInfo.hasSample());
  if (componentInfo.hasSource() && expectedComponentInfo.hasSource()) {
    TS_ASSERT_EQUALS(componentInfo.source(), expectedComponentInfo.source());
  }
  if (componentInfo.hasSample() && expectedComponentInfo.hasSample()) {
    TS_ASSERT_EQUALS(componentInfo.sample(), expectedComponentInfo.sample());
  }
  for (size_t i = 0; i < componentInfo.size(); ++i) {
    TS_ASSERT_EQUALS(componentInfo.name(i), expectedComponentInfo.name(i));
    TS_ASSERT_EQUALS(componentInfo.position(i), expectedComponentInfo.position(i));
    TS_ASSERT_EQUALS(componentInfo.rotation(i), expectedComponentInfo.rotation(i));
    TS_ASSERT_EQUALS(componentInfo.scaleFactor(i), expectedComponentInfo.scaleFactor(i));
    TS_ASSERT_EQUALS(componentInfo.sideBySideViewPosition(i), expectedComponentInfo.sideBySideViewPosition(i));
    TS_ASSERT_EQUALS(componentInfo.componentType(i), expectedComponentInfo.componentType(i));
    TS_ASSERT_EQUALS(componentInfo.hasValidShape(i), expectedComponentInfo.hasValidShape(i));
    if (componentInfo.hasValidShape(i) && expectedComponentInfo.hasValidShape(i)) {
      TS_ASSERT_EQUALS(componentInfo.shape(i).getBoundingBox().width(),
                       expectedComponentInfo.shape(i).getBoundingBox().width());
    }
    TS_ASSERT_EQUALS(componentInfo.hasParent(i), expectedComponentInfo.hasParent(i));
    if (componentInfo.hasParent(i)) {
      TS_ASSERT_EQUALS(componentInfo.parent(i), expectedComponentInfo.parent(i));
    }
    TS_ASSERT_EQUALS(componentInfo.children(i), expectedComponentInfo.children(i));
    TS_ASSERT_EQUALS(componentInfo.detectorsInSubtree(i), expectedComponentInfo.detectorsInSubtree(i));
    TS_ASSERT_EQUALS(componentInfo.componentsInSubtree(i), expectedComponentInfo.componentsInSubtree(i));
    if (componentInfo.isGridDetector(i)) {
      auto const grid = componentInfo.pixelGridComponent(i);
      auto const expectedGrid = expectedComponentInfo.pixelGridComponent(i);
      TS_ASSERT_EQUALS(grid.nX, expectedGrid.nX);
      TS_ASSERT_EQUALS(grid.nZ, expectedGrid.nZ);
      TS_ASSERT_EQUALS(grid.idStart, expectedGrid.idStart);
      TS_ASSERT_EQUALS(grid.idFillOrder, expectedGrid.idFillOrder);
    }
    if (componentInfo.isStructuredDetector(i)) {
      TS_ASSERT_EQUALS(componentInfo.structuredGridComponent(i).xVertices,
                       expectedComponentInfo.structuredGridComponent(i).xVertices);
    }
  }
  TS_ASSERT_EQUALS(detectorInfo.detectorIDs(), expectedDetectorInfo.detectorIDs());
  for (size_t i = 0; i < detectorInfo.size(); ++i) {
    TS_ASSERT_EQUALS(detectorInfo.isMonitor(i), expectedDetectorInfo.isMonitor(i));
    TS_ASSERT_EQUALS(detectorInfo.position(i), expectedDetectorInfo.position(i));
    TS_ASSERT_EQUALS(detectorInfo.rotation(i), expectedDetectorInfo.rotation(i));
  }
}

void assertSameBeamline(BeamlineRef const &beamline, BeamlineRef const &expected) {
  assertSameBeamline(*beamline.componentInfo, *beamline.detectorInfo, *expected.componentInfo, *expected.detectorInfo);
}

} // namespace

class BeamlineBuilderTest : public CxxTest::TestSuite {
public:
  static BeamlineBuilderTest *createSuite() { return new BeamlineBuilderTest(); }
  static void destroySuite(BeamlineBuilderTest *suite) { delete suite; }

  void test_matches_the_visitor_walking_the_same_tree() {
    Placement const placement;
    InstrumentVisitor visitor(makeLegacyInstrument(placement));
    visitor.walkInstrument();
    auto const [expectedComponentInfo, expectedDetectorInfo] = visitor.makeWrappers();
    auto const result = makeBuilder(placement).build();
    assertSameBeamline(*result.componentInfo, *result.detectorInfo, *expectedComponentInfo, *expectedDetectorInfo);
  }

  void test_detectors_come_first_in_id_order_and_the_root_last() {
    auto const result = makeBuilder(Placement()).build();
    auto const &componentInfo = *result.componentInfo;
    TS_ASSERT_EQUALS(result.detectorInfo->detectorIDs(), (std::vector<detid_t>{1, 5, 6, 7}));
    TS_ASSERT_EQUALS(componentInfo.name(1), "pixel5");
    TS_ASSERT_EQUALS(componentInfo.root(), componentInfo.size() - 1);
    TS_ASSERT_EQUALS(componentInfo.name(componentInfo.root()), "inst");
    TS_ASSERT(result.detectorInfo->isMonitor(0));
  }

  void test_positions_are_composed_from_parents() {
    BeamlineBuilder builder;
    auto const root = builder.addAssembly(BeamlineBuilder::noParent, "root", V3D(1, 0, 0), Quat(90, V3D(0, 0, 1)));
    builder.addDetector(root, "pixel", 1, V3D(1, 0, 0), Quat(), nullptr);
    auto const result = builder.build();
    TS_ASSERT_EQUALS(result.detectorInfo->position(0), V3D(1, 1, 0));
    TS_ASSERT_EQUALS(result.detectorInfo->rotation(0), Quat(90, V3D(0, 0, 1)));
  }

  void test_component_types() {
    BeamlineBuilder builder;
    auto const root = builder.addAssembly(BeamlineBuilder::noParent, "root");
    Mantid::Beamline::PixelGridComponent pixelGrid;
    pixelGrid.nX = 1;
    pixelGrid.nY = 1;
    addPixelColumn(builder, builder.addRectangularBank(root, "rectangular", V3D(), Quat(), pixelGrid), 1);
    pixelGrid.nZ = 1;
    auto const grid = builder.addGridBank(root, "grid", V3D(), Quat(), pixelGrid);
    addPixelColumn(builder, builder.addAssembly(grid, "layer"), 2);
    Mantid::Beamline::StructuredGridComponent structuredGrid;
    structuredGrid.nX = 1;
    structuredGrid.nY = 1;
    addPixelColumn(builder, builder.addStructuredBank(root, "structured", V3D(), Quat(), structuredGrid), 3);
    builder.addOutlineAssembly(root, "outline", V3D(), Quat(), ComponentCreationHelper::createSphere(0.01));
    builder.addOutlineAssembly(root, "unshaped", V3D(), Quat(), nullptr);
    builder.addObjComponent(root, "finite", V3D(), Quat(), ComponentCreationHelper::createSphere(0.01));
    builder.addObjComponent(root, "infinite", V3D(), Quat(), nullptr);
    auto const generic = builder.addComponent(root, "generic");
    auto const result = builder.build();
    auto const &componentInfo = *result.componentInfo;
    auto const typeOf = [&componentInfo](std::string const &name) {
      for (size_t i = 0; i < componentInfo.size(); ++i) {
        if (componentInfo.name(i) == name) {
          return componentInfo.componentType(i);
        }
      }
      throw std::runtime_error("no component " + name);
    };
    using Mantid::Beamline::ComponentType;
    TS_ASSERT_EQUALS(typeOf("root"), ComponentType::Unstructured);
    TS_ASSERT_EQUALS(typeOf("rectangular"), ComponentType::Rectangular);
    TS_ASSERT_EQUALS(typeOf("grid"), ComponentType::Grid);
    TS_ASSERT_EQUALS(typeOf("structured"), ComponentType::Structured);
    TS_ASSERT_EQUALS(typeOf("outline"), ComponentType::OutlineComposite);
    TS_ASSERT_EQUALS(typeOf("unshaped"), ComponentType::Unstructured);
    TS_ASSERT_EQUALS(typeOf("finite"), ComponentType::Generic);
    TS_ASSERT_EQUALS(typeOf("infinite"), ComponentType::Infinite);
    TS_ASSERT_EQUALS(typeOf("generic"), ComponentType::Generic);
    TS_ASSERT_EQUALS(builder.size(), generic + 1);
  }

  void test_bank_metadata_is_kept() {
    BeamlineBuilder builder;
    auto const root = builder.addAssembly(BeamlineBuilder::noParent, "root");
    Mantid::Beamline::PixelGridComponent pixelGrid;
    pixelGrid.nX = 1;
    pixelGrid.nY = 1;
    pixelGrid.idStart = 3;
    addPixelColumn(builder, builder.addRectangularBank(root, "bank", V3D(), Quat(), pixelGrid), 3);
    Mantid::Beamline::StructuredGridComponent structuredGrid;
    structuredGrid.nX = 1;
    structuredGrid.nY = 1;
    structuredGrid.idStart = 4;
    structuredGrid.xVertices = {0, 1, 0, 1};
    structuredGrid.yVertices = {0, 0, 1, 1};
    addPixelColumn(builder, builder.addStructuredBank(root, "structured", V3D(), Quat(), structuredGrid), 4);
    auto const result = builder.build();
    auto const &componentInfo = *result.componentInfo;
    // Two detectors, then each column and its bank in post-order, then the root
    TS_ASSERT_EQUALS(componentInfo.name(3), "bank");
    TS_ASSERT_EQUALS(componentInfo.pixelGridComponent(3).idStart, 3);
    TS_ASSERT_EQUALS(componentInfo.name(5), "structured");
    TS_ASSERT_EQUALS(componentInfo.structuredGridComponent(5).idStart, 4);
    TS_ASSERT_EQUALS(componentInfo.structuredGridComponent(5).xVertices, structuredGrid.xVertices);
  }

  void test_banks_must_hold_their_grid_of_pixels() {
    Mantid::Beamline::PixelGridComponent pixelGrid;
    pixelGrid.nX = 1;
    pixelGrid.nY = 2;
    {
      BeamlineBuilder builder;
      auto const root = builder.addAssembly(BeamlineBuilder::noParent, "root");
      builder.addRectangularBank(root, "empty", V3D(), Quat(), pixelGrid);
      TS_ASSERT_THROWS(builder.build(), std::invalid_argument const &);
    }
    {
      BeamlineBuilder builder;
      auto const root = builder.addAssembly(BeamlineBuilder::noParent, "root");
      addPixelColumn(builder, builder.addRectangularBank(root, "short column", V3D(), Quat(), pixelGrid), 1);
      TS_ASSERT_THROWS(builder.build(), std::invalid_argument const &);
    }
    {
      BeamlineBuilder builder;
      auto const root = builder.addAssembly(BeamlineBuilder::noParent, "root");
      auto layeredGrid = pixelGrid;
      layeredGrid.nY = 1;
      layeredGrid.nZ = 1;
      addPixelColumn(builder, builder.addRectangularBank(root, "layered", V3D(), Quat(), layeredGrid), 1);
      TS_ASSERT_THROWS(builder.build(), std::invalid_argument const &);
    }
    {
      BeamlineBuilder builder;
      auto const root = builder.addAssembly(BeamlineBuilder::noParent, "root");
      pixelGrid.nZ = 2;
      auto const grid = builder.addGridBank(root, "one layer", V3D(), Quat(), pixelGrid);
      auto const column = builder.addAssembly(builder.addAssembly(grid, "layer"), "column");
      builder.addDetector(column, "a", 1, V3D(), Quat(), nullptr);
      builder.addDetector(column, "b", 2, V3D(), Quat(), nullptr);
      TS_ASSERT_THROWS(builder.build(), std::invalid_argument const &);
    }
  }

  void test_parameters_are_keyed_by_component_index() {
    BeamlineBuilder builder;
    auto const root = builder.addAssembly(BeamlineBuilder::noParent, "root");
    auto const pixel = builder.addDetector(root, "pixel", 1, V3D(), Quat(), nullptr);
    auto parameter = ParameterFactory::create("double", "a");
    parameter->fromString("2.5");
    builder.addParameter(pixel, parameter);
    auto rootParameter = ParameterFactory::create("string", "b");
    rootParameter->fromString("text");
    builder.addParameter(root, rootParameter);
    std::string penaltyFactor;
    auto const logfileParameter = std::make_shared<XMLInstrumentParameter>(
        /*logfileID*/ "log", /*value*/ "", std::make_shared<Mantid::Kernel::Interpolation>(), /*formula*/ "",
        /*formulaUnit*/ "", /*resultUnit*/ "", /*paramName*/ "c", /*type*/ "double", /*tie*/ "",
        std::vector<std::string>(2, ""), penaltyFactor, /*fitFunc*/ "", /*extractSingleValueAs*/ "", /*eq*/ "",
        /*comp*/ nullptr, /*angleConvertConst*/ 1.0, /*description*/ "", /*visible*/ "true");
    builder.addLogfileParameter(root, "c", logfileParameter);
    auto const result = builder.build();
    auto const &componentInfo = *result.componentInfo;
    TS_ASSERT_EQUALS(componentInfo.getParameterAsString(0, "a", false), "2.5");
    TS_ASSERT_EQUALS(componentInfo.getParameterAsString(componentInfo.root(), "b", false), "text");
    auto const &logfileParameters = componentInfo.logfileParameters();
    TS_ASSERT_EQUALS(logfileParameters.size(), 1);
    TS_ASSERT_EQUALS(logfileParameters.begin()->first, std::make_pair(componentInfo.root(), std::string("c")));
  }

  void test_reference_frame_is_passed_on() {
    BeamlineBuilder builder;
    builder.addAssembly(BeamlineBuilder::noParent, "root");
    auto const frame = std::make_shared<ReferenceFrame>(X, Y, Left, "frame");
    builder.setReferenceFrame(frame);
    auto const result = builder.build();
    TS_ASSERT_EQUALS(result.componentInfo->referenceFrame(), frame);
    TS_ASSERT(!result.metadata);
  }

  void test_the_first_node_must_be_a_parentless_assembly() {
    BeamlineBuilder builder;
    TS_ASSERT_THROWS(builder.addComponent(BeamlineBuilder::noParent, "root"), std::invalid_argument const &);
    TS_ASSERT_THROWS(builder.addAssembly(0, "root"), std::invalid_argument const &);
    auto const root = builder.addAssembly(BeamlineBuilder::noParent, "root");
    TS_ASSERT_THROWS(builder.addAssembly(BeamlineBuilder::noParent, "second root"), std::invalid_argument const &);
    TS_ASSERT_THROWS(builder.addAssembly(root + 1, "orphan"), std::out_of_range const &);
  }

  void test_only_assemblies_have_children() {
    BeamlineBuilder builder;
    auto const root = builder.addAssembly(BeamlineBuilder::noParent, "root");
    auto const pixel = builder.addDetector(root, "pixel", 1, V3D(), Quat(), nullptr);
    auto const component = builder.addComponent(root, "component");
    TS_ASSERT_THROWS(builder.addComponent(pixel, "child"), std::invalid_argument const &);
    TS_ASSERT_THROWS(builder.addComponent(component, "child"), std::invalid_argument const &);
  }

  void test_duplicate_detector_ids_throw_on_build() {
    BeamlineBuilder builder;
    auto const root = builder.addAssembly(BeamlineBuilder::noParent, "root");
    builder.addDetector(root, "a", 1, V3D(), Quat(), nullptr);
    builder.addDetector(root, "b", 1, V3D(), Quat(), nullptr);
    TS_ASSERT_THROWS(builder.build(), Mantid::Kernel::Exception::InstrumentDefinitionError const &);
  }

  void test_building_nothing_throws() { TS_ASSERT_THROWS(BeamlineBuilder().build(), std::logic_error const &); }

  void test_unknown_nodes_throw() {
    BeamlineBuilder builder;
    builder.addAssembly(BeamlineBuilder::noParent, "root");
    TS_ASSERT_THROWS(builder.setSource(1), std::out_of_range const &);
    TS_ASSERT_THROWS(builder.setSample(1), std::out_of_range const &);
    TS_ASSERT_THROWS(builder.setScaleFactor(1, V3D(1, 1, 1)), std::out_of_range const &);
    TS_ASSERT_THROWS(builder.addParameter(1, nullptr), std::out_of_range const &);
  }

  void test_the_first_source_and_sample_are_kept() {
    BeamlineBuilder builder;
    auto const root = builder.addAssembly(BeamlineBuilder::noParent, "root");
    builder.setSource(builder.addComponent(root, "source"));
    builder.setSource(builder.addComponent(root, "second source"));
    builder.setSample(builder.addComponent(root, "sample"));
    builder.setSample(builder.addComponent(root, "second sample"));
    auto const result = builder.build();
    TS_ASSERT_EQUALS(result.componentInfo->name(result.componentInfo->source()), "source");
    TS_ASSERT_EQUALS(result.componentInfo->name(result.componentInfo->sample()), "sample");
  }

  void test_a_grid_without_layers_has_a_bounding_box() {
    BeamlineBuilder builder;
    auto const root = builder.addAssembly(BeamlineBuilder::noParent, "root");
    Mantid::Beamline::PixelGridComponent pixelGrid;
    pixelGrid.nX = 1;
    pixelGrid.nY = 1;
    auto const grid = builder.addGridBank(root, "grid", V3D(1, 2, 3), Quat(), pixelGrid);
    auto const column = builder.addAssembly(grid, "column");
    builder.addDetector(column, "pixel", 1, V3D(), Quat(), ComponentCreationHelper::createSphere(0.01));
    auto const result = builder.build();
    auto const &componentInfo = *result.componentInfo;
    auto const box = componentInfo.boundingBox(componentInfo.indexOfAny("grid"));
    TS_ASSERT_DELTA(box.centrePoint().distance(V3D(1, 2, 3)), 0, 1e-12);
  }

  void test_one_component_can_be_source_and_sample() {
    BeamlineBuilder builder;
    auto const root = builder.addAssembly(BeamlineBuilder::noParent, "root");
    auto const both = builder.addComponent(root, "both");
    builder.setSource(both);
    builder.setSample(both);
    auto const beamline = builder.build();
    auto const walked = walk(Instrument::fromBeamline(beamline));
    TS_ASSERT_EQUALS(walked.componentInfo->source(), beamline.componentInfo->source());
    TS_ASSERT_EQUALS(walked.componentInfo->sample(), beamline.componentInfo->sample());
  }

  void test_bank_pixels_keep_their_side_by_side_view_positions() {
    auto const legacy = ComponentCreationHelper::createTestInstrumentRectangular2(1, 2);
    std::shared_ptr<ICompAssembly> bank;
    for (int i = 0; i < legacy->nelements() && !bank; ++i) {
      bank = std::dynamic_pointer_cast<RectangularDetector>((*legacy)[i]);
    }
    TS_ASSERT(bank);
    auto const column = std::dynamic_pointer_cast<ICompAssembly>((*bank)[0]);
    (*column)[1]->setSideBySideViewPos(V2D(0.5, 0.25));
    auto const beamline = walk(legacy);
    assertSameBeamline(walk(Instrument::fromBeamline(beamline)), beamline);
  }

  void test_null_parameters_throw() {
    BeamlineBuilder builder;
    auto const root = builder.addAssembly(BeamlineBuilder::noParent, "root");
    TS_ASSERT_THROWS(builder.addParameter(root, nullptr), std::invalid_argument const &);
    TS_ASSERT_THROWS(builder.addLogfileParameter(root, "a", nullptr), std::invalid_argument const &);
  }

  void test_either_info_keeps_both_alive() {
    auto componentInfo = makeBuilder(Placement()).build().componentInfo;
    // The DetectorInfo pointer is gone, but the ComponentInfo still reads detector positions through it
    TS_ASSERT_EQUALS(componentInfo->name(0), "monitor");
    TS_ASSERT_EQUALS(componentInfo->position(0), makeBuilder(Placement()).build().detectorInfo->position(0));

    auto detectorInfo = makeBuilder(Placement()).build().detectorInfo;
    std::weak_ptr<const ComponentInfo> const weakComponentInfo = [&detectorInfo] {
      auto beamline = makeBuilder(Placement()).build();
      detectorInfo = beamline.detectorInfo;
      return std::weak_ptr<const ComponentInfo>(beamline.componentInfo);
    }();
    TS_ASSERT(!weakComponentInfo.expired());
    detectorInfo.reset();
    TS_ASSERT(weakComponentInfo.expired());
  }
  void test_a_tree_made_from_a_built_beamline_walks_back_to_it() {
    auto const beamline = makeBuilder(Placement()).build();
    Instrument_sptr const instrument = Instrument::fromBeamline(beamline);
    assertSameBeamline(walk(instrument), beamline);
  }

  void test_a_tree_made_from_a_beamline_takes_it_as_its_base_beamline() {
    auto const beamline = makeBuilder(Placement()).build();
    auto const instrument = Instrument::fromBeamline(beamline);
    auto const [componentInfo, detectorInfo] = instrument->makeBaseBeamline();
    TS_ASSERT(componentInfo->hasSameBase(*beamline.componentInfo));
    auto const &builtInfo = *beamline.componentInfo;
    for (auto const &name : {"inst", "bank", "source", "slit", "pixel6"}) {
      auto const component = instrument->getComponentByName(name);
      TS_ASSERT_EQUALS(builtInfo.name(component->index()), name);
    }
    TS_ASSERT_EQUALS(instrument->getSource()->getName(), "source");
    TS_ASSERT_EQUALS(instrument->getSample()->getName(), "sample");
    TS_ASSERT(instrument->isMonitor(1));
    TS_ASSERT_EQUALS(instrument->getDetectorIDs(), (std::vector<detid_t>{1, 5, 6, 7}));
  }

  void test_legacy_trees_survive_a_round_trip() {
    for (auto const &instrument : {ComponentCreationHelper::createTestInstrumentRectangular2(2, 3),
                                   ComponentCreationHelper::createTestInstrumentCylindrical(2),
                                   ComponentCreationHelper::createInstrumentWithPSDTubes(3, 4),
                                   ComponentCreationHelper::createMinimalInstrumentWithMonitor(V3D(0, 0, -1), Quat()),
                                   makeLegacyInstrument(Placement()), makeBankInstrument()}) {
      auto const beamline = walk(instrument);
      assertSameBeamline(walk(Instrument::fromBeamline(beamline)), beamline);
    }
  }

  void test_logfile_parameters_go_to_the_logfile_cache() {
    BeamlineBuilder builder;
    auto const root = builder.addAssembly(BeamlineBuilder::noParent, "root");
    auto const pixel = builder.addDetector(root, "pixel", 1, V3D(), Quat(), nullptr);
    std::string penaltyFactor;
    auto const parameter = std::make_shared<XMLInstrumentParameter>(
        /*logfileID*/ "", /*value*/ "1.5", std::make_shared<Mantid::Kernel::Interpolation>(), /*formula*/ "",
        /*formulaUnit*/ "", /*resultUnit*/ "", /*paramName*/ "a", /*type*/ "double", /*tie*/ "",
        std::vector<std::string>(2, ""), penaltyFactor, /*fitFunc*/ "", /*extractSingleValueAs*/ "", /*eq*/ "",
        /*comp*/ nullptr, /*angleConvertConst*/ 1.0, /*description*/ "", /*visible*/ "true");
    builder.addLogfileParameter(pixel, "a", parameter);
    auto const instrument = Instrument::fromBeamline(builder.build());
    auto const &cache = instrument->getLogfileCache();
    TS_ASSERT_EQUALS(cache.size(), 1);
    auto const detector = instrument->getComponentByName("pixel");
    TS_ASSERT_EQUALS(cache.begin()->first.first, "a");
    TS_ASSERT_EQUALS(cache.begin()->first.second, detector.get());
    TS_ASSERT_EQUALS(cache.begin()->second, parameter);
  }

  void test_metadata_goes_to_the_instrument() {
    BeamlineBuilder physicalBuilder;
    physicalBuilder.addAssembly(BeamlineBuilder::noParent, "physical");
    auto const physical = physicalBuilder.build();
    BeamlineBuilder builder;
    builder.addAssembly(BeamlineBuilder::noParent, "neutronic");
    Mantid::Types::Core::DateAndTime const from("2020-01-01T00:00:00"), to("2030-01-01T00:00:00");
    builder.setMetadata(std::make_shared<InstrumentMetadata>(from, to, "file.xml", "<xml/>", "3D", "Z+",
                                                             physical.componentInfo, physical.detectorInfo));
    auto const instrument = Instrument::fromBeamline(builder.build());
    TS_ASSERT_EQUALS(instrument->getValidFromDate(), from);
    TS_ASSERT_EQUALS(instrument->getValidToDate(), to);
    TS_ASSERT_EQUALS(instrument->getFilename(), "file.xml");
    TS_ASSERT_EQUALS(instrument->getXmlText(), "<xml/>");
    TS_ASSERT_EQUALS(instrument->getDefaultView(), "3D");
    TS_ASSERT_EQUALS(instrument->getDefaultAxis(), "Z+");
    TS_ASSERT_EQUALS(instrument->getPhysicalInstrument()->getName(), "physical");
  }

  void test_a_tree_cannot_hold_scale_factors() {
    BeamlineBuilder builder;
    auto const root = builder.addAssembly(BeamlineBuilder::noParent, "root");
    builder.setScaleFactor(builder.addComponent(root, "scaled"), V3D(2, 1, 1));
    TS_ASSERT_THROWS(Instrument::fromBeamline(builder.build()), std::invalid_argument const &);
  }

  void test_a_tree_root_is_a_plain_assembly() {
    BeamlineBuilder builder;
    builder.addOutlineAssembly(BeamlineBuilder::noParent, "outlined", V3D(), Quat(),
                               ComponentCreationHelper::createSphere(0.01));
    TS_ASSERT_THROWS(Instrument::fromBeamline(builder.build()), std::invalid_argument const &);
  }

  void test_bank_pixels_must_be_where_the_beamline_puts_them() {
    InstrumentVisitor visitor(ComponentCreationHelper::createTestInstrumentRectangular2(1, 2));
    visitor.walkInstrument();
    auto [componentInfo, detectorInfo] = visitor.makeWrappers();
    detectorInfo->setPosition(0, detectorInfo->position(0) + V3D(0, 0, 1));
    auto const beamline = makeBeamlineRef(std::move(componentInfo), std::move(detectorInfo));
    TS_ASSERT_THROWS(Instrument::fromBeamline(beamline), std::invalid_argument const &);
  }

  void test_a_tree_cannot_hold_plain_parameters() {
    BeamlineBuilder builder;
    auto const root = builder.addAssembly(BeamlineBuilder::noParent, "root");
    builder.addParameter(root, ParameterFactory::create("double", "a"));
    TS_ASSERT_THROWS(Instrument::fromBeamline(builder.build()), std::invalid_argument const &);
  }
};
