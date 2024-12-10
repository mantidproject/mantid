// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2009 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
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
    const ReadMaterial::MaterialParameters params = [this]() -> auto {
      ReadMaterial::MaterialParameters setMaterial;
      setMaterial.chemicalSymbol = FORMULA;
      setMaterial.atomicNumber = 0;
      setMaterial.massNumber = 0;
      return setMaterial;
    }();

    auto result = ReadMaterial::validateInputs(params);

    TS_ASSERT(result.empty());
  }

  void testSuccessfullValidateInputsAtomicNumber() {
    const ReadMaterial::MaterialParameters params = []() -> auto {
      ReadMaterial::MaterialParameters setMaterial;
      setMaterial.atomicNumber = 1;
      setMaterial.massNumber = 1;
      return setMaterial;
    }();

    auto result = ReadMaterial::validateInputs(params);

    TS_ASSERT(result.empty());
  }

  void testFailureValidateInputsFormulaPlusAtomicNumber() {
    const ReadMaterial::MaterialParameters params = [this]() -> auto {
      ReadMaterial::MaterialParameters setMaterial;
      setMaterial.chemicalSymbol = FORMULA;
      setMaterial.atomicNumber = 1;
      setMaterial.massNumber = 1;
      return setMaterial;
    }();

    auto result = ReadMaterial::validateInputs(params);

    TS_ASSERT_EQUALS(result["AtomicNumber"], "Cannot specify both ChemicalFormula and AtomicNumber")
  }

  void testFailureValidateInputsNoCoherentXSection() {
    const ReadMaterial::MaterialParameters params = []() -> auto {
      ReadMaterial::MaterialParameters setMaterial;
      setMaterial.atomicNumber = 0;
      setMaterial.massNumber = 0;
      setMaterial.incoherentXSection = 1.;
      setMaterial.attenuationXSection = 1.;
      setMaterial.scatteringXSection = 1.;
      setMaterial.numberDensity = 1.;
      return setMaterial;
    }();

    auto result = ReadMaterial::validateInputs(params);
    TS_ASSERT_EQUALS(result.size(), 1)
    TS_ASSERT_EQUALS(result["CoherentXSection"], "The cross section must be specified when "
                                                 "no ChemicalFormula or AtomicNumber is "
                                                 "given.")
  }

  void testFailureValidateInputsNoIncoherentXSection() {
    const ReadMaterial::MaterialParameters params = []() -> auto {
      ReadMaterial::MaterialParameters setMaterial;
      setMaterial.atomicNumber = 0;
      setMaterial.massNumber = 0;
      setMaterial.coherentXSection = 1.;
      setMaterial.attenuationXSection = 1.;
      setMaterial.scatteringXSection = 1.;
      setMaterial.numberDensity = 1.;
      return setMaterial;
    }();

    auto result = ReadMaterial::validateInputs(params);
    TS_ASSERT_EQUALS(result.size(), 1)
    TS_ASSERT_EQUALS(result["IncoherentXSection"], "The cross section must be specified when "
                                                   "no ChemicalFormula or AtomicNumber is "
                                                   "given.")
  }
  void testFailureValidateInputsNoAttenuationXSection() {
    const ReadMaterial::MaterialParameters params = []() -> auto {
      ReadMaterial::MaterialParameters setMaterial;
      setMaterial.atomicNumber = 0;
      setMaterial.massNumber = 0;
      setMaterial.coherentXSection = 1.;
      setMaterial.incoherentXSection = 1.;
      setMaterial.scatteringXSection = 1.;
      setMaterial.numberDensity = 1.;
      return setMaterial;
    }();

    auto result = ReadMaterial::validateInputs(params);
    TS_ASSERT_EQUALS(result.size(), 1)
    TS_ASSERT_EQUALS(result["AttenuationXSection"], "The cross section must be specified when "
                                                    "no ChemicalFormula or AtomicNumber is "
                                                    "given.")
  }
  void testFailureValidateInputsNoScatteringXSection() {
    const ReadMaterial::MaterialParameters params = []() -> auto {
      ReadMaterial::MaterialParameters setMaterial;
      setMaterial.atomicNumber = 0;
      setMaterial.massNumber = 0;
      setMaterial.coherentXSection = 1.;
      setMaterial.incoherentXSection = 1.;
      setMaterial.attenuationXSection = 1.;
      setMaterial.numberDensity = 1.;
      return setMaterial;
    }();

    auto result = ReadMaterial::validateInputs(params);
    TS_ASSERT_EQUALS(result.size(), 1)
    TS_ASSERT_EQUALS(result["ScatteringXSection"], "The cross section must be specified when "
                                                   "no ChemicalFormula or AtomicNumber is "
                                                   "given.")
  }
  void testFailureValidateInputsNoNumberDensityParams() {
    const ReadMaterial::MaterialParameters params = []() -> auto {
      ReadMaterial::MaterialParameters setMaterial;
      setMaterial.atomicNumber = 0;
      setMaterial.massNumber = 0;
      setMaterial.coherentXSection = 1.;
      setMaterial.incoherentXSection = 1.;
      setMaterial.attenuationXSection = 1.;
      setMaterial.scatteringXSection = 1.;
      return setMaterial;
    }();

    auto result = ReadMaterial::validateInputs(params);
    TS_ASSERT_EQUALS(result.size(), 1)
    TS_ASSERT_EQUALS(result["NumberDensity"],
                     "The number density or effective number density or Z Parameter\\Unit Cell Volume must "
                     " be specified with a user-defined material")
  }

  void testSuccessfullValidateInputsSampleNumber() {
    const ReadMaterial::MaterialParameters params = []() -> auto {
      ReadMaterial::MaterialParameters setMaterial;
      setMaterial.atomicNumber = 1;
      setMaterial.massNumber = 1;
      setMaterial.numberDensity = 1;
      return setMaterial;
    }();

    auto result = ReadMaterial::validateInputs(params);

    TS_ASSERT(result.empty());
  }

  void testSuccessfullValidateInputsZParam() {
    const ReadMaterial::MaterialParameters params = []() -> auto {
      ReadMaterial::MaterialParameters setMaterial;
      setMaterial.atomicNumber = 1;
      setMaterial.massNumber = 1;
      setMaterial.zParameter = 1;
      setMaterial.unitCellVolume = 1;
      return setMaterial;
    }();

    auto result = ReadMaterial::validateInputs(params);

    TS_ASSERT(result.empty());
  }

  void testSuccessfullValidateInputsMass() {
    const ReadMaterial::MaterialParameters params = []() -> auto {
      ReadMaterial::MaterialParameters setMaterial;
      setMaterial.atomicNumber = 1;
      setMaterial.massNumber = 1;
      setMaterial.massDensity = 1;
      return setMaterial;
    }();

    auto result = ReadMaterial::validateInputs(params);

    TS_ASSERT(result.empty());
  }

  void testSuccessfulValidateInputsNumberAndZParam() {
    const ReadMaterial::MaterialParameters params = []() -> auto {
      ReadMaterial::MaterialParameters setMaterial;
      setMaterial.atomicNumber = 1;
      setMaterial.massNumber = 1;
      setMaterial.numberDensity = 1;
      setMaterial.zParameter = 1;
      setMaterial.unitCellVolume = 1;
      return setMaterial;
    }();

    auto result = ReadMaterial::validateInputs(params);

    TS_ASSERT(result.empty());
  }

  void testFailureValidateInputsNumbersAndPacking() {
    const ReadMaterial::MaterialParameters params = []() -> auto {
      ReadMaterial::MaterialParameters setMaterial;
      setMaterial.atomicNumber = 1;
      setMaterial.numberDensity = 1;
      setMaterial.numberDensityEffective = 1;
      setMaterial.packingFraction = 1;
      return setMaterial;
    }();

    auto result = ReadMaterial::validateInputs(params);

    TS_ASSERT_EQUALS(result["NumberDensity"], "Number Density cannot be determined when "
                                              "both the effective number density and "
                                              "packing fraction are set. Only two can "
                                              "be specified at most.")
  }

  void testFailureValidateInputsEffectiveWithMass() {
    const ReadMaterial::MaterialParameters params = []() -> auto {
      ReadMaterial::MaterialParameters setMaterial;
      setMaterial.atomicNumber = 1;
      setMaterial.massDensity = 1;
      setMaterial.numberDensityEffective = 1;
      return setMaterial;
    }();

    auto result = ReadMaterial::validateInputs(params);

    TS_ASSERT_EQUALS(result["EffectiveNumberDensity"], "Cannot set effective number density when the mass density "
                                                       "is specified. The value specified will be overwritten "
                                                       "because it will be computed from the mass density.");
  }

  void testFailureValidateInputsLargePackingFrac() {
    const ReadMaterial::MaterialParameters params = []() -> auto {
      ReadMaterial::MaterialParameters setMaterial;
      setMaterial.atomicNumber = 1;
      setMaterial.massNumber = 1;
      setMaterial.packingFraction = 2.1;
      setMaterial.zParameter = 1;
      setMaterial.unitCellVolume = 1;
      return setMaterial;
    }();

    auto result = ReadMaterial::validateInputs(params);

    TS_ASSERT_EQUALS(result["PackingFraction"], "Cannot have a packing fraction larger than 2")
  }

  void testFailureValidateInputsNegativePackingFrac() {
    const ReadMaterial::MaterialParameters params = []() -> auto {
      ReadMaterial::MaterialParameters setMaterial;
      setMaterial.atomicNumber = 1;
      setMaterial.massNumber = 1;
      setMaterial.packingFraction = -1;
      setMaterial.zParameter = 1;
      setMaterial.unitCellVolume = 1;
      return setMaterial;
    }();

    auto result = ReadMaterial::validateInputs(params);

    TS_ASSERT_EQUALS(result["PackingFraction"], "Cannot have a packing fraction less than 0")
  }

  void testSuccessfulValidateInputsPackingFracElementMaterial() {
    const ReadMaterial::MaterialParameters params = []() -> auto {
      ReadMaterial::MaterialParameters setMaterial;
      setMaterial.atomicNumber = 1;
      setMaterial.packingFraction = 1;
      return setMaterial;
    }();

    auto result = ReadMaterial::validateInputs(params);

    TS_ASSERT(result.empty())
  }

  void testFailureValidateInputsPackingWithAll() {
    const ReadMaterial::MaterialParameters params = []() -> auto {
      ReadMaterial::MaterialParameters setMaterial;
      setMaterial.atomicNumber = 1;
      setMaterial.zParameter = 1;
      setMaterial.unitCellVolume = 1;
      setMaterial.massDensity = 1;
      setMaterial.packingFraction = 1;
      return setMaterial;
    }();

    auto result = ReadMaterial::validateInputs(params);

    TS_ASSERT_EQUALS(result["PackingFraction"], "Cannot set packing fraction when both the number density "
                                                "and effective number density are determined from "
                                                "the mass density and cell volume + zParameter.")
  }

  void testSuccessfulValidateInputsPackingFracOnly() {
    const ReadMaterial::MaterialParameters params = []() -> auto {
      ReadMaterial::MaterialParameters setMaterial;
      setMaterial.packingFraction = 1;
      setMaterial.zParameter = 1;
      setMaterial.unitCellVolume = 1;
      setMaterial.coherentXSection = 1.;
      setMaterial.incoherentXSection = 1.;
      setMaterial.attenuationXSection = 1.;
      setMaterial.scatteringXSection = 1.;
      return setMaterial;
    }();

    auto result = ReadMaterial::validateInputs(params);

    // Should succeed because number density can be determined from
    //  the zParameter and unitCellVolume
    TS_ASSERT(result.empty());
  }

  void testSuccessfulValidateInputsZParamWithMass() {
    const ReadMaterial::MaterialParameters params = []() -> auto {
      ReadMaterial::MaterialParameters setMaterial;
      setMaterial.atomicNumber = 1;
      setMaterial.massNumber = 1;
      setMaterial.zParameter = 1;
      setMaterial.unitCellVolume = 1;
      setMaterial.massDensity = 1;
      return setMaterial;
    }();

    auto result = ReadMaterial::validateInputs(params);

    TS_ASSERT(result.empty());
  }

  void testFailureValidateInputsZParamWithoutUnitCell() {
    const ReadMaterial::MaterialParameters params = []() -> auto {
      ReadMaterial::MaterialParameters setMaterial;
      setMaterial.atomicNumber = 1;
      setMaterial.massNumber = 1;
      setMaterial.zParameter = 1;
      return setMaterial;
    }();

    auto result = ReadMaterial::validateInputs(params);

    TS_ASSERT_EQUALS(result["UnitCellVolume"], "UnitCellVolume must be provided with ZParameter")
  }

  void testSuccessfulValidateInputsNumWithMass() {
    const ReadMaterial::MaterialParameters params = []() -> auto {
      ReadMaterial::MaterialParameters setMaterial;
      setMaterial.atomicNumber = 1;
      setMaterial.massNumber = 1;
      setMaterial.numberDensity = 1;
      setMaterial.massDensity = 1;
      return setMaterial;
    }();

    auto result = ReadMaterial::validateInputs(params);

    TS_ASSERT(result.empty());
  }

  void testMaterialIsCorrect() {
    const ReadMaterial::MaterialParameters params = [this]() -> auto {
      ReadMaterial::MaterialParameters setMaterial;
      setMaterial.chemicalSymbol = FORMULA;
      setMaterial.numberDensity = 1;
      setMaterial.numberDensityEffective = 1;
      setMaterial.coherentXSection = 1;
      setMaterial.incoherentXSection = 2;
      setMaterial.attenuationXSection = 3;
      setMaterial.scatteringXSection = 4;
      return setMaterial;
    }();

    ReadMaterial reader;
    reader.setMaterialParameters(params);
    auto material = reader.buildMaterial();

    Kernel::MaterialBuilder builder;
    builder.setFormula(FORMULA);
    builder.setNumberDensity(1);
    builder.setCoherentXSection(1);
    builder.setIncoherentXSection(2);
    builder.setAbsorptionXSection(3);
    builder.setTotalScatterXSection(4);
    auto check = builder.build();

    compareMaterial(*material, check);
  }

  void testGenerateScatteringInfo() {
    const ReadMaterial::MaterialParameters params = [this]() -> auto {
      ReadMaterial::MaterialParameters setMaterial;
      setMaterial.chemicalSymbol = FORMULA;
      setMaterial.numberDensity = 1;
      return setMaterial;
    }();

    ReadMaterial reader;
    reader.setMaterialParameters(params);
    auto material = reader.buildMaterial();

    Kernel::MaterialBuilder builder;
    builder.setFormula(FORMULA);
    builder.setNumberDensity(1);
    builder.setCoherentXSection(0.0184000000);
    builder.setIncoherentXSection(5.0800000022);
    builder.setAbsorptionXSection(5.0800000022);
    builder.setTotalScatterXSection(5.1000000044);
    auto check = builder.build();

    compareMaterial(*material, check);
  }

  void testCalculateDensity() {
    // test getting the number density from the table
    const ReadMaterial::MaterialParameters tableParams = [this]() -> auto {
      ReadMaterial::MaterialParameters setMaterial;
      setMaterial.chemicalSymbol = FORMULA;
      return setMaterial;
    }();

    {
      ReadMaterial reader;
      reader.setMaterialParameters(tableParams);
      auto material = reader.buildMaterial();

      TS_ASSERT_DELTA(material->numberDensity(), NUMBER_DENSITY, 1e-7);
    }

    // test getting the number density from the mass density
    const ReadMaterial::MaterialParameters massParams = [this]() -> auto {
      ReadMaterial::MaterialParameters setMaterial;
      setMaterial.chemicalSymbol = FORMULA;
      setMaterial.massDensity = MASS_DENSITY;
      return setMaterial;
    }();

    {
      ReadMaterial reader;
      reader.setMaterialParameters(massParams);
      auto material = reader.buildMaterial();
      TS_ASSERT_DELTA(material->numberDensity(), NUMBER_DENSITY, 1e-7);
    }

    // test getting the number density from the mass and volume
    const ReadMaterial::MaterialParameters params = [this]() -> auto {
      ReadMaterial::MaterialParameters setMaterial;
      setMaterial.chemicalSymbol = FORMULA;
      setMaterial.mass = 5.; // grams
      setMaterial.volume = setMaterial.mass / MASS_DENSITY;
      return setMaterial;
    }();

    {
      ReadMaterial reader;
      reader.setMaterialParameters(params);
      auto material = reader.buildMaterial();
      TS_ASSERT_DELTA(material->numberDensity(), NUMBER_DENSITY, 1e-7);
    }
  }

  void testNoMaterialFailure() {
    const ReadMaterial::MaterialParameters params = [this]() -> auto {
      ReadMaterial::MaterialParameters setMaterial;
      setMaterial.chemicalSymbol = EMPTY;
      setMaterial.atomicNumber = 0;
      setMaterial.massNumber = 0;
      setMaterial.numberDensity = 1;
      setMaterial.zParameter = EMPTY_DOUBLE_VAL;
      setMaterial.unitCellVolume = EMPTY_DOUBLE_VAL;
      setMaterial.massDensity = EMPTY_DOUBLE_VAL;
      return setMaterial;
    }();

    ReadMaterial reader;
    reader.setMaterialParameters(params);
    TS_ASSERT_THROWS(reader.buildMaterial(), const std::runtime_error &);
  }

private:
  // these values are for elemental vanadium
  const double EMPTY_DOUBLE_VAL{8.9884656743115785e+307};
  const double PRECISION{1e-8};
  const double NUMBER_DENSITY{0.0722305};
  const double MASS_DENSITY{6.11}; // matches the number density
  const std::string EMPTY{""};
  const std::string FORMULA{"V"};

  void compareMaterial(const Material &material, const Material &check) {
    std::vector<Material::FormulaUnit> checkFormula = check.chemicalFormula();
    std::vector<Material::FormulaUnit> materialFormula = material.chemicalFormula();

    TS_ASSERT_EQUALS(material.numberDensity(), check.numberDensity());
    TS_ASSERT_EQUALS(material.numberDensityEffective(), check.numberDensityEffective())
    TS_ASSERT_EQUALS(material.packingFraction(), check.packingFraction())
    TS_ASSERT_DELTA(material.cohScatterXSection(), check.cohScatterXSection(), PRECISION);
    TS_ASSERT_DELTA(material.incohScatterXSection(), check.incohScatterXSection(), PRECISION);
    TS_ASSERT_DELTA(material.absorbXSection(), check.absorbXSection(), PRECISION);
    TS_ASSERT_DELTA(material.totalScatterXSection(), check.totalScatterXSection(), PRECISION);
    TS_ASSERT_EQUALS(checkFormula[0].multiplicity, materialFormula[0].multiplicity);
    TS_ASSERT_EQUALS(checkFormula.size(), materialFormula.size())
  }
};
