// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2019 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include "MantidQtWidgets/Common/QtJSONUtils.h"

#include <cxxtest/TestSuite.h>

#include <Poco/TemporaryFile.h>
#include <QFile>
#include <QMap>
#include <QString>
#include <QVariant>

static QString JSON{"{\"int\": 1, \"double\": 1.0, \"string\": \"text\", \"bool\": true, "
                    "\"list\":[1,2,3]}"};

class QtJSONUtilsTest : public CxxTest::TestSuite {
public:
  // This pair of boilerplate methods prevent the suite being created statically
  // This means the constructor isn't called when running other tests
  static QtJSONUtilsTest *createSuite() { return new QtJSONUtilsTest(); }

  static void destroySuite(QtJSONUtilsTest *suite) { delete suite; }

  void test_saveJSONToFile_and_loadJSONFromFile() {
    Poco::TemporaryFile tmpFile;
    QString filename(tmpFile.path().data());
    auto map1 = constructJSONMap();
    MantidQt::API::saveJSONToFile(filename, map1);

    auto map2 = MantidQt::API::loadJSONFromFile(filename);
    testMaps(map1, map2);

    QFile file(filename);
  }

  void test_loadJSONFromString() {
    auto map = MantidQt::API::loadJSONFromString(JSON);
    testMaps(map, constructJSONMap());
  }

  void test_outputJSONToStringQVariantMap() {
    QVariantMap m{{"testkey", "testval"}, {"testkey1", "testval1"}, {"testkey2", "testval2"}};
    QVariant v{m};
    std::string output = MantidQt::API::outputJsonToString(v);
    TS_ASSERT_EQUALS("{\"testkey\":\"testval\",\"testkey1\":\"testval1\",\"testkey2\":\"testval2\"}", output);
  }

  void test_outputJSONToStringQVariantList() {
    QVariantList l{"teststr", "teststr1", "teststr2"};
    QVariant v{l};
    std::string output = MantidQt::API::outputJsonToString(v);
    TS_ASSERT_EQUALS("[\"teststr\",\"teststr1\",\"teststr2\"]", output);
  }

  void test_outputJSONToStringQVariantString() {
    QVariant v{"teststr"};
    std::string output = MantidQt::API::outputJsonToString(v);
    TS_ASSERT_EQUALS("teststr", output);
  }

private:
  QMap<QString, QVariant> constructJSONMap() {
    QMap<QString, QVariant> map;
    map.insert(QString("int"), QVariant(1));
    map.insert(QString("double"), QVariant(1.0));
    map.insert(QString("string"), QVariant(QString("text")));
    map.insert(QString("bool"), QVariant(true));
    QList<QVariant> list{QVariant(1), QVariant(2), QVariant(3)};
    map.insert(QString("list"), QVariant(list));
    return map;
  }

  void testMaps(const QMap<QString, QVariant> &map1, const QMap<QString, QVariant> &map2) {
    TS_ASSERT_EQUALS(map1[QString("int")].toInt(), 1)
    TS_ASSERT_EQUALS(map2[QString("int")].toInt(), map1[QString("int")].toInt())
    TS_ASSERT_EQUALS(map1[QString("double")].toDouble(), 1.0)
    TS_ASSERT_EQUALS(map2[QString("double")].toDouble(), map1[QString("double")].toDouble())
    TS_ASSERT_EQUALS(map1[QString("string")].toString(), QString("text"))
    TS_ASSERT_EQUALS(map2[QString("string")].toString(), map1[QString("string")].toString())
    TS_ASSERT_EQUALS(map1[QString("bool")].toBool(), true)
    TS_ASSERT_EQUALS(map2[QString("bool")].toBool(), map1[QString("bool")].toBool())
    QList<QVariant> list{QVariant(1), QVariant(2), QVariant(3)};
    TS_ASSERT_EQUALS(list, map1[QString("list")].toList())
    TS_ASSERT_EQUALS(map2[QString("list")].toList(), map1[QString("list")].toList())
  }
};
