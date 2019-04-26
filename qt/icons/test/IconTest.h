// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#ifndef MANTIDQT_ICONS_ICONTEST_H
#define MANTIDQT_ICONS_ICONTEST_H

#include "MantidQtIcons/Icon.h"

#include <QApplication>
#include <cxxtest/TestSuite.h>

using namespace MantidQt::Icons;

/// This QApplication object is required to construct the view
class QApplicationHolder : CxxTest::GlobalFixture {
public:
  bool setUpWorld() override {
    int argc(0);
    char **argv = {};
    m_app = new QApplication(argc, argv);
    return true;
  }

  bool tearDownWorld() override {
    delete m_app;
    return true;
  }

private:
  QApplication *m_app;
};

static QApplicationHolder MAIN_QAPPLICATION;

class IconTest : public CxxTest::TestSuite {
public:
  static IconTest *createSuite() { return new IconTest; }
  static void destroySuite(IconTest *suite) { delete suite; }

  void testGetIconShort() {
    auto icon =
        MantidQt::Icons::getIcon(QString("mdi.run-fast"), QString("red"), 1.5);
    auto isNull = icon.isNull();
    TS_ASSERT(!isNull);
  }

  void testGetIconLong() {
    QList<QHash<QString, QVariant>> options;
    QHash<QString, QVariant> option1;
    option1.insert(QString("color"), QVariant(QString("red")));
    option1.insert(QString("scaleFactor"), QVariant(1.5));
    options.append(option1);
    auto icon = MantidQt::Icons::getIcon({QString("mdi.run-fast")}, options);
    auto isNull = icon.isNull();
    TS_ASSERT(!isNull);
  }

  void testIconicFontGetIconThrowsOnOptionsAndVectorOfDifferentSizes() {
    QList<QHash<QString, QVariant>> options;
    QHash<QString, QVariant> option1;
    option1.insert(QString("color"), QVariant(QString("red")));
    option1.insert(QString("scaleFactor"), QVariant(1.5));
    options.append(option1);
    TS_ASSERT_THROWS(
        MantidQt::Icons::getIcon(
            {QString("mdi.run-fast"), QString("mdi.run-fast")}, options),
        std::invalid_argument)
  }

  void testGetFont() {
    IconicFont iconicFont;
    auto font = iconicFont.getFont(QString("mdi"), 16);
    TS_ASSERT_EQUALS(font.family().toStdString(), "Material Design Icons")
    TS_ASSERT_EQUALS(font.pixelSize(), 16)
  }

  void testGetIconThrowsIfIconLibraryIsNotPresent() {
    TS_ASSERT_THROWS(MantidQt::Icons::getIcon(QString("fda.run-fast")),
                     std::invalid_argument);
  }
};

#endif