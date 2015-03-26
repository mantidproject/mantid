#include "MantidAlgorithms/PolarizationCorrection.h"
#include "MantidAPI/WorkspaceGroup.h"
#include "MantidAPI/WorkspaceFactory.h"
#include "MantidKernel/Unit.h"
#include "MantidKernel/ArrayProperty.h"
#include "MantidGeometry/Instrument.h"
#include "MantidKernel/ListValidator.h"
#include "MantidDataObjects/WorkspaceSingleValue.h"
#include <algorithm>
#include <boost/shared_ptr.hpp>

using namespace Mantid::API;
using namespace Mantid::Kernel;
using namespace Mantid::Geometry;

namespace {

const std::string pNRLabel() { return "PNR"; }

const std::string pALabel() { return "PA"; }

const std::string crhoLabel() { return "CRho"; }

const std::string cppLabel() { return "CPp"; }

const std::string cAlphaLabel() { return "CAlpha"; }

const std::string cApLabel() { return "CAp"; }

std::vector<std::string> modes() {
  std::vector<std::string> modes;
  modes.push_back(pALabel());
  modes.push_back(pNRLabel());
  return modes;
}

Instrument_const_sptr fetchInstrument(WorkspaceGroup const *const groupWS) {
  if (groupWS->size() == 0) {
    throw std::invalid_argument("Input group workspace has no children.");
  }
  Workspace_sptr firstWS = groupWS->getItem(0);
  MatrixWorkspace_sptr matrixWS =
      boost::dynamic_pointer_cast<MatrixWorkspace>(firstWS);
  return matrixWS->getInstrument();
}

void validateInputWorkspace(WorkspaceGroup_sptr &ws) {

  for (size_t i = 0; i < ws->size(); ++i) {
    MatrixWorkspace_sptr lastWS;

    Workspace_sptr item = ws->getItem(i);

    if (MatrixWorkspace_sptr ws2d =
            boost::dynamic_pointer_cast<MatrixWorkspace>(item)) {

      // X-units check
      auto wsUnit = ws2d->getAxis(0)->unit();
      auto expectedUnit = Units::Wavelength();
      if (wsUnit->unitID() != expectedUnit.unitID()) {
        throw std::invalid_argument(
            "Input workspaces must have units of Wavelength");
      }

      // More detailed checks based on shape.
      if (lastWS) {
        if (lastWS->getNumberHistograms() != ws2d->getNumberHistograms()) {
          throw std::invalid_argument("Not all workspaces in the "
                                      "InputWorkspace WorkspaceGroup have the "
                                      "same number of spectrum");
        }
        if (lastWS->blocksize() != ws2d->blocksize()) {
          throw std::invalid_argument("Number of bins do not match between all "
                                      "workspaces in the InputWorkspace "
                                      "WorkspaceGroup");
        }

        auto currentX = ws2d->readX(0);
        auto lastX = lastWS->readX(0);
        if (currentX != lastX) {
          throw std::invalid_argument("X-arrays do not match between all "
                                      "workspaces in the InputWorkspace "
                                      "WorkspaceGroup.");
        }
      }

      lastWS = ws2d; // Cache the last workspace so we can use it for comparison
                     // purposes.

    } else {
      std::stringstream messageBuffer;
      messageBuffer << "Item with index: " << i
                    << "in the InputWorkspace is not a MatrixWorkspace";
      throw std::invalid_argument(messageBuffer.str());
    }
  }
}

typedef std::vector<double> VecDouble;
}

namespace Mantid {
namespace Algorithms {

// Register the algorithm into the AlgorithmFactory
DECLARE_ALGORITHM(PolarizationCorrection)

//----------------------------------------------------------------------------------------------
/** Constructor
 */
PolarizationCorrection::PolarizationCorrection() {}

//----------------------------------------------------------------------------------------------
/** Destructor
 */
PolarizationCorrection::~PolarizationCorrection() {}

//----------------------------------------------------------------------------------------------
/// Algorithm's name for identification. @see Algorithm::name
const std::string PolarizationCorrection::name() const {
  return "PolarizationCorrection";
}

/// Algorithm's version for identification. @see Algorithm::version
int PolarizationCorrection::version() const { return 1; }

/// Algorithm's category for identification. @see Algorithm::category
const std::string PolarizationCorrection::category() const {
  return "Reflectometry";
}

bool PolarizationCorrection::isPropertyDefault(
    const std::string &propertyName) const {
  Property *prop = this->getProperty(propertyName);
  return prop->isDefault();
}

/**
 * @return Return the algorithm summary.
 */
const std::string PolarizationCorrection::summary() const {
  return "Makes corrections for polarization efficiencies of the polarizer and "
         "analyzer in a reflectometry neutron spectrometer.";
}

/**
 * Add a constant value to a workspace
 * @param lhsWS WS to add to
 * @param rhs Value to add
 * @return Summed workspace
 */
MatrixWorkspace_sptr PolarizationCorrection::add(MatrixWorkspace_sptr &lhsWS,
                                                 const double &rhs) {
  auto plus = this->createChildAlgorithm("Plus");
  auto rhsWS = boost::make_shared<DataObjects::WorkspaceSingleValue>(rhs);
  plus->initialize();
  plus->setProperty("LHSWorkspace", lhsWS);
  plus->setProperty("RHSWorkspace", rhsWS);
  plus->execute();
  MatrixWorkspace_sptr outWS = plus->getProperty("OutputWorkspace");
  return outWS;
}

//----------------------------------------------------------------------------------------------
/** Initialize the algorithm's properties.
 */
void PolarizationCorrection::init() {
  declareProperty(new WorkspaceProperty<Mantid::API::WorkspaceGroup>(
                      "InputWorkspace", "", Direction::Input),
                  "An input workspace to process.");

  auto propOptions = modes();
  declareProperty("PolarizationAnalysis", "PA",
                  boost::make_shared<StringListValidator>(propOptions),
                  "What Polarization mode will be used?\n"
                  "PNR: Polarized Neutron Reflectivity mode\n"
                  "PA: Full Polarization Analysis PNR-PA");

  declareProperty(new ArrayProperty<double>(cppLabel(), Direction::Input),
                  "Effective polarizing power of the polarizing system. "
                  "Expressed as a ratio 0 < Pp < 1");

  declareProperty(new ArrayProperty<double>(cApLabel(), Direction::Input),
                  "Effective polarizing power of the analyzing system. "
                  "Expressed as a ratio 0 < Ap < 1");

  declareProperty(new ArrayProperty<double>(crhoLabel(), Direction::Input),
                  "Ratio of efficiencies of polarizer spin-down to polarizer "
                  "spin-up. This is characteristic of the polarizer flipper. "
                  "Values are constants for each term in a polynomial "
                  "expression.");

  declareProperty(new ArrayProperty<double>(cAlphaLabel(), Direction::Input),
                  "Ratio of efficiencies of analyzer spin-down to analyzer "
                  "spin-up. This is characteristic of the analyzer flipper. "
                  "Values are factors for each term in a polynomial "
                  "expression.");

  declareProperty(new WorkspaceProperty<Mantid::API::WorkspaceGroup>(
                      "OutputWorkspace", "", Direction::Output),
                  "An output workspace.");
}

MatrixWorkspace_sptr PolarizationCorrection::execPolynomialCorrection(
    MatrixWorkspace_sptr &input, const VecDouble &coefficients) {
  auto polyCorr = this->createChildAlgorithm("PolynomialCorrection");
  polyCorr->initialize();
  polyCorr->setProperty("InputWorkspace", input);
  polyCorr->setProperty("Coefficients", coefficients);
  polyCorr->execute();
  MatrixWorkspace_sptr corrected = polyCorr->getProperty("OutputWorkspace");
  return corrected;
}

MatrixWorkspace_sptr
PolarizationCorrection::copyShapeAndFill(MatrixWorkspace_sptr &base,
                                         const double &value) {
  MatrixWorkspace_sptr wsTemplate = WorkspaceFactory::Instance().create(base);
  // Copy the x-array across to the new workspace.
  for (size_t i = 0; i < wsTemplate->getNumberHistograms(); ++i) {
    wsTemplate->setX(i, base->readX(i));
  }
  auto filled = this->add(wsTemplate, value);
  return filled;
}

WorkspaceGroup_sptr PolarizationCorrection::execPA(WorkspaceGroup_sptr inWS) {

  if (isPropertyDefault(cAlphaLabel())) {
    throw std::invalid_argument("Must provide as input for PA: " +
                                cAlphaLabel());
  }
  if (isPropertyDefault(cApLabel())) {
    throw std::invalid_argument("Must provide as input for PA: " + cApLabel());
  }

  size_t itemIndex = 0;
  MatrixWorkspace_sptr Ipp =
      boost::dynamic_pointer_cast<MatrixWorkspace>(inWS->getItem(itemIndex++));
  MatrixWorkspace_sptr Iaa =
      boost::dynamic_pointer_cast<MatrixWorkspace>(inWS->getItem(itemIndex++));
  MatrixWorkspace_sptr Ipa =
      boost::dynamic_pointer_cast<MatrixWorkspace>(inWS->getItem(itemIndex++));
  MatrixWorkspace_sptr Iap =
      boost::dynamic_pointer_cast<MatrixWorkspace>(inWS->getItem(itemIndex++));

  MatrixWorkspace_sptr ones = copyShapeAndFill(Iaa, 1.0);
  // The ones workspace is now identical to the input workspaces in x, but has 1
  // as y values. It can therefore be used to build real polynomial functions.

  const VecDouble c_rho = getProperty(crhoLabel());
  const VecDouble c_alpha = getProperty(cAlphaLabel());
  const VecDouble c_pp = getProperty(cppLabel());
  const VecDouble c_ap = getProperty(cApLabel());

  const auto rho = this->execPolynomialCorrection(
      ones, c_rho); // Execute polynomial expression
  const auto pp = this->execPolynomialCorrection(
      ones, c_pp); // Execute polynomial expression
  const auto alpha = this->execPolynomialCorrection(
      ones, c_alpha); // Execute polynomial expression
  const auto ap = this->execPolynomialCorrection(
      ones, c_ap); // Execute polynomial expression

  const auto A0 =
      (Iaa + ap * Ipa * rho + ap * Iap * alpha + Ipp * ap * alpha * rho) * pp;
  const auto A1 = pp * Iaa;
  const auto A2 = pp * Iap;
  const auto A3 = ap * Iaa;
  const auto A4 = ap * Ipa;
  const auto apAlpha = ap * alpha;
  const auto A5 = apAlpha * Ipp;
  const auto A6 = apAlpha * Iap;
  const auto ppRho = pp * rho;
  const auto A7 = ppRho * Ipp;
  const auto A8 = ppRho * Ipa;

  const auto D = pp * ap * (rho + alpha + 1.0 + rho * alpha);
  const auto IppPlusIaaMinusIpaMinusIap = Ipp + Iaa - Ipa - Iap;
  const auto IpaPlusIapMinusIppMinusIaa = Ipa + Iap - Ipp - Iaa;
  const auto AOperations = A0 - A1 + A2 - A3 + A4 + A5 - A6 + A7 - A8;
  const auto negateAOperations = A0 + A1 - A2 - A3 + A4 + A5 - A6 - A7 + A8;

  const auto nIpp = (AOperations + IppPlusIaaMinusIpaMinusIap) / D;
  const auto nIaa = (negateAOperations + IppPlusIaaMinusIpaMinusIap) / D;
  const auto nIpa = (AOperations + IpaPlusIapMinusIppMinusIaa) / D;
  const auto nIap = (negateAOperations + IpaPlusIapMinusIppMinusIaa) / D;

  // Preserve the history of the inside workspaces
  nIpp->history().addHistory(Ipp->getHistory());
  nIaa->history().addHistory(Iaa->getHistory());
  nIpa->history().addHistory(Ipa->getHistory());
  nIap->history().addHistory(Iap->getHistory());

  WorkspaceGroup_sptr dataOut = boost::make_shared<WorkspaceGroup>();
  dataOut->addWorkspace(nIpp);
  dataOut->addWorkspace(nIaa);
  dataOut->addWorkspace(nIpa);
  dataOut->addWorkspace(nIap);

  return dataOut;
}

WorkspaceGroup_sptr PolarizationCorrection::execPNR(WorkspaceGroup_sptr inWS) {
  size_t itemIndex = 0;
  MatrixWorkspace_sptr Ip =
      boost::dynamic_pointer_cast<MatrixWorkspace>(inWS->getItem(itemIndex++));
  MatrixWorkspace_sptr Ia =
      boost::dynamic_pointer_cast<MatrixWorkspace>(inWS->getItem(itemIndex++));

  MatrixWorkspace_sptr ones = copyShapeAndFill(Ip, 1.0);

  const VecDouble c_rho = getProperty(crhoLabel());
  const VecDouble c_pp = getProperty(cppLabel());

  const auto rho = this->execPolynomialCorrection(
      ones, c_rho); // Execute polynomial expression
  const auto pp = this->execPolynomialCorrection(
      ones, c_pp); // Execute polynomial expression

  const auto D = pp * (rho + 1);

  const auto nIp = (Ip * (rho * pp + 1.0) + Ia * (pp - 1.0)) / D;
  const auto nIa = (Ip * (rho * pp - 1.0) + Ia * (pp + 1.0)) / D;

  // Preserve the history of the inside workspaces
  nIp->history().addHistory(Ip->getHistory());
  nIa->history().addHistory(Ia->getHistory());

  WorkspaceGroup_sptr dataOut = boost::make_shared<WorkspaceGroup>();
  dataOut->addWorkspace(nIp);
  dataOut->addWorkspace(nIa);

  return dataOut;
}

//----------------------------------------------------------------------------------------------
/** Execute the algorithm.
 */
void PolarizationCorrection::exec() {
  WorkspaceGroup_sptr inWS = getProperty("InputWorkspace");
  const std::string analysisMode = getProperty("PolarizationAnalysis");
  const size_t nWorkspaces = inWS->size();

  validateInputWorkspace(inWS);

  Instrument_const_sptr instrument = fetchInstrument(inWS.get());

  // Check if we need to fetch polarization parameters from the instrument's
  // parameters
  std::map<std::string, std::string> loadableProperties;
  loadableProperties[crhoLabel()] = "crho";
  loadableProperties[cppLabel()] = "cPp";

  // In PA mode, we also require cap and calpha
  if (analysisMode == pALabel()) {
    loadableProperties[cApLabel()] = "cAp";
    loadableProperties[cAlphaLabel()] = "calpha";
  }

  for (auto propName = loadableProperties.begin();
       propName != loadableProperties.end(); ++propName) {
    Property *prop = getProperty(propName->first);

    if (!prop)
      continue;

    if (prop->isDefault()) {
      auto vals = instrument->getStringParameter(propName->second);
      if (vals.empty())
        throw std::runtime_error(
            "Cannot find value for " + propName->first +
            " in parameter file. Please specify this property manually.");
      prop->setValue(vals[0]);
    }
  }

  WorkspaceGroup_sptr outWS;
  if (analysisMode == pALabel()) {
    if (nWorkspaces != 4) {
      throw std::invalid_argument(
          "For PA analysis, input group must have 4 periods.");
    }
    g_log.notice("PA polarization correction");
    outWS = execPA(inWS);
  } else if (analysisMode == pNRLabel()) {
    if (nWorkspaces != 2) {
      throw std::invalid_argument(
          "For PNR analysis, input group must have 2 periods.");
    }
    outWS = execPNR(inWS);
    g_log.notice("PNR polarization correction");
  }
  this->setProperty("OutputWorkspace", outWS);
}

} // namespace Algorithms
} // namespace Mantid
