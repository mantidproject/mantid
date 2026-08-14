// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2026 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include "MantidQtWidgets/InstrumentView/InstrumentActor.h"
#include "MantidQtWidgets/InstrumentView/InstrumentWidgetPickTab.h"
#include "MantidQtWidgets/InstrumentView/InstrumentWidgetRenderTab.h"

#include <QFile>
#include <QSettings>
#include <QTemporaryDir>
#include <cxxtest/TestSuite.h>

using namespace MantidQt::MantidWidgets;

class InstrumentViewSettingsTest : public CxxTest::TestSuite {
public:
  void test_actor_read_does_not_modify_storage_and_save_preserves_unrelated_keys() {
    QTemporaryDir directory;
    TS_ASSERT(directory.isValid());
    auto const filename = directory.filePath("settings.ini");
    QSettings storage(filename, QSettings::IniFormat);
    storage.setValue("ColormapFile", "map.xml");
    storage.setValue("ColormapFileHighlightZeros", true);
    storage.setValue("ScaleType", 1);
    storage.setValue("ShowGuides", true);
    storage.setValue("unrelated", "preserved");
    storage.sync();
    auto const contentsBefore = fileContents(filename);

    auto const values = InstrumentActorSettings::readSettings(storage);
    storage.sync();

    TS_ASSERT_EQUALS(values.colorMapFile(), QString("map.xml"));
    TS_ASSERT(values.highlightZeros());
    TS_ASSERT_EQUALS(values.scaleType(), 1);
    TS_ASSERT(values.showGuides());
    TS_ASSERT_EQUALS(fileContents(filename), contentsBefore);

    InstrumentActorSettings::saveSettings(storage, InstrumentActorSettings("new.xml", false, 0, false));
    storage.sync();
    TS_ASSERT_EQUALS(storage.value("unrelated").toString(), QString("preserved"));
    TS_ASSERT_EQUALS(storage.childKeys().size(), 5);
  }

  void test_pick_and_render_reads_do_not_modify_storage_and_saves_preserve_unrelated_keys() {
    QTemporaryDir directory;
    TS_ASSERT(directory.isValid());
    auto const filename = directory.filePath("settings.ini");
    QSettings storage(filename, QSettings::IniFormat);
    storage.setValue("TubeXUnits", 2);
    storage.setValue("PlotType", 3);
    storage.setValue("RebinKeeporiginal", false);
    storage.setValue("3DAxesShown", 0);
    storage.setValue("unrelated", "preserved");
    storage.sync();
    auto const contentsBefore = fileContents(filename);

    auto const pick = InstrumentWidgetPickTabSettings::readSettings(storage);
    auto const render = InstrumentWidgetRenderTabSettings::readSettings(storage);
    storage.sync();

    TS_ASSERT_EQUALS(pick.tubeXUnits(), 2);
    TS_ASSERT_EQUALS(pick.plotType(), 3);
    TS_ASSERT(!pick.rebinKeepOriginal());
    TS_ASSERT(!render.axesShown());
    TS_ASSERT_EQUALS(fileContents(filename), contentsBefore);

    InstrumentWidgetPickTabSettings::saveSettings(storage, InstrumentWidgetPickTabSettings(1, 2, true));
    InstrumentWidgetRenderTabSettings::saveSettings(storage, InstrumentWidgetRenderTabSettings(true));
    storage.sync();
    TS_ASSERT_EQUALS(storage.value("unrelated").toString(), QString("preserved"));
    TS_ASSERT_EQUALS(storage.childKeys().size(), 5);
  }

private:
  QByteArray fileContents(const QString &filename) {
    QFile file(filename);
    TS_ASSERT(file.open(QIODevice::ReadOnly));
    return file.readAll();
  }
};
