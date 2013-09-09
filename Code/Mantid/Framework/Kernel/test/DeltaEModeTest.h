#ifndef MANTID_KERNEL_DELTAEMODETEST_H_
#define MANTID_KERNEL_DELTAEMODETEST_H_

#include "MantidKernel/DeltaEMode.h"
#include <cxxtest/TestSuite.h>

using Mantid::Kernel::DeltaEMode;

class DeltaEModeTest : public CxxTest::TestSuite
{
public:
  // This pair of boilerplate methods prevent the suite being created statically
  // This means the constructor isn't called when running other tests
  static DeltaEModeTest *createSuite() { return new DeltaEModeTest(); }
  static void destroySuite( DeltaEModeTest *suite ) { delete suite; }
  
  DeltaEModeTest() :
    m_elasticString("Elastic"),
    m_directString("Direct"),
    m_indirectString("Indirect")
  {}

  void test_availableTypes_Are_The_3_Expected_In_The_Correct_Order()
  {
    const std::vector<std::string> modes = DeltaEMode::availableTypes();
    TS_ASSERT_EQUALS(modes.size(), 3);
    TS_ASSERT_EQUALS(modes[0], m_elasticString);
    TS_ASSERT_EQUALS(modes[1], m_directString);
    TS_ASSERT_EQUALS(modes[2], m_indirectString);
  }

  void test_elastic_mode_is_correctly_transformed_to_and_from_string()
  {
    TS_ASSERT_EQUALS(DeltaEMode::asString(DeltaEMode::Elastic), m_elasticString);
    TS_ASSERT_EQUALS(DeltaEMode::fromString(m_elasticString), DeltaEMode::Elastic);
  }

  void test_direct_mode_is_correctly_transformed_to_and_from_string()
  {
    TS_ASSERT_EQUALS(DeltaEMode::asString(DeltaEMode::Direct), m_directString);
    TS_ASSERT_EQUALS(DeltaEMode::fromString(m_elasticString), DeltaEMode::Elastic);
  }

  void test_indirect_mode_is_correctly_transformed_to_and_from_string()
  {
    TS_ASSERT_EQUALS(DeltaEMode::asString(DeltaEMode::Indirect), m_indirectString);
    TS_ASSERT_EQUALS(DeltaEMode::fromString(m_elasticString), DeltaEMode::Elastic);
  }

  void test_unknown_mode_raises_error_in_to_and_from_string()
  {
    const unsigned int mode = 1000;
    TS_ASSERT_THROWS(DeltaEMode::asString((DeltaEMode::Type)mode), std::invalid_argument);
    TS_ASSERT_THROWS(DeltaEMode::fromString("Not emode"), std::invalid_argument);
  }

private:
  const std::string m_elasticString, m_directString, m_indirectString;
};

#endif /* MANTID_KERNEL_DELTAEMODETEST_H_ */
