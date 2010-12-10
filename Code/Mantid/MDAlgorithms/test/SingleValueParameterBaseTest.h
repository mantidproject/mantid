
#ifndef SINGLE_VALUE_PARAMETER_TEST_H
#define SINGLE_VALUE_PARAMETER_TEST_H 

template <typename SingleValueParameter>
class SingleValueParameterTests
{
public:

  void testGetName(std::string expectedValue)
  {
    SingleValueParameter svp;
    TSM_ASSERT_EQUALS("getName not configured correctly", expectedValue, svp.getName());
    TSM_ASSERT_EQUALS("getName does not match parameterName", SingleValueParameter::parameterName() , svp.getName());
  }

  void testIsValid()
  {
    SingleValueParameter svp(0);
    TSM_ASSERT_EQUALS("The Parameter should be valid.", true, svp.isValid());
  }

  void testIsNotValid()
  {
    SingleValueParameter svp;
    TSM_ASSERT_EQUALS("Parameter constructed via parameterless constructor should be invalid.", false, svp.isValid());	 
  }

  void testAssigment()
  {
    SingleValueParameter A;
    SingleValueParameter B(2);
    A = B;
    TSM_ASSERT_EQUALS("Assigned Parameter getValue() is not correct.", 2, A.getValue());
    TSM_ASSERT_EQUALS("Assigned Parameter isValid() is not same as original.", B.isValid(), A.isValid() );
  }

  void testClone()
  {
    SingleValueParameter original(2);
    boost::scoped_ptr<SingleValueParameter> cloned(original.clone());

    TSM_ASSERT_EQUALS("Cloned Parameter getValue() is not same as original.", 2, cloned->getValue() );
    TSM_ASSERT_EQUALS("Cloned Parameter isValid() is not same as original.", original.isValid(), cloned->isValid() );
  }

  void testCopy()
  {
    SingleValueParameter original(2);
    SingleValueParameter copy(original);

    TSM_ASSERT_EQUALS("Copied Parameter getValue() is not same as original.", 2, copy.getValue() );
    TSM_ASSERT_EQUALS("Copied Parameter isValid() is not same as original.", original.isValid(), copy.isValid() );
  }

  void testToXML()
  {
    SingleValueParameter svp(1);
    std::string expectation = "<Parameter><Type>" + svp.getName() + "</Type><Value>1.0000</Value></Parameter>";
    TSM_ASSERT_EQUALS("The generated xml for the Parameter does not match the specification.", expectation, svp.toXMLString());
  }

  void testEqual()
  {
    SingleValueParameter A(2);
    SingleValueParameter B(2);

    TSM_ASSERT_EQUALS("The two parameter instances are not considered equal, but should be.", A, B);
  }

  void testNotEqual()
  {
    SingleValueParameter A(2);
    SingleValueParameter B(1);

    TSM_ASSERT_DIFFERS("The two parameter instances are considered equal, but should not be.", A, B);
  }
};

#endif