#include "MantidCurveFitting/PawleyFit.h"

#include "MantidAPI/FunctionFactory.h"
#include "MantidCurveFitting/PawleyFunction.h"
#include "MantidAPI/TableRow.h"

#include "MantidGeometry/Crystal/UnitCell.h"
#include "MantidKernel/ListValidator.h"
#include "MantidKernel/UnitFactory.h"
#include "MantidKernel/UnitConversion.h"

#include <algorithm>

namespace Mantid {
namespace CurveFitting {

using namespace API;
using namespace Kernel;
using namespace Geometry;

DECLARE_ALGORITHM(PawleyFit)

/// Default constructor
PawleyFit::PawleyFit() : Algorithm(), m_dUnit() {}

/// Returns the summary
const std::string PawleyFit::summary() const {
  return "This algorithm performs a Pawley-fit on the supplied workspace.";
}

/// Transforms the specified value from d-spacing to the supplied unit.
double PawleyFit::getTransformedCenter(double d, const Unit_sptr &unit) const {
  if (boost::dynamic_pointer_cast<Units::Empty>(unit) ||
      boost::dynamic_pointer_cast<Units::dSpacing>(unit)) {
    return d;
  }

  return UnitConversion::run(*m_dUnit, *unit, d, 0, 0, 0, DeltaEMode::Elastic,
                             0);
}

/**
 * Add HKLs from a TableWorkspace to the PawleyFunction.
 *
 * This method tries to extract reflections from the specified TableWorkspace.
 * For the extraction to work properly it needs to have columns with the
 * following labels:
 *  HKL, d, Intensity, FWHM (rel.)
 *
 * The latter three must be convertible to double, otherwise the there will be
 * no peaks in the function. The value of d is converted to the unit of the
 * workspace to obtain an absolute FWHM-value, since FWHM (rel.) is defined
 * as FWHM / center.
 *
 * The HKLs can either be a column of V3D or a string column that contains 3
 * numbers separated by space, comma, semi-colon, or [ ]
 *
 * @param pawleyFn :: PawleyFunction which the HKLs should be added to.
 * @param tableWs :: TableWorkspace that contains the reflection information.
 * @param unit :: Unit of the workspace.
 * @param startX :: Lowest allowed x-value for reflection position.
 * @param endX :: Highest allowed x-value for reflection position.
 */
void PawleyFit::addHKLsToFunction(PawleyFunction_sptr &pawleyFn,
                                  const ITableWorkspace_sptr &tableWs,
                                  const Unit_sptr &unit, double startX,
                                  double endX) const {
  if (!tableWs || !pawleyFn) {
    throw std::invalid_argument("Can only process non-null function & table.");
  }

  pawleyFn->clearPeaks();

  try {
    Column_const_sptr hklColumn = tableWs->getColumn("HKL");
    Column_const_sptr dColumn = tableWs->getColumn("d");
    Column_const_sptr intensityColumn = tableWs->getColumn("Intensity");
    Column_const_sptr fwhmColumn = tableWs->getColumn("FWHM (rel.)");

    for (size_t i = 0; i < tableWs->rowCount(); ++i) {
      try {
        V3D hkl = getHKLFromColumn(i, hklColumn);

        double d = (*dColumn)[i];
        double center = getTransformedCenter(d, unit);
        double fwhm = (*fwhmColumn)[i] * center;
        double height = (*intensityColumn)[i];

        if (center > startX && center < endX) {
          pawleyFn->addPeak(hkl, fwhm, height);
        }
      }
      catch (std::bad_alloc) {
        // do nothing.
      }
    }
  }
  catch (std::runtime_error) {
    // Column does not exist
    throw std::runtime_error("Can not process table, the following columns are "
                             "required: HKL, d, Intensity, FWHM (rel.)");
  }
}

/// Tries to extract Miller indices as V3D from column.
V3D PawleyFit::getHKLFromColumn(size_t i,
                                const Column_const_sptr &hklColumn) const {
  if (hklColumn->type() == "V3D") {
    return hklColumn->cell<V3D>(i);
  }

  return getHkl(hklColumn->cell<std::string>(i));
}

/// Try to extract a V3D from the given string with different separators.
V3D PawleyFit::getHkl(const std::string &hklString) const {
  auto delimiters = boost::is_any_of(" ,[];");

  std::string workingCopy = boost::trim_copy_if(hklString, delimiters);
  std::vector<std::string> indicesStr;
  boost::split(indicesStr, workingCopy, delimiters);

  if (indicesStr.size() != 3) {
    throw std::invalid_argument("Input string cannot be parsed as HKL.");
  }

  V3D hkl;
  hkl.setX(boost::lexical_cast<double>(indicesStr[0]));
  hkl.setY(boost::lexical_cast<double>(indicesStr[1]));
  hkl.setZ(boost::lexical_cast<double>(indicesStr[2]));

  return hkl;
}

/// Creates a table containing the cell parameters stored in the supplied
/// function.
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

/// Extracts all profile parameters from the supplied function.
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

/// Returns a composite function consisting of the Pawley function and Chebyshev
/// background if enabled in the algorithm.
IFunction_sptr
PawleyFit::getCompositeFunction(const PawleyFunction_sptr &pawleyFn) const {
  CompositeFunction_sptr composite = boost::make_shared<CompositeFunction>();
  composite->addFunction(pawleyFn);

  bool enableChebyshev = getProperty("EnableChebyshevBackground");
  if (enableChebyshev) {
    int degree = getProperty("ChebyshevBackgroundDegree");
    IFunction_sptr chebyshev =
        FunctionFactory::Instance().createFunction("Chebyshev");
    chebyshev->setAttributeValue("n", degree);

    composite->addFunction(chebyshev);
  }

  return composite;
}

/// Initialization of properties.
void PawleyFit::init() {
  declareProperty(new WorkspaceProperty<MatrixWorkspace>("InputWorkspace", "",
                                                         Direction::Input),
                  "Input workspace that contains the spectrum on which to "
                  "perform the Pawley fit.");

  declareProperty("WorkspaceIndex", 0,
                  "Spectrum on which the fit should be performed.");

  declareProperty("StartX", 0.0, "Lower border of fitted data range.");
  declareProperty("EndX", 0.0, "Upper border of fitted data range.");

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

  declareProperty(
      new WorkspaceProperty<ITableWorkspace>("PeakTable", "", Direction::Input),
      "Table with peak information. Can be used instead of "
      "supplying a list of indices for better starting parameters.");

  declareProperty("RefineZeroShift", false, "If checked, a zero-shift with the "
                                            "same unit as the spectrum is "
                                            "refined.");

  auto peakFunctionValidator = boost::make_shared<StringListValidator>(
      FunctionFactory::Instance().getFunctionNames<IPeakFunction>());

  declareProperty("PeakProfileFunction", "Gaussian", peakFunctionValidator,
                  "Profile function that is used for each peak.");

  declareProperty("EnableChebyshevBackground", false,
                  "If checked, a Chebyshev "
                  "polynomial will be added "
                  "to model the background.");

  declareProperty("ChebyshevBackgroundDegree", 0,
                  "Degree of the Chebyshev polynomial, if used as background.");

  declareProperty("CalculationOnly", false, "If enabled, no fit is performed, "
                                            "the function is only evaluated "
                                            "and output is generated.");

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

  declareProperty("ReducedChiSquare", 0.0, "Outputs the reduced chi square "
                                           "value as a measure for the quality "
                                           "of the fit.",
                  Direction::Output);

  m_dUnit = UnitFactory::Instance().create("dSpacing");
}

/// Execution of algorithm.
void PawleyFit::exec() {
  // Setup PawleyFunction with cell from input parameters
  PawleyFunction_sptr pawleyFn = boost::dynamic_pointer_cast<PawleyFunction>(
      FunctionFactory::Instance().createFunction("PawleyFunction"));
  g_log.information() << "Setting up Pawley function..." << std::endl;

  std::string profileFunction = getProperty("PeakProfileFunction");
  pawleyFn->setProfileFunction(profileFunction);
  g_log.information() << "  Selected profile function: " << profileFunction
                      << std::endl;

  std::string crystalSystem = getProperty("CrystalSystem");
  pawleyFn->setCrystalSystem(crystalSystem);
  g_log.information() << "  Selected crystal system: " << crystalSystem
                      << std::endl;

  pawleyFn->setUnitCell(getProperty("InitialCell"));
  PawleyParameterFunction_sptr pawleyParameterFunction =
      pawleyFn->getPawleyParameterFunction();
  g_log.information()
      << "  Initial unit cell: "
      << unitCellToStr(pawleyParameterFunction->getUnitCellFromParameters())
      << std::endl;

  // Get the input workspace with the data
  MatrixWorkspace_const_sptr ws = getProperty("InputWorkspace");
  int wsIndex = getProperty("WorkspaceIndex");

  // Get x-range start and end values, depending on user input
  const MantidVec &xData = ws->readX(static_cast<size_t>(wsIndex));
  double startX = xData.front();
  double endX = xData.back();

  Property *startXProperty = getPointerToProperty("StartX");
  if (!startXProperty->isDefault()) {
    double startXInput = getProperty("StartX");
    startX = std::max(startX, startXInput);
  }

  Property *endXProperty = getPointerToProperty("EndX");
  if (!endXProperty->isDefault()) {
    double endXInput = getProperty("EndX");
    endX = std::min(endX, endXInput);
  }

  g_log.information() << "  Refined range: " << startX << " - " << endX
                      << std::endl;

  // Get HKLs from TableWorkspace
  ITableWorkspace_sptr peakTable = getProperty("PeakTable");
  Axis *xAxis = ws->getAxis(0);
  Unit_sptr xUnit = xAxis->unit();
  addHKLsToFunction(pawleyFn, peakTable, xUnit, startX, endX);

  g_log.information() << "  Peaks in PawleyFunction: "
                      << pawleyFn->getPeakCount() << std::endl;

  // Determine if zero-shift should be refined
  bool refineZeroShift = getProperty("RefineZeroShift");
  if (!refineZeroShift) {
    pawleyFn->fix(pawleyFn->parameterIndex("f0.ZeroShift"));
  } else {
    g_log.information() << "  Refining ZeroShift." << std::endl;
  }

  pawleyFn->setMatrixWorkspace(ws, static_cast<size_t>(wsIndex), startX, endX);

  g_log.information() << "Setting up Fit..." << std::endl;

  // Generate Fit-algorithm with required properties.
  Algorithm_sptr fit = createChildAlgorithm("Fit", -1, -1, true);
  fit->setProperty("Function", getCompositeFunction(pawleyFn));
  fit->setProperty("InputWorkspace",
                   boost::const_pointer_cast<MatrixWorkspace>(ws));
  fit->setProperty("StartX", startX);
  fit->setProperty("EndX", endX);
  fit->setProperty("WorkspaceIndex", wsIndex);

  bool calculationOnly = getProperty("CalculationOnly");
  if (calculationOnly) {
    fit->setProperty("MaxIterations", 0);
  }

  fit->setProperty("CreateOutput", true);

  fit->execute();
  double chiSquare = fit->getProperty("OutputChi2overDoF");

  g_log.information() << "Fit finished, reduced ChiSquare = " << chiSquare
                      << std::endl;

  g_log.information() << "Generating output..." << std::endl;

  // Create output
  MatrixWorkspace_sptr output = fit->getProperty("OutputWorkspace");
  setProperty("OutputWorkspace", output);
  setProperty("RefinedCellTable", getLatticeFromFunction(pawleyFn));
  setProperty("RefinedPeakParameterTable",
              getPeakParametersFromFunction(pawleyFn));

  setProperty("ReducedChiSquare", chiSquare);
}

} // namespace CurveFitting
} // namespace Mantid
