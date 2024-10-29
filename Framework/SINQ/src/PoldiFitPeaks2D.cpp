// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#include "MantidSINQ/PoldiFitPeaks2D.h"

#include "MantidAPI/Axis.h"
#include "MantidAPI/FunctionDomain1D.h"
#include "MantidAPI/FunctionFactory.h"
#include "MantidAPI/ILatticeFunction.h"
#include "MantidAPI/IPawleyFunction.h"
#include "MantidAPI/IPeakFunction.h"
#include "MantidAPI/MultiDomainFunction.h"
#include "MantidAPI/TableRow.h"
#include "MantidAPI/WorkspaceFactory.h"
#include "MantidAPI/WorkspaceGroup.h"
#include "MantidDataObjects/TableWorkspace.h"
#include "MantidDataObjects/Workspace2D.h"
#include "MantidGeometry/Crystal/UnitCell.h"
#include "MantidKernel/ListValidator.h"

#include "MantidSINQ/PoldiUtilities/IPoldiFunction1D.h"
#include "MantidSINQ/PoldiUtilities/Poldi2DFunction.h"
#include "MantidSINQ/PoldiUtilities/PoldiDGrid.h"
#include "MantidSINQ/PoldiUtilities/PoldiDeadWireDecorator.h"
#include "MantidSINQ/PoldiUtilities/PoldiInstrumentAdapter.h"
#include "MantidSINQ/PoldiUtilities/PoldiPeakCollection.h"
#include "MantidSINQ/PoldiUtilities/PoldiSpectrumDomainFunction.h"
#include "MantidSINQ/PoldiUtilities/PoldiSpectrumLinearBackground.h"
#include "MantidSINQ/PoldiUtilities/PoldiSpectrumPawleyFunction.h"

#include "boost/make_shared.hpp"

namespace Mantid::Poldi {
// Register the algorithm into the AlgorithmFactory
DECLARE_ALGORITHM(PoldiFitPeaks2D)

using namespace API;
using namespace Kernel;
using namespace DataObjects;
using namespace Geometry;

/// Algorithm's name for identification. @see Algorithm::name
const std::string PoldiFitPeaks2D::name() const { return "PoldiFitPeaks2D"; }

/// Algorithm's version for identification. @see Algorithm::version
int PoldiFitPeaks2D::version() const { return 1; }

/// Algorithm's category for identification. @see Algorithm::category
const std::string PoldiFitPeaks2D::category() const { return "SINQ\\Poldi"; }

/// Very short algorithm summary. @see Algorith::summary
const std::string PoldiFitPeaks2D::summary() const { return "Calculates a POLDI 2D-spectrum."; }

/// Validate inputs for algorithm in case PawleyFit is used.
std::map<std::string, std::string> PoldiFitPeaks2D::validateInputs() {
  std::map<std::string, std::string> errorMap;

  bool isPawleyFit = getProperty("PawleyFit");

  if (isPawleyFit) {
    Property *refinedCellParameters = getPointerToProperty("RefinedCellParameters");
    if (refinedCellParameters->isDefault()) {
      errorMap["RefinedCellParameters"] = "Workspace name for refined cell "
                                          "parameters must be supplied for "
                                          "PawleyFit.";
    }
  }

  return errorMap;
}

/**
 * Extracts a vector of PoldiPeakCollection objects from the input
 *
 * This method examines what kind of workspace has been supplied to the
 * PoldiPeakWorkspace input property and tries to construct a vector
 * of peak collections from this. It works with either a single TableWorkspace
 * or with a WorkspaceGroups that contains several TableWorkspaces.
 *
 * If the workspace can not be interpreted properly, the method throws an
 * std::invalid_argument exception.
 *
 * @return Vector with one or more PoldiPeakCollections.
 */
std::vector<PoldiPeakCollection_sptr> PoldiFitPeaks2D::getPeakCollectionsFromInput() const {
  Workspace_sptr peakWorkspace = getProperty("PoldiPeakWorkspace");

  std::vector<PoldiPeakCollection_sptr> peakCollections;

  // If the input workspace is a TableWorkspace, insert it into the vector and
  // return it.
  TableWorkspace_sptr peakTable = std::dynamic_pointer_cast<TableWorkspace>(peakWorkspace);
  if (peakTable) {
    try {
      peakCollections.emplace_back(getPeakCollection(peakTable));
    } catch (const std::runtime_error &) {
      // do nothing
    }

    return peakCollections;
  }

  // If it's a WorkspaceGroup there are multiple peak tables, make a collection
  // for each of them.
  WorkspaceGroup_sptr peakTables = std::dynamic_pointer_cast<WorkspaceGroup>(peakWorkspace);
  if (peakTables) {
    for (size_t i = 0; i < static_cast<size_t>(peakTables->getNumberOfEntries()); ++i) {
      peakTable = std::dynamic_pointer_cast<TableWorkspace>(peakTables->getItem(i));

      if (peakTable) {
        try {
          peakCollections.emplace_back(getPeakCollection(peakTable));
        } catch (const std::runtime_error &) {
          // do nothing
        }
      }
    }

    return peakCollections;
  }

  // Otherwise throw a runtime error.
  throw std::runtime_error("Cannot proceed without peak workspace.");
}

/**
 * Tries to construct a PoldiPeakCollection from the supplied table.
 *
 * @param peakTable :: TableWorkspace with POLDI peak data.
 * @return PoldiPeakCollection with the data from the table workspace.
 */
PoldiPeakCollection_sptr PoldiFitPeaks2D::getPeakCollection(const TableWorkspace_sptr &peakTable) const {
  try {
    return std::make_shared<PoldiPeakCollection>(peakTable);
  } catch (...) {
    throw std::runtime_error("Could not initialize peak collection.");
  }
}

/// Returns an PoldiPeakCollection with normalized peaks for each input
/// collection.
std::vector<PoldiPeakCollection_sptr>
PoldiFitPeaks2D::getNormalizedPeakCollections(const std::vector<PoldiPeakCollection_sptr> &peakCollections) const {

  std::vector<PoldiPeakCollection_sptr> normalizedPeakCollections;

  for (const auto &peakCollection : peakCollections) {
    // First integrate peak collection, then normalize and append to vector
    PoldiPeakCollection_sptr integratedPeakCollection = getIntegratedPeakCollection(peakCollection);

    normalizedPeakCollections.emplace_back(getNormalizedPeakCollection(integratedPeakCollection));
  }

  return normalizedPeakCollections;
}

/**
 * Return peak collection with integrated peaks
 *
 * This method takes a PoldiPeakCollection where the intensity is represented
 * by the maximum. Then it takes the profile function stored in the peak
 * collection, which must be the name of a registered
 * IPeakFunction-implementation. The parameters height and fwhm are assigned,
 * centre is set to 0 to avoid problems with the parameter transformation for
 * the integration from -inf to inf. The profiles are integrated using
 * a PeakFunctionIntegrator to the precision of 1e-10.
 *
 * The original peak collection is not modified, a new instance is created.
 *
 * @param rawPeakCollection :: PoldiPeakCollection
 * @return PoldiPeakCollection with integrated intensities
 */
PoldiPeakCollection_sptr
PoldiFitPeaks2D::getIntegratedPeakCollection(const PoldiPeakCollection_sptr &rawPeakCollection) const {
  if (!rawPeakCollection) {
    throw std::invalid_argument("Cannot proceed with invalid PoldiPeakCollection.");
  }

  if (!isValidDeltaT(m_deltaT)) {
    throw std::invalid_argument("Cannot proceed with invalid time bin size.");
  }

  if (!m_timeTransformer) {
    throw std::invalid_argument("Cannot proceed with invalid PoldiTimeTransformer.");
  }

  if (rawPeakCollection->intensityType() == PoldiPeakCollection::Integral) {
    /* Intensities are integral already - don't need to do anything,
     * except cloning the collection, to make behavior consistent, since
     * integrating also results in a new peak collection.
     */
    return rawPeakCollection->clone();
  }

  /* If no profile function is specified, it's not possible to get integrated
   * intensities at all and we try to use the one specified by the user
   * instead.
   */
  std::string profileFunctionName = rawPeakCollection->getProfileFunctionName();

  if (!rawPeakCollection->hasProfileFunctionName()) {
    profileFunctionName = getPropertyValue("PeakProfileFunction");
  }

  std::vector<std::string> allowedProfiles = FunctionFactory::Instance().getFunctionNames<IPeakFunction>();

  if (std::find(allowedProfiles.begin(), allowedProfiles.end(), profileFunctionName) == allowedProfiles.end()) {
    throw std::runtime_error("Cannot integrate peak profiles with invalid profile function.");
  }

  PoldiPeakCollection_sptr integratedPeakCollection =
      std::make_shared<PoldiPeakCollection>(PoldiPeakCollection::Integral);
  integratedPeakCollection->setProfileFunctionName(profileFunctionName);

  // Preserve unit cell, point group
  assignCrystalData(integratedPeakCollection, rawPeakCollection);

  for (size_t i = 0; i < rawPeakCollection->peakCount(); ++i) {
    PoldiPeak_sptr peak = rawPeakCollection->peak(i);

    IPeakFunction_sptr profileFunction =
        std::dynamic_pointer_cast<IPeakFunction>(FunctionFactory::Instance().createFunction(profileFunctionName));

    profileFunction->setHeight(peak->intensity());
    profileFunction->setFwhm(peak->fwhm(PoldiPeak::AbsoluteD));

    PoldiPeak_sptr integratedPeak = peak->clone();
    integratedPeak->setIntensity(UncertainValue(profileFunction->intensity()));
    integratedPeakCollection->addPeak(integratedPeak);
  }

  return integratedPeakCollection;
}

/**
 * Normalized the intensities of the given integrated peaks
 *
 * This function normalizes the peak intensities according to the source
 * spectrum, the number of chopper slits and the number of detector elements.
 *
 * @param peakCollection :: PoldiPeakCollection with integrated intensities
 * @return PoldiPeakCollection with normalized intensities
 */
PoldiPeakCollection_sptr
PoldiFitPeaks2D::getNormalizedPeakCollection(const PoldiPeakCollection_sptr &peakCollection) const {
  if (!peakCollection) {
    throw std::invalid_argument("Cannot proceed with invalid PoldiPeakCollection.");
  }

  if (!m_timeTransformer) {
    throw std::invalid_argument("Cannot proceed without PoldiTimeTransformer.");
  }

  PoldiPeakCollection_sptr normalizedPeakCollection =
      std::make_shared<PoldiPeakCollection>(PoldiPeakCollection::Integral);
  normalizedPeakCollection->setProfileFunctionName(peakCollection->getProfileFunctionName());

  // Carry over unit cell and point group
  assignCrystalData(normalizedPeakCollection, peakCollection);

  for (size_t i = 0; i < peakCollection->peakCount(); ++i) {
    PoldiPeak_sptr peak = peakCollection->peak(i);
    double calculatedIntensity = m_timeTransformer->calculatedTotalIntensity(peak->d());

    PoldiPeak_sptr normalizedPeak = peak->clone();
    normalizedPeak->setIntensity(peak->intensity() / calculatedIntensity);
    normalizedPeakCollection->addPeak(normalizedPeak);
  }

  return normalizedPeakCollection;
}

/// Returns a vector of peak collections extracted from the function
std::vector<PoldiPeakCollection_sptr> PoldiFitPeaks2D::getCountPeakCollections(const API::IFunction_sptr &fitFunction) {
  Poldi2DFunction_sptr poldiFunction = std::dynamic_pointer_cast<Poldi2DFunction>(fitFunction);

  if (!poldiFunction) {
    throw std::runtime_error("Can only extract peaks from Poldi2DFunction.");
  }

  // Covariance matrix - needs to be assigned to the member functions for error
  // calculation
  std::shared_ptr<const Kernel::DblMatrix> covarianceMatrix = poldiFunction->getCovarianceMatrix();

  std::vector<PoldiPeakCollection_sptr> countPeakCollections;

  size_t offset = 0;
  for (size_t i = 0; i < poldiFunction->nFunctions(); ++i) {
    IFunction_sptr localFunction = poldiFunction->getFunction(i);
    size_t nLocalParams = localFunction->nParams();

    // Assign local covariance matrix.
    std::shared_ptr<Kernel::DblMatrix> localCov = getLocalCovarianceMatrix(covarianceMatrix, offset, nLocalParams);
    localFunction->setCovarianceMatrix(localCov);

    try {
      PoldiPeakCollection_sptr normalizedPeaks = getPeakCollectionFromFunction(localFunction);

      countPeakCollections.emplace_back(getCountPeakCollection(normalizedPeaks));
    } catch (const std::invalid_argument &) {
      // not a Poldi2DFunction - skip (the background functions)
    }

    offset += nLocalParams;
  }

  return countPeakCollections;
}

/**
 * Converts normalized peak intensities to count based integral intensities
 *
 * This operation is the opposite of getNormalizedPeakCollection and is used
 * to convert the intensities back to integral intensities.
 *
 * @param peakCollection :: PoldiPeakCollection with normalized intensities
 * @return PoldiPeakCollection with integral intensities
 */
PoldiPeakCollection_sptr PoldiFitPeaks2D::getCountPeakCollection(const PoldiPeakCollection_sptr &peakCollection) const {
  if (!peakCollection) {
    throw std::invalid_argument("Cannot proceed with invalid PoldiPeakCollection.");
  }

  if (!m_timeTransformer) {
    throw std::invalid_argument("Cannot proceed without PoldiTimeTransformer.");
  }

  PoldiPeakCollection_sptr countPeakCollection = std::make_shared<PoldiPeakCollection>(PoldiPeakCollection::Integral);
  countPeakCollection->setProfileFunctionName(peakCollection->getProfileFunctionName());

  // Get crystal data into new peak collection
  assignCrystalData(countPeakCollection, peakCollection);

  for (size_t i = 0; i < peakCollection->peakCount(); ++i) {
    PoldiPeak_sptr peak = peakCollection->peak(i);
    double calculatedIntensity = m_timeTransformer->calculatedTotalIntensity(peak->d());

    PoldiPeak_sptr countPeak = peak->clone();
    countPeak->setIntensity(peak->intensity() * calculatedIntensity);

    countPeakCollection->addPeak(countPeak);
  }

  return countPeakCollection;
}

/// Creates a PoldiPeak from the given profile function/hkl pair.
PoldiPeak_sptr PoldiFitPeaks2D::getPeakFromPeakFunction(const IPeakFunction_sptr &profileFunction, const V3D &hkl) {

  // Use EstimatePeakErrors to calculate errors of FWHM and so on
  auto errorAlg = createChildAlgorithm("EstimatePeakErrors");
  errorAlg->setProperty("Function", std::dynamic_pointer_cast<IFunction>(profileFunction));
  errorAlg->setPropertyValue("OutputWorkspace", "Errors");
  errorAlg->execute();

  double centre = profileFunction->centre();
  double fwhmValue = profileFunction->fwhm();

  ITableWorkspace_sptr errorTable = errorAlg->getProperty("OutputWorkspace");
  double centreError = errorTable->cell<double>(0, 2);
  double fwhmError = errorTable->cell<double>(2, 2);

  UncertainValue d(centre, centreError);
  UncertainValue fwhm(fwhmValue, fwhmError);

  UncertainValue intensity;

  bool useIntegratedIntensities = getProperty("OutputIntegratedIntensities");
  if (useIntegratedIntensities) {
    double integratedIntensity = profileFunction->intensity();
    double integratedIntensityError = errorTable->cell<double>(3, 2);
    intensity = UncertainValue(integratedIntensity, integratedIntensityError);
  } else {
    double height = profileFunction->height();
    double heightError = errorTable->cell<double>(1, 2);
    intensity = UncertainValue(height, heightError);
  }

  // Create peak with extracted parameters and supplied hkl
  PoldiPeak_sptr peak = PoldiPeak::create(MillerIndices(hkl), d, intensity, UncertainValue(1.0));
  peak->setFwhm(fwhm, PoldiPeak::FwhmRelation::AbsoluteD);

  return peak;
}

/**
 * Constructs a proper function from a peak collection
 *
 * This method constructs a Poldi2DFunction depending on whether or not a
 * Pawley fit is performed each peak contained in the peak collection.
 *
 * @param peakCollection :: PoldiPeakCollection containing peaks with integral
 *                          intensities
 * @return Poldi2DFunction with one PoldiSpectrumDomainFunction per peak
 */
Poldi2DFunction_sptr PoldiFitPeaks2D::getFunctionFromPeakCollection(const PoldiPeakCollection_sptr &peakCollection) {
  std::string profileFunctionName = getProperty("PeakProfileFunction");

  bool pawleyFit = getProperty("PawleyFit");
  if (pawleyFit) {
    return getFunctionPawley(profileFunctionName, peakCollection);
  }

  // Only use ties for independent peaks.
  Poldi2DFunction_sptr poldi2DFunction = getFunctionIndividualPeaks(profileFunctionName, peakCollection);

  std::string ties = getUserSpecifiedTies(poldi2DFunction);
  if (!ties.empty()) {
    poldi2DFunction->addTies(ties);
  }

  // Only use bounds for independent peaks.
  std::string bounds = getUserSpecifiedBounds(poldi2DFunction);
  if (!bounds.empty()) {
    poldi2DFunction->addConstraints(bounds);
  }

  return poldi2DFunction;
}

/**
 * Returns a Poldi2DFunction that encapsulates individual peaks
 *
 * This function takes all peaks from the supplied peak collection and
 * generates an IPeakFunction of the type given in the name parameter, wraps
 * them in a Poldi2DFunction and returns it.
 *
 * @param profileFunctionName :: Profile function name.
 * @param peakCollection :: Peak collection with peaks to be used in the fit.
 * @return :: A Poldi2DFunction with peak profile functions.
 */
Poldi2DFunction_sptr PoldiFitPeaks2D::getFunctionIndividualPeaks(const std::string &profileFunctionName,
                                                                 const PoldiPeakCollection_sptr &peakCollection) const {
  auto mdFunction = std::make_shared<Poldi2DFunction>();

  for (size_t i = 0; i < peakCollection->peakCount(); ++i) {
    PoldiPeak_sptr peak = peakCollection->peak(i);

    std::shared_ptr<PoldiSpectrumDomainFunction> peakFunction = std::dynamic_pointer_cast<PoldiSpectrumDomainFunction>(
        FunctionFactory::Instance().createFunction("PoldiSpectrumDomainFunction"));

    if (!peakFunction) {
      throw std::invalid_argument("Cannot process null pointer poldi function.");
    }

    peakFunction->setDecoratedFunction(profileFunctionName);

    IPeakFunction_sptr wrappedProfile = std::dynamic_pointer_cast<IPeakFunction>(peakFunction->getProfileFunction());

    if (wrappedProfile) {
      wrappedProfile->setCentre(peak->d());
      wrappedProfile->setFwhm(peak->fwhm(PoldiPeak::AbsoluteD));
      wrappedProfile->setIntensity(peak->intensity());
    }

    mdFunction->addFunction(peakFunction);
  }

  return mdFunction;
}

/**
 * Returns a Poldi2DFunction that encapsulates a PawleyFunction
 *
 * This function creates a PawleyFunction using the supplied profile function
 * name and the crystal system as well as initial cell from the input
 * properties of the algorithm and wraps it in a Poldi2DFunction.
 *
 * The cell is refined using LatticeFunction to get better starting values.
 *
 * Because the peak intensities are integral at this step but PawleyFunction
 * expects peak heights, a profile function is created and
 * setIntensity/height-methods are used to convert.
 *
 * @param profileFunctionName :: Profile function name for PawleyFunction.
 * @param peakCollection :: Peak collection with peaks to be used in the fit.
 * @return :: A Poldi2DFunction with a PawleyFunction.
 */
Poldi2DFunction_sptr PoldiFitPeaks2D::getFunctionPawley(const std::string &profileFunctionName,
                                                        const PoldiPeakCollection_sptr &peakCollection) {
  auto mdFunction = std::make_shared<Poldi2DFunction>();

  std::shared_ptr<PoldiSpectrumPawleyFunction> poldiPawleyFunction =
      std::dynamic_pointer_cast<PoldiSpectrumPawleyFunction>(
          FunctionFactory::Instance().createFunction("PoldiSpectrumPawleyFunction"));

  if (!poldiPawleyFunction) {
    throw std::invalid_argument("Could not create pawley function.");
  }

  poldiPawleyFunction->setDecoratedFunction("PawleyFunction");

  IPawleyFunction_sptr pawleyFunction = poldiPawleyFunction->getPawleyFunction();
  pawleyFunction->setProfileFunction(profileFunctionName);

  // Extract crystal system from peak collection
  PointGroup_sptr pointGroup = peakCollection->pointGroup();
  if (!pointGroup) {
    throw std::invalid_argument("Can not initialize pawley function properly - "
                                "peaks do not have point group.");
  }

  std::string latticeSystem = getLatticeSystemFromPointGroup(pointGroup);
  pawleyFunction->setLatticeSystem(latticeSystem);

  UnitCell cell = peakCollection->unitCell();
  // Extract unit cell from peak collection
  pawleyFunction->setUnitCell(getRefinedStartingCell(unitCellToStr(cell), latticeSystem, peakCollection));

  IPeakFunction_sptr pFun =
      std::dynamic_pointer_cast<IPeakFunction>(FunctionFactory::Instance().createFunction(profileFunctionName));

  for (size_t i = 0; i < peakCollection->peakCount(); ++i) {
    PoldiPeak_sptr peak = peakCollection->peak(i);

    pFun->setCentre(peak->d());
    pFun->setFwhm(peak->fwhm(PoldiPeak::AbsoluteD));
    pFun->setIntensity(peak->intensity());

    pawleyFunction->addPeak(peak->hkl().asV3D(), peak->fwhm(PoldiPeak::AbsoluteD), pFun->height());
  }

  pawleyFunction->fix(pawleyFunction->parameterIndex("f0.ZeroShift"));
  mdFunction->addFunction(poldiPawleyFunction);

  return mdFunction;
}

/**
 * Returns the lattice system for the specified point group
 *
 * This function simply uses Geometry::getLatticeSystemAsString().
 *
 * @param pointGroup :: The point group for which to find the crystal system
 * @return The crystal system for the point group
 */
std::string PoldiFitPeaks2D::getLatticeSystemFromPointGroup(const PointGroup_sptr &pointGroup) const {
  if (!pointGroup) {
    throw std::invalid_argument("Cannot return lattice system for null PointGroup.");
  }

  return Geometry::getLatticeSystemAsString(pointGroup->latticeSystem());
}

/**
 * Tries to refine the initial cell using the supplied peaks
 *
 * This method tries to refine the initial unit cell using the indexed peaks
 * that are supplied in the PoldiPeakCollection. If there are unindexed peaks,
 * the cell will not be refined at all, instead the unmodified initial cell
 * is returned.
 *
 * @param initialCell :: String with the initial unit cell
 * @param crystalSystem :: Crystal system name
 * @param peakCollection :: Collection of bragg peaks, must be indexed
 *
 * @return String for refined unit cell
 */
std::string PoldiFitPeaks2D::getRefinedStartingCell(const std::string &initialCell, const std::string &latticeSystem,
                                                    const PoldiPeakCollection_sptr &peakCollection) {

  Geometry::UnitCell cell = Geometry::strToUnitCell(initialCell);

  ILatticeFunction_sptr latticeFunction =
      std::dynamic_pointer_cast<ILatticeFunction>(FunctionFactory::Instance().createFunction("LatticeFunction"));

  latticeFunction->setLatticeSystem(latticeSystem);
  latticeFunction->fix(latticeFunction->parameterIndex("ZeroShift"));
  latticeFunction->setUnitCell(cell);

  // Remove errors from d-values
  PoldiPeakCollection_sptr clone = peakCollection->clone();
  for (size_t i = 0; i < clone->peakCount(); ++i) {
    PoldiPeak_sptr peak = clone->peak(i);

    // If there are unindexed peaks, don't refine, just return the initial cell
    if (peak->hkl() == MillerIndices()) {
      return initialCell;
    }

    peak->setD(UncertainValue(peak->d().value()));
  }

  TableWorkspace_sptr peakTable = clone->asTableWorkspace();

  auto fit = createChildAlgorithm("Fit");
  fit->setProperty("Function", std::static_pointer_cast<IFunction>(latticeFunction));
  fit->setProperty("InputWorkspace", peakTable);
  fit->setProperty("CostFunction", "Unweighted least squares");
  fit->execute();

  Geometry::UnitCell refinedCell = latticeFunction->getUnitCell();

  return Geometry::unitCellToStr(refinedCell);
}

/**
 * @brief Returns a string with ties that is passed to Fit
 *
 * This method uses the GlobalParameters property, which may contain a comma-
 * separated list of parameter names that should be the same for all peaks.
 *
 * Parameters that do not exist are silently ignored, but a warning is written
 * to the log so that users have a chance to find typos.
 *
 * @param poldiFn :: Function with some parameters.
 * @return :: String to pass to the Ties-property of Fit.
 */
std::string PoldiFitPeaks2D::getUserSpecifiedTies(const IFunction_sptr &poldiFn) {
  std::string tieParameterList = getProperty("GlobalParameters");

  if (!tieParameterList.empty()) {
    std::vector<std::string> tieParameters;

    boost::split(tieParameters, tieParameterList, boost::is_any_of(",;"));

    std::vector<std::string> parameters = poldiFn->getParameterNames();

    std::vector<std::string> tieComponents;
    for (auto &tieParameter : tieParameters) {
      if (!tieParameter.empty()) {
        std::vector<std::string> matchedParameters;

        for (auto &parameter : parameters) {
          if (parameter.ends_with(tieParameter)) {
            matchedParameters.emplace_back(parameter);
          }
        }

        if (matchedParameters.size() == 0) {
          g_log.warning("Function does not have a parameter called '" + tieParameter + "', ignoring.");
        } else if (matchedParameters.size() == 1) {
          g_log.warning("There is only one peak, no ties necessary.");
        } else {
          std::string reference = matchedParameters.front();
          for (auto par = matchedParameters.begin() + 1; par != matchedParameters.end(); ++par) {
            tieComponents.emplace_back(*par + "=" + reference);
          }
        }
      }
    }

    if (!tieComponents.empty()) {
      return boost::algorithm::join(tieComponents, ",");
    }
  }
  return "";
}

/**
 * @brief Returns a string with bounded parameters that are passed to Fit
 *
 * This method uses the BoundedParameters property, which may contain a comma-
 * separated list of parameter names that will be bounded for all peaks.
 *
 * Parameters that do not exist are silently ignored, but a warning is written
 * to the log so that users have a chance to find typos.
 *
 * @param poldiFn :: Function with some parameters.
 * @return :: String to pass to the Constraint-property of Fit.
 */
std::string PoldiFitPeaks2D::getUserSpecifiedBounds(const IFunction_sptr &poldiFn) {
  std::string boundedParametersList = getProperty("BoundedParameters");

  if (!boundedParametersList.empty()) {
    std::vector<std::string> boundedParameters;

    boost::split(boundedParameters, boundedParametersList, boost::is_any_of(",;"));

    std::vector<std::string> parameters = poldiFn->getParameterNames();

    std::vector<std::string> boundedComponents;
    for (auto &boundedParameter : boundedParameters) {
      if (!boundedParameter.empty()) {
        std::vector<std::string> matchedParameters;

        for (auto &parameter : parameters) {
          if (parameter.ends_with(boundedParameter)) {
            matchedParameters.emplace_back(parameter);
          }
        }

        if (matchedParameters.size() == 0) {
          g_log.warning("Function does not have a parameter called '" + boundedParameter + "', ignoring.");
        } else {
          for (auto par = matchedParameters.begin(); par != matchedParameters.end(); ++par) {
            boundedComponents.emplace_back("1.0<=" + *par + "<=20.0");
          }
        }
      }
    }

    if (!boundedComponents.empty()) {
      return boost::algorithm::join(boundedComponents, ",");
    }
  }
  return "";
}

/**
 * Construct a PoldiPeakCollection from a Poldi2DFunction
 *
 * This method performs the opposite operation of
 *getFunctionFromPeakCollection.
 * It takes a function, checks if it's of the proper type and turns the
 * information into a PoldiPeakCollection.
 *
 * @param  Poldi2DFunction with one PoldiSpectrumDomainFunction per peak
 * @return PoldiPeakCollection containing peaks with normalized intensities
 */
PoldiPeakCollection_sptr PoldiFitPeaks2D::getPeakCollectionFromFunction(const IFunction_sptr &fitFunction) {
  Poldi2DFunction_sptr poldi2DFunction = std::dynamic_pointer_cast<Poldi2DFunction>(fitFunction);

  if (!poldi2DFunction) {
    throw std::invalid_argument("Cannot process function that is not a Poldi2DFunction.");
  }

  PoldiPeakCollection_sptr normalizedPeaks = std::make_shared<PoldiPeakCollection>(PoldiPeakCollection::Integral);

  std::shared_ptr<const Kernel::DblMatrix> covarianceMatrix = poldi2DFunction->getCovarianceMatrix();

  size_t offset = 0;

  for (size_t i = 0; i < poldi2DFunction->nFunctions(); ++i) {
    std::shared_ptr<PoldiSpectrumPawleyFunction> poldiPawleyFunction =
        std::dynamic_pointer_cast<PoldiSpectrumPawleyFunction>(poldi2DFunction->getFunction(i));

    // If it's a Pawley function, there are several peaks in one function.
    if (poldiPawleyFunction) {
      IPawleyFunction_sptr pawleyFunction = poldiPawleyFunction->getPawleyFunction();

      if (pawleyFunction) {
        CompositeFunction_sptr decoratedFunction =
            std::dynamic_pointer_cast<CompositeFunction>(pawleyFunction->getDecoratedFunction());

        offset = decoratedFunction->getFunction(0)->nParams();

        for (size_t j = 0; j < pawleyFunction->getPeakCount(); ++j) {
          IPeakFunction_sptr profileFunction = pawleyFunction->getPeakFunction(j);

          size_t nLocalParams = profileFunction->nParams();
          std::shared_ptr<Kernel::DblMatrix> localCov =
              getLocalCovarianceMatrix(covarianceMatrix, offset, nLocalParams);
          profileFunction->setCovarianceMatrix(localCov);

          // Increment offset for next function
          offset += nLocalParams;

          V3D peakHKL = pawleyFunction->getPeakHKL(j);

          PoldiPeak_sptr peak = getPeakFromPeakFunction(profileFunction, peakHKL);

          normalizedPeaks->addPeak(peak);
        }
      }
      break;
    }

    // Otherwise, it's just one peak in this function.
    std::shared_ptr<PoldiSpectrumDomainFunction> peakFunction =
        std::dynamic_pointer_cast<PoldiSpectrumDomainFunction>(poldi2DFunction->getFunction(i));

    if (peakFunction) {
      IPeakFunction_sptr profileFunction = std::dynamic_pointer_cast<IPeakFunction>(peakFunction->getProfileFunction());

      // Get local covariance matrix
      size_t nLocalParams = profileFunction->nParams();
      std::shared_ptr<Kernel::DblMatrix> localCov = getLocalCovarianceMatrix(covarianceMatrix, offset, nLocalParams);
      profileFunction->setCovarianceMatrix(localCov);

      // Increment offset for next function
      offset += nLocalParams;

      PoldiPeak_sptr peak = getPeakFromPeakFunction(profileFunction, V3D(0, 0, 0));

      normalizedPeaks->addPeak(peak);
    }
  }

  return normalizedPeaks;
}

/// Assign Miller indices from one peak collection to another.
void PoldiFitPeaks2D::assignMillerIndices(const PoldiPeakCollection_sptr &from, PoldiPeakCollection_sptr &to) const {
  if (!from || !to) {
    throw std::invalid_argument("Cannot process invalid peak collections.");
  }

  if (from->peakCount() != to->peakCount()) {
    throw std::runtime_error("Cannot assign indices if number of peaks does not match.");
  }

  for (size_t i = 0; i < from->peakCount(); ++i) {
    PoldiPeak_sptr fromPeak = from->peak(i);
    PoldiPeak_sptr toPeak = to->peak(i);

    toPeak->setHKL(fromPeak->hkl());
  }
}

/// Copy crystal data from source to target collection to preserve during
/// integration etc.
void PoldiFitPeaks2D::assignCrystalData(PoldiPeakCollection_sptr &targetCollection,
                                        const PoldiPeakCollection_sptr &sourceCollection) const {
  targetCollection->setUnitCell(sourceCollection->unitCell());
  targetCollection->setPointGroup(sourceCollection->pointGroup());
}

/**
 * Calculates the 1D diffractogram based on the supplied function
 *
 * This method takes a fit function and checks whether it implements the
 * IPoldiFunction1D interface. If that's the case, it calculates the
 * diffractogram based on the function.
 *
 * @param fitFunction :: IFunction that also implements IPoldiFunction1D.
 * @param workspace :: Workspace with POLDI raw data.
 * @return :: Q-based diffractogram.
 */
MatrixWorkspace_sptr PoldiFitPeaks2D::get1DSpectrum(const IFunction_sptr &fitFunction,
                                                    const API::MatrixWorkspace_sptr &workspace) const {

  // Check whether the function is of correct type
  std::shared_ptr<IPoldiFunction1D> poldiFunction = std::dynamic_pointer_cast<IPoldiFunction1D>(fitFunction);

  if (!poldiFunction) {
    throw std::invalid_argument("Can only process Poldi2DFunctions.");
  }

  // And that we have an instrument available
  if (!m_poldiInstrument) {
    throw std::runtime_error("No POLDI instrument available.");
  }

  PoldiAbstractDetector_sptr detector(
      new PoldiDeadWireDecorator(workspace->detectorInfo(), m_poldiInstrument->detector()));
  std::vector<int> indices = detector->availableElements();

  // Create the grid for the diffractogram and corresponding domain/values
  double lambdaMin = getProperty("LambdaMin");
  double lambdaMax = getProperty("LambdaMax");

  PoldiDGrid grid(detector, m_poldiInstrument->chopper(), m_deltaT, std::make_pair(lambdaMin, lambdaMax));

  FunctionDomain1DVector domain(grid.grid());
  FunctionValues values(domain);

  // Calculate 1D function
  poldiFunction->poldiFunction1D(indices, domain, values);

  // Create and return Q-based workspace with spectrum
  return getQSpectrum(domain, values);
}

/// Takes a d-based domain and creates a Q-based MatrixWorkspace.
MatrixWorkspace_sptr PoldiFitPeaks2D::getQSpectrum(const FunctionDomain1D &domain, const FunctionValues &values) const {
  // Put result into workspace, based on Q
  MatrixWorkspace_sptr ws1D = WorkspaceFactory::Instance().create("Workspace2D", 1, domain.size(), values.size());

  auto &xData = ws1D->mutableX(0);
  auto &yData = ws1D->mutableY(0);
  size_t offset = values.size() - 1;
  for (size_t i = 0; i < values.size(); ++i) {
    xData[offset - i] = Conversions::dToQ(domain[i]);
    yData[offset - i] = values[i];
  }

  ws1D->getAxis(0)->setUnit("MomentumTransfer");
  return ws1D;
}

/// Returns a TableWorkspace with refined cell parameters and error.
ITableWorkspace_sptr PoldiFitPeaks2D::getRefinedCellParameters(const IFunction_sptr &fitFunction) const {
  Poldi2DFunction_sptr poldi2DFunction = std::dynamic_pointer_cast<Poldi2DFunction>(fitFunction);

  if (!poldi2DFunction || poldi2DFunction->nFunctions() < 1) {
    throw std::invalid_argument("Cannot process function that is not a Poldi2DFunction.");
  }

  // Create a new table for lattice parameters
  ITableWorkspace_sptr latticeParameterTable = WorkspaceFactory::Instance().createTable();

  latticeParameterTable->addColumn("str", "Parameter");
  latticeParameterTable->addColumn("double", "Value");
  latticeParameterTable->addColumn("double", "Error");

  // The first function should be PoldiSpectrumPawleyFunction
  std::shared_ptr<PoldiSpectrumPawleyFunction> poldiPawleyFunction =
      std::dynamic_pointer_cast<PoldiSpectrumPawleyFunction>(poldi2DFunction->getFunction(0));

  if (!poldiPawleyFunction) {
    throw std::invalid_argument("First function in Poldi2DFunction is not "
                                "PoldiSpectrumPawleyFunction.");
  }

  // Get the actual PawleyFunction to extract parameters.
  IPawleyFunction_sptr pawleyFunction =
      std::dynamic_pointer_cast<IPawleyFunction>(poldiPawleyFunction->getDecoratedFunction());

  if (pawleyFunction) {
    CompositeFunction_sptr pawleyParts =
        std::dynamic_pointer_cast<CompositeFunction>(pawleyFunction->getDecoratedFunction());

    // The first function in PawleyFunction contains the parameters
    IFunction_sptr pawleyParameters = pawleyParts->getFunction(0);

    for (size_t i = 0; i < pawleyParameters->nParams(); ++i) {
      TableRow newRow = latticeParameterTable->appendRow();
      newRow << pawleyParameters->parameterName(i) << pawleyParameters->getParameter(i)
             << pawleyParameters->getError(i);
    }
  }

  return latticeParameterTable;
}

/**
 * Performs the fit and returns the fit algorithm
 *
 * In this method the actual function fit/calculation is performed
 * using the Fit algorithm. After execution the algorithm is returned for
 * further processing.
 *
 * @param peakCollection :: PoldiPeakCollection
 * @param matrixWorkspace :: MatrixWorkspace with POLDI instrument and correct
 *                           dimensions
 * @return Instance of Fit-algorithm, after execution
 */
IAlgorithm_sptr PoldiFitPeaks2D::calculateSpectrum(const std::vector<PoldiPeakCollection_sptr> &peakCollections,
                                                   const MatrixWorkspace_sptr &matrixWorkspace) {

  std::vector<PoldiPeakCollection_sptr> normalizedPeakCollections = getNormalizedPeakCollections(peakCollections);

  // Create a Poldi2DFunction that collects all sub-functions
  auto mdFunction = std::make_shared<Poldi2DFunction>();

  // Add one Poldi2DFunction for each peak collection
  for (auto &normalizedPeakCollection : normalizedPeakCollections) {
    mdFunction->addFunction(getFunctionFromPeakCollection(normalizedPeakCollection));
  }

  // And finally background terms
  addBackgroundTerms(mdFunction);

  auto fit = createChildAlgorithm("Fit", -1, -1, true);

  if (!fit) {
    throw std::runtime_error("Could not initialize 'Fit'-algorithm.");
  }

  fit->setProperty("Function", std::dynamic_pointer_cast<IFunction>(mdFunction));
  fit->setProperty("InputWorkspace", matrixWorkspace);
  fit->setProperty("CreateOutput", true);

  int maxIterations = getProperty("MaximumIterations");
  fit->setProperty("MaxIterations", maxIterations);

  fit->setProperty("Minimizer", "Levenberg-MarquardtMD");

  // Setting the level to Notice to avoid problems with Levenberg-MarquardtMD.
  int oldLogLevel = g_log.getLevel();
  g_log.setLevelForAll(Poco::Message::PRIO_NOTICE);
  fit->execute();
  g_log.setLevelForAll(oldLogLevel);

  return fit;
}

/// Returns the output workspace stored in the Fit algorithm.
MatrixWorkspace_sptr PoldiFitPeaks2D::getWorkspace(const IAlgorithm_sptr &fitAlgorithm) const {
  if (!fitAlgorithm) {
    throw std::invalid_argument("Cannot extract workspace from null-algorithm.");
  }

  MatrixWorkspace_sptr outputWorkspace = fitAlgorithm->getProperty("OutputWorkspace");
  return outputWorkspace;
}

/// Extracts the fit function from the fit algorithm
IFunction_sptr PoldiFitPeaks2D::getFunction(const IAlgorithm_sptr &fitAlgorithm) const {
  if (!fitAlgorithm) {
    throw std::invalid_argument("Cannot extract function from null-algorithm.");
  }

  IFunction_sptr fitFunction = fitAlgorithm->getProperty("Function");
  return fitFunction;
}

/**
 * Adds background functions for the background if applicable
 *
 * If specified by the user via the corresponding algorithm parameters,
 * this function adds a constant and a linear background term to the
 * supplied Poldi2DFunction.
 *
 * @param poldi2DFunction :: Poldi2DFunction to which the background is added.
 */
void PoldiFitPeaks2D::addBackgroundTerms(const Poldi2DFunction_sptr &poldi2DFunction) const {
  bool addConstantBackground = getProperty("FitConstantBackground");
  if (addConstantBackground) {
    IFunction_sptr constantBackground = FunctionFactory::Instance().createFunction("PoldiSpectrumConstantBackground");
    constantBackground->setParameter(0, getProperty("ConstantBackgroundParameter"));
    poldi2DFunction->addFunction(constantBackground);
  }

  bool addLinearBackground = getProperty("FitLinearBackground");
  if (addLinearBackground) {
    IFunction_sptr linearBackground = FunctionFactory::Instance().createFunction("PoldiSpectrumLinearBackground");
    linearBackground->setParameter(0, getProperty("LinearBackgroundParameter"));
    poldi2DFunction->addFunction(linearBackground);
  }
}

/**
 * Extracts the covariance matrix for the supplied function
 *
 * This method extracts the covariance matrix for a sub-function from
 * the global covariance matrix. If no matrix is given, a zero-matrix is
 * returned.
 *
 * @param covarianceMatrix :: Global covariance matrix.
 * @param parameterOffset :: Offset for the parameters of profileFunction.
 * @param nParams :: Number of parameters of the local function.
 * @return
 */
std::shared_ptr<DblMatrix>
PoldiFitPeaks2D::getLocalCovarianceMatrix(const std::shared_ptr<const Kernel::DblMatrix> &covarianceMatrix,
                                          size_t parameterOffset, size_t nParams) const {
  if (covarianceMatrix) {
    std::shared_ptr<Kernel::DblMatrix> localCov = std::make_shared<Kernel::DblMatrix>(nParams, nParams, false);
    for (size_t j = 0; j < nParams; ++j) {
      for (size_t k = 0; k < nParams; ++k) {
        (*localCov)[j][k] = (*covarianceMatrix)[parameterOffset + j][parameterOffset + k];
      }
    }

    return localCov;
  }

  return std::make_shared<Kernel::DblMatrix>(nParams, nParams, false);
}

void PoldiFitPeaks2D::setPoldiInstrument(const PoldiInstrumentAdapter_sptr &instrument) {
  m_poldiInstrument = instrument;
}

/**
 * Constructs a PoldiTimeTransformer from given instrument and calls
 * setTimeTransformer.
 *
 * @param poldiInstrument :: PoldiInstrumentAdapter with valid components
 */
void PoldiFitPeaks2D::setTimeTransformerFromInstrument(const PoldiInstrumentAdapter_sptr &poldiInstrument) {
  setTimeTransformer(std::make_shared<PoldiTimeTransformer>(poldiInstrument));
}

/**
 * Sets the time transformer object that is used for all calculations.
 *
 * @param poldiTimeTransformer
 */
void PoldiFitPeaks2D::setTimeTransformer(const PoldiTimeTransformer_sptr &poldiTimeTransformer) {
  m_timeTransformer = poldiTimeTransformer;
}

/**
 * Extracts time bin width from workspace parameter
 *
 * The method uses the difference between first and second x-value of the
 * first spectrum as time bin width. If the workspace does not contain proper
 * data (0 spectra or less than 2 x-values), the method throws an
 * std::invalid_argument-exception Otherwise it calls setDeltaT.
 *
 * @param matrixWorkspace :: MatrixWorkspace with at least one spectrum with
 *at
 *                           least two x-values.
 */
void PoldiFitPeaks2D::setDeltaTFromWorkspace(const MatrixWorkspace_sptr &matrixWorkspace) {
  if (matrixWorkspace->getNumberHistograms() < 1) {
    throw std::invalid_argument("MatrixWorkspace does not contain any data.");
  }

  auto &xData = matrixWorkspace->x(0);

  if (xData.size() < 2) {
    throw std::invalid_argument("Cannot process MatrixWorkspace with less than 2 x-values.");
  }

  // difference between first and second x-value is assumed to be the bin
  // width.
  setDeltaT(matrixWorkspace->x(0)[1] - matrixWorkspace->x(0)[0]);
}

/**
 * Assigns delta t, throws std::invalid_argument on invalid value (determined
 * by isValidDeltaT).
 *
 * @param newDeltaT :: Value to be used as delta t for calculations.
 */
void PoldiFitPeaks2D::setDeltaT(double newDeltaT) {
  if (!isValidDeltaT(newDeltaT)) {
    throw std::invalid_argument("Time bin size must be larger than 0.");
  }

  m_deltaT = newDeltaT;
}

/**
 * Checks whether delta t is larger than 0.
 *
 * @param deltaT :: Value to be checked for validity as a time difference.
 * @return True if delta t is larger than 0, otherwise false.
 */
bool PoldiFitPeaks2D::isValidDeltaT(double deltaT) const { return deltaT > 0.0; }

/// Initialization of algorithm properties.
void PoldiFitPeaks2D::init() {
  declareProperty(std::make_unique<WorkspaceProperty<MatrixWorkspace>>("InputWorkspace", "", Direction::Input),
                  "Measured POLDI 2D-spectrum.");
  declareProperty(std::make_unique<WorkspaceProperty<Workspace>>("PoldiPeakWorkspace", "", Direction::Input),
                  "Table workspace with peak information.");

  auto peakFunctionValidator =
      std::make_shared<StringListValidator>(FunctionFactory::Instance().getFunctionNames<IPeakFunction>());
  declareProperty("PeakProfileFunction", "Gaussian", peakFunctionValidator,
                  "Profile function to use for integrating the peak profiles "
                  "before calculating the spectrum.");

  declareProperty("GlobalParameters", "",
                  "Comma-separated list of parameter "
                  "names that are identical for all "
                  "peaks, is ignored when PawleyFit is selected.");

  declareProperty("BoundedParameters", "",
                  "Comma-separated list of parameter "
                  "names that will will be bound to a specific "
                  "range of values for all peaks, is ignored when PawleyFit is selected.");

  declareProperty("PawleyFit", false,
                  "Instead of refining individual peaks, "
                  "refine a unit cell. Peaks must be "
                  "indexed using PoldiIndexKnownCompounds.");

  declareProperty("FitConstantBackground", true, "Add a constant background term to the fit.");
  declareProperty("ConstantBackgroundParameter", 0.0, "Initial value of constant background.");

  declareProperty("FitLinearBackground", true, "Add a background term linear in 2theta to the fit.");
  declareProperty("LinearBackgroundParameter", 0.0, "Initial value of linear background.");

  declareProperty("MaximumIterations", 0,
                  "Maximum number of iterations for "
                  "the fit. Use 0 to calculate "
                  "2D-spectrum without fitting.");

  declareProperty(std::make_unique<WorkspaceProperty<MatrixWorkspace>>("OutputWorkspace", "", Direction::Output),
                  "Calculated POLDI 2D-spectrum");
  declareProperty(std::make_unique<WorkspaceProperty<MatrixWorkspace>>("Calculated1DSpectrum", "", Direction::Output),
                  "Calculated POLDI 1D-spectrum.");

  declareProperty("LambdaMin", 1.1, "Minimum wavelength for 1D spectrum calculation");
  declareProperty("LambdaMax", 5.0, "Maximum wavelength for 1D spectrum calculation");

  declareProperty(std::make_unique<WorkspaceProperty<Workspace>>("RefinedPoldiPeakWorkspace", "", Direction::Output),
                  "Table workspace with fitted peaks.");

  declareProperty("OutputIntegratedIntensities", false,
                  "If this option is checked, the peaks in the algorithm's "
                  "output will have integrated intensities instead of the "
                  "maximum.");

  declareProperty(std::make_unique<WorkspaceProperty<Workspace>>("RefinedCellParameters", "", Direction::Output,
                                                                 PropertyMode::Optional));

  declareProperty(
      std::make_unique<WorkspaceProperty<Workspace>>("RawFitParameters", "", Direction::Output, PropertyMode::Optional),
      "Table workspace that contains all raw fit parameters.");
}

/// Executes the algorithm
void PoldiFitPeaks2D::exec() {
  std::vector<PoldiPeakCollection_sptr> peakCollections = getPeakCollectionsFromInput();

  // Try to setup the 2D data and poldi instrument
  MatrixWorkspace_sptr ws = getProperty("InputWorkspace");
  setDeltaTFromWorkspace(ws);

  setPoldiInstrument(std::make_shared<PoldiInstrumentAdapter>(ws));
  setTimeTransformerFromInstrument(m_poldiInstrument);

  // If a profile function is selected, set it on the peak collections.
  Property *profileFunctionProperty = getPointerToProperty("PeakProfileFunction");
  if (!profileFunctionProperty->isDefault()) {
    for (auto &peakCollection : peakCollections) {
      peakCollection->setProfileFunctionName(profileFunctionProperty->value());
    }
  }

  // Perform 2D-fit and return Fit algorithm to extract various information
  auto fitAlgorithm = calculateSpectrum(peakCollections, ws);

  // The FitFunction is used to generate...
  IFunction_sptr fitFunction = getFunction(fitAlgorithm);

  // ...a calculated 1D-spectrum...
  MatrixWorkspace_sptr outWs1D = get1DSpectrum(fitFunction, ws);

  // ...a vector of peak collections.
  std::vector<PoldiPeakCollection_sptr> integralPeaks = getCountPeakCollections(fitFunction);

  for (size_t i = 0; i < peakCollections.size(); ++i) {
    assignMillerIndices(peakCollections[i], integralPeaks[i]);
  }

  // Get the calculated 2D workspace
  setProperty("OutputWorkspace", getWorkspace(fitAlgorithm));

  // Set the output peaks depending on whether it's one or more workspaces
  if (integralPeaks.size() == 1) {
    setProperty("RefinedPoldiPeakWorkspace", integralPeaks.front()->asTableWorkspace());
  } else {
    WorkspaceGroup_sptr peaksGroup = std::make_shared<WorkspaceGroup>();

    for (auto &integralPeak : integralPeaks) {
      peaksGroup->addWorkspace(integralPeak->asTableWorkspace());
    }

    setProperty("RefinedPoldiPeakWorkspace", peaksGroup);
  }

  // Set the 1D-spectrum output
  setProperty("Calculated1DSpectrum", outWs1D);

  // If it was a PawleyFit, also produce one or more cell parameter tables.
  bool isPawleyFit = getProperty("PawleyFit");
  if (isPawleyFit) {
    Poldi2DFunction_sptr poldi2DFunction = std::dynamic_pointer_cast<Poldi2DFunction>(fitFunction);

    if (poldi2DFunction) {
      std::vector<ITableWorkspace_sptr> cells;
      for (size_t i = 0; i < poldi2DFunction->nFunctions(); ++i) {
        try {
          ITableWorkspace_sptr cell = getRefinedCellParameters(poldi2DFunction->getFunction(i));
          cells.emplace_back(cell);
        } catch (const std::invalid_argument &) {
          // do nothing
        }
      }

      if (cells.size() == 1) {
        setProperty("RefinedCellParameters", cells.front());
      } else {
        WorkspaceGroup_sptr cellsGroup = std::make_shared<WorkspaceGroup>();

        for (auto &cell : cells) {
          cellsGroup->addWorkspace(cell);
        }

        setProperty("RefinedCellParameters", cellsGroup);
      }

    } else {
      g_log.warning() << "Warning: Cell parameter table is empty.";
    }
  }

  // Optionally output the raw fitting parameters.
  Property *rawFitParameters = getPointerToProperty("RawFitParameters");
  if (!rawFitParameters->isDefault()) {
    ITableWorkspace_sptr parameters = fitAlgorithm->getProperty("OutputParameters");
    setProperty("RawFitParameters", parameters);
  }
}

} // namespace Mantid::Poldi
