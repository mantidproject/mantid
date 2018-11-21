// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2009 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#include "MantidDataHandling/ReadMaterial.h"
#include "MantidKernel/Atom.h"
#include "MantidKernel/Material.h"
#include <cxxtest/TestSuite.h>

using namespace Mantid;
using namespace Mantid::Kernel;
using namespace Mantid::DataHandling;

class ReadMaterialTest : public CxxTest::TestSuite {
public:
  static ReadMaterialTest *createSuite() { return new ReadMaterialTest(); }
  static void destroySuite(ReadMaterialTest *suite) { delete suite; }

  void testSuccessfullValidateInputsFormula() {
    ReadMaterial::MaterialParameters params = [this]() -> auto {
      ReadMaterial::MaterialParameters setMaterial;
      setMaterial.chemicalSymbol = FORMULA;
      setMaterial.atomicNumber = 0;
      setMaterial.massNumber = 0;
      setMaterial.sampleNumberDensity = EMPTY_DOUBLE_VAL;
      setMaterial.zParameter = EMPTY_DOUBLE_VAL;
      setMaterial.unitCellVolume = EMPTY_DOUBLE_VAL;
      setMaterial.sampleMassDensity = EMPTY_DOUBLE_VAL;
      return setMaterial;
    }
    ();
    auto result = ReadMaterial::validateInputs(params);
    TS_ASSERT(result.empty());
  }

  void testSuccessfullValidateInputsAtomicNumber() {
    ReadMaterial::MaterialParameters params = [this]() -> auto {
      ReadMaterial::MaterialParameters setMaterial;
      setMaterial.chemicalSymbol = EMPTY;
      setMaterial.atomicNumber = 1;
      setMaterial.massNumber = 1;
      setMaterial.sampleNumberDensity = EMPTY_DOUBLE_VAL;
      setMaterial.zParameter = EMPTY_DOUBLE_VAL;
      setMaterial.unitCellVolume = EMPTY_DOUBLE_VAL;
      setMaterial.sampleMassDensity = EMPTY_DOUBLE_VAL;
      return setMaterial;
    }
    ();
    auto result = ReadMaterial::validateInputs(params);
    TS_ASSERT(result.empty());
  }

  void testFailureValidateInputsFormulaPlusAtomicNumber() {
    ReadMaterial::MaterialParameters params = [this](
        const auto FORMULA, const auto EMPTY_DOUBLE_VAL) -> auto {
      ReadMaterial::MaterialParameters setMaterial;
      setMaterial.chemicalSymbol = FORMULA;
      setMaterial.atomicNumber = 1;
      setMaterial.massNumber = 1;
      setMaterial.sampleNumberDensity = EMPTY_DOUBLE_VAL;
      setMaterial.zParameter = EMPTY_DOUBLE_VAL;
      setMaterial.unitCellVolume = EMPTY_DOUBLE_VAL;
      setMaterial.sampleMassDensity = EMPTY_DOUBLE_VAL;
      return setMaterial;
    }
    (FORMULA, EMPTY_DOUBLE_VAL);
    auto result = ReadMaterial::validateInputs(params);
    TS_ASSERT_EQUALS(result["AtomicNumber"],
                     "Cannot specify both ChemicalFormula and AtomicNumber")
  }

  void testFailureValidateInputsNoMaterial() {
    ReadMaterial::MaterialParameters params = [this]() -> auto {
      ReadMaterial::MaterialParameters setMaterial;
      setMaterial.chemicalSymbol = EMPTY;
      setMaterial.atomicNumber = 0;
      setMaterial.massNumber = 0;
      setMaterial.sampleNumberDensity = EMPTY_DOUBLE_VAL;
      setMaterial.zParameter = EMPTY_DOUBLE_VAL;
      setMaterial.unitCellVolume = EMPTY_DOUBLE_VAL;
      setMaterial.sampleMassDensity = EMPTY_DOUBLE_VAL;
      return setMaterial;
    }
    ();
    auto result = ReadMaterial::validateInputs(params);
    TS_ASSERT_EQUALS(result["ChemicalFormula"], "Need to specify the material")
  }

  void testSuccessfullValidateInputsSampleNumber() {
    ReadMaterial::MaterialParameters params = [this]() -> auto {
      ReadMaterial::MaterialParameters setMaterial;
      setMaterial.chemicalSymbol = EMPTY;
      setMaterial.atomicNumber = 1;
      setMaterial.massNumber = 1;
      setMaterial.sampleNumberDensity = 1;
      setMaterial.zParameter = EMPTY_DOUBLE_VAL;
      setMaterial.unitCellVolume = EMPTY_DOUBLE_VAL;
      setMaterial.sampleMassDensity = EMPTY_DOUBLE_VAL;
      return setMaterial;
    }
    ();
    auto result = ReadMaterial::validateInputs(params);
    TS_ASSERT(result.empty());
  }

  void testSuccessfullValidateInputsZParam() {
    ReadMaterial::MaterialParameters params = [this]() -> auto {
      ReadMaterial::MaterialParameters setMaterial;
      setMaterial.chemicalSymbol = EMPTY;
      setMaterial.atomicNumber = 1;
      setMaterial.massNumber = 1;
      setMaterial.sampleNumberDensity = EMPTY_DOUBLE_VAL;
      setMaterial.zParameter = 1;
      setMaterial.unitCellVolume = 1;
      setMaterial.sampleMassDensity = EMPTY_DOUBLE_VAL;
      return setMaterial;
    }
    ();
    auto result = ReadMaterial::validateInputs(params);
    TS_ASSERT(result.empty());
  }

  void testSuccessfullValidateInputsSampleMass() {
    ReadMaterial::MaterialParameters params = [this]() -> auto {
      ReadMaterial::MaterialParameters setMaterial;
      setMaterial.chemicalSymbol = EMPTY;
      setMaterial.atomicNumber = 1;
      setMaterial.massNumber = 1;
      setMaterial.sampleNumberDensity = EMPTY_DOUBLE_VAL;
      setMaterial.zParameter = EMPTY_DOUBLE_VAL;
      setMaterial.unitCellVolume = EMPTY_DOUBLE_VAL;
      setMaterial.sampleMassDensity = 1;
      return setMaterial;
    }
    ();
    auto result = ReadMaterial::validateInputs(params);
    TS_ASSERT(result.empty());
  }

  void testFailureValidateInputsSampleNumberAndZParam() {
    ReadMaterial::MaterialParameters params = [this]() -> auto {
      ReadMaterial::MaterialParameters setMaterial;
      setMaterial.chemicalSymbol = EMPTY;
      setMaterial.atomicNumber = 1;
      setMaterial.massNumber = 1;
      setMaterial.sampleNumberDensity = 1;
      setMaterial.zParameter = 1;
      setMaterial.unitCellVolume = 1;
      setMaterial.sampleMassDensity = EMPTY_DOUBLE_VAL;
      return setMaterial;
    }
    ();
    auto result = ReadMaterial::validateInputs(params);
    TS_ASSERT_EQUALS(result["ZParameter"],
                     "Can not give ZParameter with SampleNumberDensity set")
  }

  void testFailureValidateInputsZParamWithSampleMass() {
    ReadMaterial::MaterialParameters params = [this]() -> auto {
      ReadMaterial::MaterialParameters setMaterial;
      setMaterial.chemicalSymbol = EMPTY;
      setMaterial.atomicNumber = 1;
      setMaterial.massNumber = 1;
      setMaterial.sampleNumberDensity = EMPTY_DOUBLE_VAL;
      setMaterial.zParameter = 1;
      setMaterial.unitCellVolume = 1;
      setMaterial.sampleMassDensity = 1;
      return setMaterial;
    }
    ();
    auto result = ReadMaterial::validateInputs(params);
    TS_ASSERT_EQUALS(result["SampleMassDensity"],
                     "Can not give SampleMassDensity with ZParameter set")
  }

  void testFailureValidateInputsZParamWithoutUnitCell() {
    ReadMaterial::MaterialParameters params = [this]() -> auto {
      ReadMaterial::MaterialParameters setMaterial;
      setMaterial.chemicalSymbol = EMPTY;
      setMaterial.atomicNumber = 1;
      setMaterial.massNumber = 1;
      setMaterial.sampleNumberDensity = EMPTY_DOUBLE_VAL;
      setMaterial.zParameter = 1;
      setMaterial.unitCellVolume = EMPTY_DOUBLE_VAL;
      setMaterial.sampleMassDensity = EMPTY_DOUBLE_VAL;
      return setMaterial;
    }
    ();
    auto result = ReadMaterial::validateInputs(params);
    TS_ASSERT_EQUALS(result["UnitCellVolume"],
                     "UnitCellVolume must be provided with ZParameter")
  }

  void testFailureValidateInputsSampleNumWithSampleMass() {
    ReadMaterial::MaterialParameters params = [this]() -> auto {
      ReadMaterial::MaterialParameters setMaterial;
      setMaterial.chemicalSymbol = EMPTY;
      setMaterial.atomicNumber = 1;
      setMaterial.massNumber = 1;
      setMaterial.sampleNumberDensity = 1;
      setMaterial.zParameter = EMPTY_DOUBLE_VAL;
      setMaterial.unitCellVolume = EMPTY_DOUBLE_VAL;
      setMaterial.sampleMassDensity = 1;
      return setMaterial;
    }
    ();
    auto result = ReadMaterial::validateInputs(params);
    TS_ASSERT_EQUALS(
        result["SampleMassDensity"],
        "Can not give SampleMassDensity with SampleNumberDensity set")
  }

  void testMaterialIsCorrect() {
    ReadMaterial reader;
    reader.setMaterial(FORMULA, 0, 0);
    reader.setNumberDensity(EMPTY_DOUBLE_VAL, 1, EMPTY_DOUBLE_VAL,
                            EMPTY_DOUBLE_VAL);
    reader.setScatteringInfo(1, 2, 3, 4);
    auto material = reader.buildMaterial();
    compareMaterial(material, 1, 1, 2, 3, 4, FORMULA);
  }

  void testGenerateScatteringInfo() {
    ReadMaterial reader;
    reader.setMaterial(FORMULA, 0, 0);
    reader.setNumberDensity(EMPTY_DOUBLE_VAL, 1, EMPTY_DOUBLE_VAL,
                            EMPTY_DOUBLE_VAL);
    auto material = reader.buildMaterial();
    compareMaterial(material, 1, 0.0184000000, 5.0800000022, 5.0800000022,
                    5.1000000044, FORMULA);
  }

  void testNoMaterialFailure() {
    ReadMaterial reader;
    reader.setMaterial(EMPTY, 0, 0);
    reader.setNumberDensity(EMPTY_DOUBLE_VAL, 1, EMPTY_DOUBLE_VAL,
                            EMPTY_DOUBLE_VAL);
    reader.setScatteringInfo(EMPTY_DOUBLE_VAL, EMPTY_DOUBLE_VAL,
                             EMPTY_DOUBLE_VAL, EMPTY_DOUBLE_VAL);
    TS_ASSERT_THROWS(reader.buildMaterial(), std::runtime_error);
  }

private:
  const double EMPTY_DOUBLE_VAL = 8.9884656743115785e+307;
  const std::string EMPTY = "";
  const std::string FORMULA = "V";

  void compareMaterial(std::unique_ptr<Material> &material,
                       const double numberDensity,
                       const double coherentXSection,
                       const double incoherentXSection,
                       const double absorbXSection, const double totalXSection,
                       const std::string &formula) {
    std::vector<Material::FormulaUnit> checkFormula =
        Material::parseChemicalFormula(formula);
    std::vector<Material::FormulaUnit> materialFormula =
        material->chemicalFormula();
    TS_ASSERT_EQUALS(material->numberDensity(), numberDensity);
    TS_ASSERT_DELTA(material->cohScatterXSection(), coherentXSection,
                    0.00000001);
    TS_ASSERT_DELTA(material->incohScatterXSection(), incoherentXSection,
                    0.00000001);
    TS_ASSERT_DELTA(material->absorbXSection(), absorbXSection, 0.00000001);
    TS_ASSERT_DELTA(material->totalScatterXSection(), totalXSection,
                    0.00000001);
    TS_ASSERT_EQUALS(checkFormula[0].multiplicity,
                     materialFormula[0].multiplicity);
    TS_ASSERT_EQUALS(checkFormula.size(), materialFormula.size())
  }
};