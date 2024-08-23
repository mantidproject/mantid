// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2024 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include "MantidDataObjects/Workspace2D.h"
#include "MantidGeometry/Instrument.h"
#include "MantidQtWidgets/Common/MessageHandler.h"
#include "MantidQtWidgets/InstrumentView/InstrumentActor.h"
#include "MantidQtWidgets/InstrumentView/InstrumentRendererClassic.h"

#include <cxxtest/TestSuite.h>

using namespace Mantid::DataObjects;
using namespace Mantid::Geometry;
using namespace MantidQt::MantidWidgets;

class InstrumentRendererTest : public CxxTest::TestSuite {
public:
  // This pair of boilerplate methods prevent the suite being created statically
  // This means the constructor isn't called when running other tests
  static InstrumentRendererTest *createSuite() { return new InstrumentRendererTest(); }
  static void destroySuite(InstrumentRendererTest *suite) { delete suite; }

  void test_renderer_constructs_with_empty_instrument() {
    auto ws = std::make_shared<Workspace2D>();
    auto inst = std::make_shared<Instrument>();
    inst->setName("DEVA");
    ws->setInstrument(inst);
    auto messageHandler = std::make_unique<MessageHandler>();

    InstrumentActor actor(ws, *messageHandler);
    TS_ASSERT(actor.componentInfo().size() <= 1);
    TS_ASSERT_THROWS_NOTHING(auto renderer = std::make_unique<InstrumentRendererClassic>(actor));
  }
};