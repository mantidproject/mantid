#include "MantidCurveFitting/PawleyFit.h"

#include "MantidAPI/FunctionFactory.h"
#include "MantidCurveFitting/PawleyFunction.h"
#include "MantidAPI/TableRow.h"

#include "MantidGeometry/Crystal/UnitCell.h"
#include "MantidKernel/ListValidator.h"

#include <algorithm>

namespace Mantid {
namespace CurveFitting {

using namespace API;
using namespace Kernel;
using namespace Geometry;

DECLARE_ALGORITHM(PawleyFit);

const std::string PawleyFit::summary() const {
  return "This algorithm performs a Pawley-fit on the supplied workspace.";
}

std::vector<V3D> PawleyFit::hklsFromString(const std::string &hklString) const {
  std::vector<std::string> hklStrings;
  boost::split(hklStrings, hklString, boost::is_any_of(";"));

  std::vector<V3D> hkls(hklStrings.size());
  for (size_t i = 0; i < hkls.size(); ++i) {
    std::istringstream strm(hklStrings[i]);
    strm >> hkls[i];
  }

  return hkls;
}

void PawleyFit::addHKLsToFunction(PawleyFunction_sptr &pawleyFn,
                                  const ITableWorkspace_sptr &tableWs) const {
  if (!tableWs || !pawleyFn) {
    throw std::invalid_argument("Can only process non-null function & table.");
  }

  pawleyFn->clearPeaks();

  for (size_t i = 0; i < tableWs->rowCount(); ++i) {
    TableRow currentRow = tableWs->getRow(i);

    try {
      V3D hkl = getHkl(currentRow.String(0));
      double d = boost::lexical_cast<double>(currentRow.String(1));
      double height = boost::lexical_cast<double>(currentRow.String(3));
      double fwhm = boost::lexical_cast<double>(currentRow.String(4)) * d;

      pawleyFn->addPeak(hkl, fwhm, height);
    }
    catch (...) {
      // do nothing.
    }
  }
}

V3D PawleyFit::getHkl(const std::string &hklString) const {
  std::vector<std::string> indicesStr;
  boost::split(indicesStr, hklString, boost::is_any_of(" "));

  if (indicesStr.size() != 3) {
    throw std::invalid_argument("Input string cannot be parsed as HKL.");
  }

  V3D hkl;
  hkl.setX(boost::lexical_cast<double>(indicesStr[0]));
  hkl.setY(boost::lexical_cast<double>(indicesStr[1]));
  hkl.setZ(boost::lexical_cast<double>(indicesStr[2]));

  return hkl;
}

ITableWorkspace_sptr
PawleyFit::getLatticeFromFunction(const PawleyFunction_sptr &pawleyFn) const {
  if (!pawleyFn) {
    throw std::invalid_argument(
        "Cannot extract lattice parameters from null function.");
  }

  ITableWorkspace_sptr latticeParameterTable =
      WorkspaceFactory::Instance().createTable();

  latticeParameterTable->addColumn("str", "Parameter");
  latticeParameterTable->addColumn("double", "Value");
  latticeParameterTable->addColumn("double", "Error");

  PawleyParameterFunction_sptr parameters =
      pawleyFn->getPawleyParameterFunction();

  for (size_t i = 0; i < parameters->nParams(); ++i) {
    TableRow newRow = latticeParameterTable->appendRow();
    newRow << parameters->parameterName(i) << parameters->getParameter(i)
           << parameters->getError(i);
  }

  return latticeParameterTable;
}

ITableWorkspace_sptr PawleyFit::getPeakParametersFromFunction(
    const PawleyFunction_sptr &pawleyFn) const {
  if (!pawleyFn) {
    throw std::invalid_argument(
        "Cannot extract peak parameters from null function.");
  }

  ITableWorkspace_sptr peakParameterTable =
      WorkspaceFactory::Instance().createTable();

  peakParameterTable->addColumn("int", "Peak");
  peakParameterTable->addColumn("V3D", "HKL");
  peakParameterTable->addColumn("str", "Parameter");
  peakParameterTable->addColumn("double", "Value");
  peakParameterTable->addColumn("double", "Error");

  for (size_t i = 0; i < pawleyFn->getPeakCount(); ++i) {

    IPeakFunction_sptr currentPeak = pawleyFn->getPeakFunction(i);

    int peakNumber = static_cast<int>(i + 1);
    V3D peakHKL = pawleyFn->getPeakHKL(i);

    for (size_t j = 0; j < currentPeak->nParams(); ++j) {
      TableRow newRow = peakParameterTable->appendRow();
      newRow << peakNumber << peakHKL << currentPeak->parameterName(j)
             << currentPeak->getParameter(j) << currentPeak->getError(j);
    }
  }

  return peakParameterTable;
}

void PawleyFit::init() {
  declareProperty(new WorkspaceProperty<MatrixWorkspace>("InputWorkspace", "",
                                                         Direction::Input),
                  "Input workspace that contains the spectrum on which to "
                  "perform the Pawley fit.");

  declareProperty("WorkspaceIndex", 0,
                  "Spectrum on which the fit should be performed.");

  std::vector<std::string> crystalSystems;
  crystalSystems.push_back("Cubic");
  crystalSystems.push_back("Tetragonal");
  crystalSystems.push_back("Hexagonal");
  crystalSystems.push_back("Trigonal");
  crystalSystems.push_back("Orthorhombic");
  crystalSystems.push_back("Monoclinic");
  crystalSystems.push_back("Triclinic");

  auto crystalSystemValidator =
      boost::make_shared<StringListValidator>(crystalSystems);

  declareProperty("CrystalSystem", crystalSystems.back(),
                  crystalSystemValidator,
                  "Crystal system to use for refinement.");

  declareProperty("InitialCell", "1.0 1.0 1.0 90.0 90.0 90.0",
                  "Specification of initial unit cell, given as 'a, b, c, "
                  "alpha, beta, gamma'.");

  declareProperty("MillerIndices", "[1,0,0];[1,1,0]",
                  "Semi-colon separated list of Miller indices given in the "
                  "format '[h,k,l]'.");

  declareProperty(
      new WorkspaceProperty<ITableWorkspace>("PeakTable", "", Direction::Input,
                                             PropertyMode::Optional),
      "Table with peak information. Can be used instead of "
      "supplying a list of indices for better starting parameters.");

  declareProperty("RefineZeroShift", false, "If checked, a zero-shift with the "
                                            "same unit as the spectrum is "
                                            "refined.");

  auto peakFunctionValidator = boost::make_shared<StringListValidator>(
      FunctionFactory::Instance().getFunctionNames<IPeakFunction>());

  declareProperty("PeakProfileFunction", "Gaussian", peakFunctionValidator,
                  "Profile function that is used for each peak.");

  declareProperty(new WorkspaceProperty<MatrixWorkspace>("OutputWorkspace", "",
                                                         Direction::Output),
                  "Workspace that contains measured spectrum, calculated "
                  "spectrum and difference curve.");

  declareProperty(
      new WorkspaceProperty<ITableWorkspace>("RefinedCellTable", "",
                                             Direction::Output),
      "TableWorkspace with refined lattice parameters, including errors.");

  declareProperty(
      new WorkspaceProperty<ITableWorkspace>("RefinedPeakParameterTable", "",
                                             Direction::Output),
      "TableWorkspace with refined peak parameters, including errors.");
}

void PawleyFit::exec() {
  boost::shared_ptr<PawleyFunction> pawleyFn =
      boost::dynamic_pointer_cast<PawleyFunction>(
          FunctionFactory::Instance().createFunction("PawleyFunction"));

  pawleyFn->setProfileFunction(getProperty("PeakProfileFunction"));
  pawleyFn->setCrystalSystem(getProperty("CrystalSystem"));
  pawleyFn->setUnitCell(getProperty("InitialCell"));

  MatrixWorkspace_const_sptr ws = getProperty("InputWorkspace");
  int wsIndex = getProperty("WorkspaceIndex");

  Property *peakTableProperty = getPointerToProperty("PeakTable");
  if (!peakTableProperty->isDefault()) {
    ITableWorkspace_sptr peakTable = getProperty("PeakTable");
    addHKLsToFunction(pawleyFn, peakTable);
  } else {
    std::vector<V3D> hkls = hklsFromString(getProperty("MillerIndices"));

    const MantidVec &data = ws->readY(static_cast<size_t>(wsIndex));
    pawleyFn->setPeaks(hkls, 0.005,
                       *(std::max_element(data.begin(), data.end())));
  }

  bool refineZeroShift = getProperty("RefineZeroShift");
  if (!refineZeroShift) {
    pawleyFn->fix(pawleyFn->parameterIndex("f0.ZeroShift"));
  }

  const MantidVec &xData = ws->readX(static_cast<size_t>(wsIndex));
  pawleyFn->setMatrixWorkspace(ws, static_cast<size_t>(wsIndex), xData.front(),
                               xData.back());

  Algorithm_sptr fit = createChildAlgorithm("Fit");
  fit->setProperty("Function", boost::static_pointer_cast<IFunction>(pawleyFn));
  fit->setProperty("InputWorkspace",
                   boost::const_pointer_cast<MatrixWorkspace>(ws));
  fit->setProperty("WorkspaceIndex", wsIndex);
  fit->setProperty("CreateOutput", true);
  fit->execute();

  MatrixWorkspace_sptr output = fit->getProperty("OutputWorkspace");
  setProperty("OutputWorkspace", output);
  setProperty("RefinedCellTable", getLatticeFromFunction(pawleyFn));
  setProperty("RefinedPeakParameterTable",
              getPeakParametersFromFunction(pawleyFn));
}

} // namespace CurveFitting
} // namespace Mantid
