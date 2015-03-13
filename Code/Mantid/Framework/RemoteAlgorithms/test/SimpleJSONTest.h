#ifndef MANTID_REMOTEALGORITHMS_SIMPLEJSONTEST_H_
#define MANTID_REMOTEALGORITHMS_SIMPLEJSONTEST_H_

#include <cxxtest/TestSuite.h>

#include "MantidRemoteAlgorithms/SimpleJSON.h"

class SimpleJSONTest : public CxxTest::TestSuite {
public:
  // This pair of boilerplate methods prevent the suite being created statically
  // This means the constructor isn't called when running other tests
  static SimpleJSONTest *createSuite() { return new SimpleJSONTest(); }
  static void destroySuite(SimpleJSONTest *suite) { delete suite; }

  void test_JSONValue() {
    bool b = true;
    TS_ASSERT_THROWS_NOTHING(JSONValue vBool(b));

    double d = 0.1;
    TS_ASSERT_THROWS_NOTHING(JSONValue vDbl(d));

    TS_ASSERT_THROWS_NOTHING(b = d);

    JSONValue vBool(b);
    bool getBool = false;
    TS_ASSERT_EQUALS(true, vBool.getValue(getBool));

    JSONValue vDbl(d);
    TS_ASSERT_THROWS_NOTHING(vBool = vDbl);
    TS_ASSERT_EQUALS(false, vBool.getValue(getBool));
    TS_ASSERT_EQUALS(true, getBool);
    TS_ASSERT_THROWS_NOTHING(vDbl = 0.0);
    TS_ASSERT_THROWS_NOTHING(vBool = vDbl);
    TS_ASSERT_EQUALS(false, vBool.getValue(getBool));
    TS_ASSERT_EQUALS(true, getBool);

    TS_ASSERT_THROWS_NOTHING(JSONValue str1(""));
    TS_ASSERT_THROWS_NOTHING(JSONValue str2("str"));

    JSONValue str1("s1");
    JSONValue str2("s2");
    TS_ASSERT_THROWS_NOTHING(str1 = str2);

    JSONValue vs;
    std::ostringstream out;
    TS_ASSERT_THROWS_NOTHING(vs.prettyPrint(out, 1));
  }

  void test_JSONArray() {
    std::string str = "json failure here";
    std::istringstream input(str);
    std::string res;

    JSONArray ja;
    TS_ASSERT_THROWS_NOTHING(ja.push_back(str));
    TS_ASSERT_THROWS_NOTHING(ja.push_back(str));
    JSONValue jv(ja);

    JSONValue vBool(true);
    TS_ASSERT_THROWS_NOTHING(vBool = ja);
  }

  void test_JSONObjectWrongStrings() {
    std::string str = "json failure here";
    std::istringstream input(str);
    std::string res;

    str = "";
    JSONObject jo;
    TS_ASSERT_THROWS(initFromStream(jo, input), JSONParseException);
    TS_ASSERT_THROWS_NOTHING(jo["no_param"].getValue(res));
    TS_ASSERT_EQUALS(false, jo["no_param"].getValue(res));
    TS_ASSERT_EQUALS(res, "");
    TS_ASSERT_EQUALS(false, jo["another_no_param"].getValue(res));
    TS_ASSERT_EQUALS(res, "");

    TS_ASSERT_THROWS(initFromStream(jo, input), JSONParseException);
    TS_ASSERT_THROWS_NOTHING(jo["doesnt_exist"].getValue(res));
    TS_ASSERT_EQUALS(false, jo["doesnt_exist"].getValue(res));
    TS_ASSERT_EQUALS(res, "");

    str = "{ mistake: }";
    TS_ASSERT_THROWS(initFromStream(jo, input), JSONParseException);
    TS_ASSERT_THROWS_NOTHING(jo["no no"].getValue(res));
    TS_ASSERT_EQUALS(false, jo["it's not here"].getValue(res));
    TS_ASSERT_EQUALS(res, "");

    str = "{ ";
    TS_ASSERT_THROWS(initFromStream(jo, input), JSONParseException);
    TS_ASSERT_THROWS_NOTHING(jo["no no"].getValue(res));
    TS_ASSERT_EQUALS(false, jo["it's not here"].getValue(res));
    TS_ASSERT_EQUALS(res, "");
  }

  void test_JSONObjectWrongSeparator() {
    const std::string wrongSep = ",";
    const std::string jsonStr = "{\"" + errName + "\":\"" + errVal + wrongSep +
                                "\"" + errName + "\":\"" + errVal + "\"}";
    std::istringstream input(jsonStr);
    std::string res;

    JSONObject o;
    TS_ASSERT_THROWS(initFromStream(o, input), JSONParseException);
    TS_ASSERT_THROWS_NOTHING(o["Err_Msg"].getValue(res));
    TS_ASSERT_EQUALS(false, o["Err_Msg"].getValue(res));
    TS_ASSERT_EQUALS(res, "");
  }

  void test_JSONObjectCorrectStrings() {
    const std::string name1 = "var1";
    const std::string val1 = "value1";
    const std::string name2 = "variable2";
    const std::string val2 = "[0,1,2,3]";
    const std::string sep = ",";
    std::string jsonStr = "{\"" + name1 + "\": \"" + val1 + "\"" + sep + " \"" +
                          name2 + "\": \"" + val2 + "\"}";
    std::istringstream input(jsonStr);

    JSONObject jo;
    std::string res;
    TS_ASSERT_THROWS_NOTHING(initFromStream(jo, input));
    TS_ASSERT_THROWS_NOTHING(jo[name1].getValue(res));
    TS_ASSERT_EQUALS(false, jo["missing var"].getValue(res));
    TS_ASSERT_EQUALS(res, val1);
    TS_ASSERT_EQUALS(false, jo["got ya"].getValue(res));
    TS_ASSERT_THROWS_NOTHING(jo[name2].getValue(res));
    TS_ASSERT_EQUALS(res, val2);
  }

  void test_JSONObjectExampleServerResponseSimple() {
    const std::string jsonStr = "{\"" + errName + "\":\"" + errVal + "\"}";
    std::istringstream input(jsonStr);
    std::string res;

    JSONObject o;
    TS_ASSERT_THROWS_NOTHING(initFromStream(o, input));
    TS_ASSERT_THROWS_NOTHING(o["doesnt_exist"].getValue(res));
    TS_ASSERT_EQUALS(false, o["doesnt_exist"].getValue(res));
    TS_ASSERT_THROWS_NOTHING(o[""].getValue(res));
    TS_ASSERT_EQUALS(false, o[""].getValue(res));
    TS_ASSERT_EQUALS(true, o[errName].getValue(res));
    TS_ASSERT_EQUALS(res, errVal);
  }

  void test_JSONObjectExampleServerResponseLonger() {

    const std::string longerJsonStr =
        "{\"v1\": \"[1, a, 3]\",\"" + errName + "\":\"" + errVal + "\"}";
    std::istringstream inputLong(longerJsonStr);
    std::string res;

    JSONObject ol;
    TS_ASSERT_THROWS_NOTHING(initFromStream(ol, inputLong));
    TS_ASSERT_THROWS_NOTHING(ol["doesnt exist"].getValue(res));
    TS_ASSERT_EQUALS(false, ol["doesnt exist"].getValue(res));
    TS_ASSERT_THROWS_NOTHING(ol[""].getValue(res));
    TS_ASSERT_EQUALS(false, ol[""].getValue(res));
    TS_ASSERT_EQUALS(true, ol[errName].getValue(res));
    TS_ASSERT_EQUALS(res, errVal);

    const std::string l2JsonStr = "{\"v1\": \"[1, a, 3]\",\"" + errName +
                                  "\":\"" + errVal + "\", \"" + versName +
                                  "\": \"" + versVal + "\" }"
                                                       "\"}";
    std::istringstream inputL2(l2JsonStr);

    TS_ASSERT_THROWS_NOTHING(initFromStream(ol, inputL2));
    TS_ASSERT_THROWS_NOTHING(ol["doesnt exist"].getValue(res));
    TS_ASSERT_EQUALS(false, ol["doesnt exist"].getValue(res));
    TS_ASSERT_THROWS_NOTHING(ol[""].getValue(res));
    TS_ASSERT_EQUALS(false, ol[""].getValue(res));
    TS_ASSERT_EQUALS(true, ol[errName].getValue(res));
    TS_ASSERT_EQUALS(res, errVal);
    TS_ASSERT_EQUALS(true, ol[versName].getValue(res));
    TS_ASSERT_EQUALS(res, versVal);

    const std::string l3JsonStr = "{ \"" + impName + "\": \"" + impVal +
                                  "\", \"v1\": \"[1, a, longer str, a4]\",\"" +
                                  errName + "\":\"" + errVal + "\", \"" +
                                  versName + "\": \"" + versVal + "\" }"
                                                                  "\"}";
    std::istringstream inputL3(l3JsonStr);

    TS_ASSERT_THROWS_NOTHING(initFromStream(ol, inputL3));
    TS_ASSERT_THROWS_NOTHING(ol["doesnt exist"].getValue(res));
    TS_ASSERT_EQUALS(false, ol["doesnt exist"].getValue(res));
    TS_ASSERT_THROWS_NOTHING(ol[""].getValue(res));
    TS_ASSERT_EQUALS(false, ol[""].getValue(res));
    TS_ASSERT_EQUALS(true, ol[errName].getValue(res));
    TS_ASSERT_EQUALS(res, errVal);
    TS_ASSERT_EQUALS(true, ol[versName].getValue(res));
    TS_ASSERT_EQUALS(res, versVal);
    TS_ASSERT_EQUALS(true, ol[impName].getValue(res));
    TS_ASSERT_EQUALS(res, impVal);
  }

  void test_prettyPrint() {
    std::ostringstream out;

    std::string str = "json failure here";
    std::istringstream istr(str);
    JSONObject jo;
    TS_ASSERT_THROWS(initFromStream(jo, istr), JSONParseException);
    TS_ASSERT_THROWS_NOTHING(prettyPrint(jo, out, 0));

    std::string strOK = "{ \"key1\": \"val1\"}";
    std::istringstream istrOK(strOK);
    JSONObject j2;
    TS_ASSERT_THROWS_NOTHING(initFromStream(j2, istrOK));
    TS_ASSERT_THROWS_NOTHING(prettyPrint(j2, out, 2));
  }

private:
  // these are example parameters used for the mantid web service
  // (remote job submission API:
  // http://www.mantidproject.org/Remote_Job_Submission_API)
  static const std::string errName;
  static const std::string errVal;
  static const std::string versName;
  static const std::string versVal;
  static const std::string impName;
  static const std::string impVal;
};

const std::string SimpleJSONTest::errName = "Err_Msg";
const std::string SimpleJSONTest::errVal = "fake msg";
const std::string SimpleJSONTest::versName = "API_Version";
const std::string SimpleJSONTest::versVal = "1";
const std::string SimpleJSONTest::impName =
    "Implementation_Specific_Post_Variables";
const std::string SimpleJSONTest::impVal = "example_POST_var1";

#endif // MANTID_REMOTEALGORITHMS_SIMPLEJSONTEST_H_
