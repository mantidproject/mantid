// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#ifndef MANTID_DATAOBJECTS_PEAKCOLUMNTEST_H_
#define MANTID_DATAOBJECTS_PEAKCOLUMNTEST_H_

#include "MantidDataObjects/Peak.h"
#include "MantidDataObjects/PeakColumn.h"
#include "MantidKernel/Exception.h"
#include <cxxtest/TestSuite.h>

#include "MantidTestHelpers/ComponentCreationHelper.h"

#include <boost/make_shared.hpp>
#include <locale>

using namespace Mantid::DataObjects;

// Allow testing of protected methods
class PeakColumnTestHelper : public PeakColumn {
public:
  PeakColumnTestHelper(std::vector<Peak> &peaks, const std::string &name)
      : PeakColumn(peaks, name) {}

  using PeakColumn::insert;
  using PeakColumn::remove;
  using PeakColumn::resize;
};

class PeakColumnTest : public CxxTest::TestSuite {
public:
  // This pair of boilerplate methods prevent the suite being created statically
  // This means the constructor isn't called when running other tests
  static PeakColumnTest *createSuite() { return new PeakColumnTest(); }
  static void destroySuite(PeakColumnTest *suite) { delete suite; }

  PeakColumnTest() : m_peaks(2) {
    m_inst = ComponentCreationHelper::createTestInstrumentCylindrical(1);
    m_peaks[0] = Peak(m_inst, 1, 4.0);
    m_peaks[1] = Peak(m_inst, 2, 4.1);
  }

  void test_constructor_create_valid_object_when_given_valid_name() {
    PeakColumn pc(m_peaks, "h");
    TS_ASSERT_EQUALS(pc.name(), "h");
    TS_ASSERT_EQUALS(pc.size(), 2);
  }

  void test_constructor_throws_given_unknown_name() {
    TS_ASSERT_THROWS(PeakColumn(m_peaks, "NotPeakColumn"),
                     const std::runtime_error &);
  }

  void test_clone() {
    PeakColumn pc(m_peaks, "h");
    PeakColumn *cloned = pc.clone();

    TS_ASSERT_EQUALS(pc.name(), cloned->name());
    TS_ASSERT_EQUALS(2, cloned->size());

    delete cloned;
  }

  void test_type_info_is_expected_type_based_on_string_type() {
    PeakColumnTestHelper pcInt(m_peaks, "DetID");
    TS_ASSERT(pcInt.get_type_info() == typeid(int));
    TS_ASSERT(pcInt.get_pointer_type_info() == typeid(int *));

    PeakColumnTestHelper pcDouble(m_peaks, "h");
    TS_ASSERT(pcDouble.get_type_info() == typeid(double));
    TS_ASSERT(pcDouble.get_pointer_type_info() == typeid(double *));

    PeakColumnTestHelper pcStr(m_peaks, "BankName");
    TS_ASSERT(pcStr.get_type_info() == typeid(std::string));
    TS_ASSERT(pcStr.get_pointer_type_info() == typeid(std::string *));

    PeakColumnTestHelper pcV3D(m_peaks, "QLab");
    TS_ASSERT(pcV3D.get_type_info() == typeid(Mantid::Kernel::V3D));
    TS_ASSERT(pcV3D.get_pointer_type_info() == typeid(Mantid::Kernel::V3D *));
  }

  void test_PeakColumn_Cannot_Be_Resized() {
    PeakColumnTestHelper pc(m_peaks, "DetID");
    TS_ASSERT_THROWS(pc.resize(10),
                     const Mantid::Kernel::Exception::NotImplementedError &);
  }

  void test_Row_Cannot_Be_Inserted_Into_PeakColumn() {
    PeakColumnTestHelper pc(m_peaks, "DetID");
    TS_ASSERT_THROWS(pc.insert(0),
                     const Mantid::Kernel::Exception::NotImplementedError &);
  }

  void test_Row_Cannot_Be_Removed_From_PeakColumn() {
    PeakColumnTestHelper pc(m_peaks, "DetID");
    TS_ASSERT_THROWS(pc.remove(0),
                     const Mantid::Kernel::Exception::NotImplementedError &);
  }

  void test_cell_returns_correct_value_from_PeakColumn() {
    PeakColumn pc1(m_peaks, "DetID");
    int detId = pc1.cell<int>(0);
    TS_ASSERT_EQUALS(1, detId);
    detId = pc1.cell<int>(1);
    TS_ASSERT_EQUALS(2, detId);

    PeakColumn pc2(m_peaks, "QLab");
    const Mantid::Kernel::V3D &qlab0 = pc2.cell<Mantid::Kernel::V3D>(0);
    TS_ASSERT_EQUALS(qlab0, m_peaks[0].getQLabFrame());
    const Mantid::Kernel::V3D &qlab1 = pc2.cell<Mantid::Kernel::V3D>(1);
    TS_ASSERT_EQUALS(qlab1, m_peaks[1].getQLabFrame());
  }

  void test_get_read_only_returns_correct_value() {
    PeakColumn pc1(m_peaks, "h");
    auto readOnly = pc1.getReadOnly();
    TS_ASSERT(!readOnly);

    PeakColumn pc2(m_peaks, "DetID");
    readOnly = pc2.getReadOnly();
    TS_ASSERT(readOnly);
  }

  void test_read_locale_awerness() {
    const std::vector<std::string> columnNames{"h", "k", "l", "RunNumber"};
    const std::vector<double> columnValues{-2.0, 5.0, 12.0, 143290.0};
    TestingNumpunctFacet numpunct;
    const std::locale testLocale(std::locale::classic(), &numpunct);
    for (size_t i = 0; i < columnNames.size(); ++i) {
      PeakColumn pc(m_peaks, columnNames[i]);
      // Use a fake punctuation facet for numeric formatting.
      std::ostringstream out;
      out.imbue(testLocale);
      // Force some decimals to the numbers.
      out << std::fixed;
      out << std::setprecision(2);
      out << columnValues[i];
      std::istringstream in(out.str());
      in.imbue(testLocale);
      pc.read(0, in);
      switch (i) {
      case 0:
        TS_ASSERT_EQUALS(m_peaks[0].getH(), columnValues[i])
        break;
      case 1:
        TS_ASSERT_EQUALS(m_peaks[0].getK(), columnValues[i])
        break;
      case 2:
        TS_ASSERT_EQUALS(m_peaks[0].getL(), columnValues[i])
        break;
      case 3:
        TS_ASSERT_EQUALS(m_peaks[0].getRunNumber(), columnValues[i])
        break;
      }
    }
  }

  void test_cannot_be_converted_to_double() {
    PeakColumn col(m_peaks, "DetID");
    TS_ASSERT(!col.isNumber());
  }

private:
  /// A locale facet mocking non-english numerical punctuation.
  class TestingNumpunctFacet : public std::numpunct<char> {
  public:
    /// Informs locales not to delete this facet.
    TestingNumpunctFacet() : std::numpunct<char>(1) {}

  private:
    char_type do_decimal_point() const override { return '%'; }
    char_type do_thousands_sep() const override { return '@'; }
    string_type do_grouping() const override { return "\03"; }
  };

  Mantid::Geometry::Instrument_sptr m_inst;
  std::vector<Peak> m_peaks;
};

#endif /* MANTID_DATAOBJECTS_PEAKCOLUMNTEST_H_ */
