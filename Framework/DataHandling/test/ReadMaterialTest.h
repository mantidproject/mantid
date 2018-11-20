// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2009 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#include "MantidDataHandling/ReadMaterial.h"
#include <cxxtest/TestSuite.h>

using namespace Mantid;
using namespace Mantid::DataHandling;

class ReadMaterialTest : public CxxTest::TestSuite {
public:
  static ReadMaterialTest *createSuite() {
    return new ReadMaterialTest();
  }
  static void destroySuite(ReadMaterialTest *suite) { delete suite; }

  void testSuccessfullValidateInputsFormula(){
    auto result = ReadMaterial::validateInputs(FORMULA, 0, 0, EMPTY_DOUBLE_VAL, EMPTY_DOUBLE_VAL, EMPTY_DOUBLE_VAL, EMPTY_DOUBLE_VAL);
    TS_ASSERT(result.empty());
  }

  void testSuccessfullValidateInputsAtomicNumber(){
    auto result = ReadMaterial::validateInputs(EMPTY, 1, 1, EMPTY_DOUBLE_VAL, EMPTY_DOUBLE_VAL, EMPTY_DOUBLE_VAL, EMPTY_DOUBLE_VAL);
    TS_ASSERT(result.empty());
  }

  void testFailureValidateInputsFormulaPlusAtomicNumber(){
    auto result = ReadMaterial::validateInputs(FORMULA, 1, 1, EMPTY_DOUBLE_VAL, EMPTY_DOUBLE_VAL, EMPTY_DOUBLE_VAL, EMPTY_DOUBLE_VAL);
    TS_ASSERT_EQUALS(result["AtomicNumber"], "Cannot specify both ChemicalFormula and AtomicNumber")
  }

  void testFailureValidateInputsNoMaterial(){
    auto result = ReadMaterial::validateInputs(EMPTY, 0, 0, EMPTY_DOUBLE_VAL, EMPTY_DOUBLE_VAL, EMPTY_DOUBLE_VAL, EMPTY_DOUBLE_VAL);
    TS_ASSERT_EQUALS(result["ChemicalFormula"], "Need to specify the material")
  }

  void testSuccessfullValidateInputsSampleNumber(){
    auto result = ReadMaterial::validateInputs(EMPTY, 1, 1, 1, EMPTY_DOUBLE_VAL, EMPTY_DOUBLE_VAL, EMPTY_DOUBLE_VAL);
    TS_ASSERT(result.empty());
  }

  void testSuccessfullValidateInputsZParam(){
    auto result = ReadMaterial::validateInputs(EMPTY, 1, 1, EMPTY_DOUBLE_VAL, 1, 1, EMPTY_DOUBLE_VAL);
    TS_ASSERT(result.empty());
  }

  void testSuccessfullValidateInputsSampleMass(){
    auto result = ReadMaterial::validateInputs(EMPTY, 1, 1, EMPTY_DOUBLE_VAL, EMPTY_DOUBLE_VAL, EMPTY_DOUBLE_VAL, 1);
    TS_ASSERT(result.empty());
  }

  void testFailureValidateInputsSampleNumberAndZParam(){
    auto result = ReadMaterial::validateInputs(EMPTY, 0, 0, 1, 1, 1, EMPTY_DOUBLE_VAL);
    TS_ASSERT_EQUALS(result["ZParameter"], "Can not give ZParameter with SampleNumberDensity set")
  }

  void testFailureValidateInputsZParamWithSampleMass(){
    auto result = ReadMaterial::validateInputs(EMPTY, 0, 0, EMPTY_DOUBLE_VAL, 1, 1, 1);
    TS_ASSERT_EQUALS(result["SampleMassDensity"], "Can not give SampleMassDensity with ZParameter set")
  }

  void testFailureValidateInputsZParamWithoutUnitCell(){
    auto result = ReadMaterial::validateInputs(EMPTY, 0, 0, EMPTY_DOUBLE_VAL, 1, EMPTY_DOUBLE_VAL, EMPTY_DOUBLE_VAL);
    TS_ASSERT_EQUALS(result["UnitCellVolume"], "UnitCellVolume must be provided with ZParameter")
  }

  void testFailureValidateInputsSampleNumWithSampleMass(){
    auto result = ReadMaterial::validateInputs(EMPTY, 0, 0, 1,  EMPTY_DOUBLE_VAL,  EMPTY_DOUBLE_VAL, 1);
    TS_ASSERT_EQUALS(result["SampleMassDensity"], "Can not give SampleMassDensity with SampleNumberDensity set")
  }

  


private:
    const double EMPTY_DOUBLE_VAL = 8.9884656743115785e+307;
    const std::string EMPTY = "";
    const std::string FORMULA = "V";
};