#ifndef MANTID_API_MUPARSERUTILSTEST_H_
#define MANTID_API_MUPARSERUTILSTEST_H_

#include "MantidAPI/MuParserUtils.h"
#include <cxxtest/TestSuite.h>

using namespace Mantid::API;

class MuParserUtilsTest : public CxxTest::TestSuite {
public:
  static MuParserUtilsTest *createSuite() { return new MuParserUtilsTest(); }
  static void destroySuite(MuParserUtilsTest *suite) { delete suite; }

  void test_addDefaultConstants_Only_MUPARSER_CONSTANTS_IsAdded() {
    mu::Parser parser;
    MuParserUtils::addDefaultConstants(parser);
    MuParserUtils::extraOneVarFunctions(parser);
    TS_ASSERT(defaultConstantsDefined(parser));
    TS_ASSERT(extraOneVarFunctionsDefined(parser));
    TS_ASSERT(noVariablesDefined(parser));
  }

private:
  static bool extraOneVarFunctionsDefined(const mu::Parser &parser) {
    const auto functionMap = parser.GetFunDef();
    for (const auto pair : MuParserUtils::MUPARSER_ONEVAR_FUNCTIONS) {
      const auto iterator = functionMap.find(pair.first);
      if (iterator == functionMap.end()) {
        return false;
      }
      if (iterator->first != pair.first) {
        return false;
      }
    }
    return true;
  }

  static bool defaultConstantsDefined(const mu::Parser &parser) {
    const auto constantMap = parser.GetConst();
    // muParser defines two extra constants: "_e" and "_pi".
    size_t extraConstants = 0;
    if (constantMap.find("_e") != constantMap.end()) {
      ++extraConstants;
    }
    if (constantMap.find("_pi") != constantMap.end()) {
      ++extraConstants;
    }
    if (constantMap.size() - extraConstants !=
        MuParserUtils::MUPARSER_CONSTANTS.size()) {
      return false;
    }
    // Note: the keys in constantMap are values in MUPARSER_CONSTANTS and
    // vice versa.
    for (const auto constant : MuParserUtils::MUPARSER_CONSTANTS) {
      const auto iterator = constantMap.find(constant.second);
      if (iterator == constantMap.end()) {
        return false;
      }
      if (iterator->second != constant.first) {
        return false;
      }
    }

    return true;
  }

  static bool noVariablesDefined(const mu::Parser &parser) {
    return parser.GetVar().size() == 0;
  }
};

#endif /* MANTID_API_MUPARSERUTILSTEST_H_ */
