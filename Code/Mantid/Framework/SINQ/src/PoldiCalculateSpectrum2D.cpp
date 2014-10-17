/*WIKI*
TODO: Enter a full wiki-markup description of your algorithm here. You can then use the Build/wiki_maker.py script to generate your full wiki page.
*WIKI*/

#include "MantidSINQ/PoldiCalculateSpectrum2D.h"

#include "MantidDataObjects/Workspace2D.h"
#include "MantidDataObjects/TableWorkspace.h"
#include "MantidAPI/TableRow.h"
#include "MantidAPI/FunctionFactory.h"
#include "MantidAPI/MultiDomainFunction.h"
#include "MantidSINQ/PoldiUtilities/PoldiSpectrumDomainFunction.h"

#include "MantidSINQ/PoldiUtilities/PoldiPeakCollection.h"
#include "MantidSINQ/PoldiUtilities/PoldiInstrumentAdapter.h"
#include "MantidSINQ/PoldiUtilities/PeakFunctionIntegrator.h"
#include "MantidAPI/IPeakFunction.h"

#include "MantidSINQ/PoldiUtilities/Poldi2DFunction.h"

#include "boost/make_shared.hpp"

namespace Mantid
{
namespace Poldi
{
  // Register the algorithm into the AlgorithmFactory
  DECLARE_ALGORITHM(PoldiCalculateSpectrum2D)

  using namespace API;
  using namespace Kernel;
  using namespace DataObjects;

  /** Constructor
   */
  PoldiCalculateSpectrum2D::PoldiCalculateSpectrum2D():
      Algorithm(),
      m_timeTransformer(),
      m_deltaT(0.0)
  {
  }

  /** Destructor
   */
  PoldiCalculateSpectrum2D::~PoldiCalculateSpectrum2D()
  {
  }

  /// Algorithm's name for identification. @see Algorithm::name
  const std::string PoldiCalculateSpectrum2D::name() const { return "PoldiCalculateSpectrum2D";}

  /// Algorithm's version for identification. @see Algorithm::version
  int PoldiCalculateSpectrum2D::version() const { return 1;}

  /// Algorithm's category for identification. @see Algorithm::category
  const std::string PoldiCalculateSpectrum2D::category() const { return "SINQ\\Poldi\\PoldiSet";}

  /// Very short algorithm summary. @see Algorith::summary
  const std::string PoldiCalculateSpectrum2D::summary() const
  {
      return "Calculates a POLDI 2D-spectrum.";
  }

  /// Initialization of algorithm properties.
  void PoldiCalculateSpectrum2D::init()
  {
    declareProperty(new WorkspaceProperty<MatrixWorkspace>("InputWorkspace","",Direction::Input), "Measured POLDI 2D-spectrum.");
    declareProperty(new WorkspaceProperty<TableWorkspace>("PoldiPeakWorkspace", "", Direction::Input), "Table workspace with peak information.");
    declareProperty("PeakProfileFunction", "", "Profile function to use for integrating the peak profiles before calculating the spectrum.");

    declareProperty("FitConstantBackground", true, "Add a constant background term to the fit.");
    declareProperty("ConstantBackgroundParameter", 0.0, "Initial value of constant background.");

    declareProperty("FitLinearBackground", true, "Add a background term linear in 2theta to the fit.");
    declareProperty("LinearBackgroundParameter", 0.0, "Initial value of linear background.");

    declareProperty(new WorkspaceProperty<MatrixWorkspace>("OutputWorkspace","",Direction::Output), "Calculated POLDI 2D-spectrum");
  }

  /**
   * Constructs a proper function from a peak collection
   *
   * This method constructs a Poldi2DFunction and assigns one PoldiSpectrumDomainFunction to it for
   * each peak contained in the peak collection.
   *
   * @param peakCollection :: PoldiPeakCollection containing peaks with integral intensities
   * @return Poldi2DFunction with one PoldiSpectrumDomainFunction per peak
   */
  boost::shared_ptr<Poldi2DFunction> PoldiCalculateSpectrum2D::getFunctionFromPeakCollection(const PoldiPeakCollection_sptr &peakCollection) const
  {
      boost::shared_ptr<Poldi2DFunction> mdFunction(new Poldi2DFunction);

      for(size_t i = 0; i < peakCollection->peakCount(); ++i) {
          PoldiPeak_sptr peak = peakCollection->peak(i);

          IFunction_sptr peakFunction = FunctionFactory::Instance().createFunction("PoldiSpectrumDomainFunction");
          peakFunction->setParameter("Area", peak->intensity());
          peakFunction->setParameter("Fwhm", peak->fwhm(PoldiPeak::AbsoluteD));
          peakFunction->setParameter("Centre", peak->d());

          mdFunction->addFunction(peakFunction);
      }

      return mdFunction;
  }

  /// Executes the algorithm
  void PoldiCalculateSpectrum2D::exec()
  {
      TableWorkspace_sptr peakTable = getProperty("PoldiPeakWorkspace");
      if(!peakTable) {
          throw std::runtime_error("Cannot proceed without peak workspace.");
      }

      MatrixWorkspace_sptr ws = getProperty("InputWorkspace");
      setDeltaTFromWorkspace(ws);

      setTimeTransformerFromInstrument(boost::make_shared<PoldiInstrumentAdapter>(ws));

      PoldiPeakCollection_sptr peakCollection = getPeakCollection(peakTable);

      Property *profileFunctionProperty = getPointerToProperty("PeakProfileFunction");
      if(!profileFunctionProperty->isDefault()) {
          peakCollection->setProfileFunctionName(profileFunctionProperty->value());
      }

      setProperty("OutputWorkspace", calculateSpectrum(peakCollection, ws));
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
  void PoldiCalculateSpectrum2D::addBackgroundTerms(boost::shared_ptr<Poldi2DFunction> poldi2DFunction) const
  {
      bool addConstantBackground = getProperty("FitConstantBackground");
      if(addConstantBackground) {
          IFunction_sptr constantBackground = FunctionFactory::Instance().createFunction("FlatBackground");
          constantBackground->setParameter(0, getProperty("ConstantBackgroundParameter"));
          poldi2DFunction->addFunction(constantBackground);
      }

      bool addLinearBackground = getProperty("FitLinearBackground");
      if(addLinearBackground) {
          IFunction_sptr linearBackground = FunctionFactory::Instance().createFunction("PoldiSpectrumLinearBackground");
          linearBackground->setParameter(0, getProperty("LinearBackgroundParameter"));
          poldi2DFunction->addFunction(linearBackground);
      }
  }

  /**
   * Calculates the 2D spectrum in a MatrixWorkspace
   *
   * In this method the actual function calculation is performed using Fit.
   *
   * @param peakCollection :: PoldiPeakCollection
   * @param matrixWorkspace :: MatrixWorkspace with POLDI instrument and correct dimensions
   * @return MatrixWorkspace with the calculated data
   */
  MatrixWorkspace_sptr PoldiCalculateSpectrum2D::calculateSpectrum(const PoldiPeakCollection_sptr &peakCollection, const MatrixWorkspace_sptr &matrixWorkspace)
  {
      PoldiPeakCollection_sptr integratedPeaks = getIntegratedPeakCollection(peakCollection);
      PoldiPeakCollection_sptr normalizedPeakCollection = getNormalizedPeakCollection(integratedPeaks);

      boost::shared_ptr<Poldi2DFunction> mdFunction = getFunctionFromPeakCollection(normalizedPeakCollection);

      addBackgroundTerms(mdFunction);

      IAlgorithm_sptr fit = createChildAlgorithm("Fit", -1, -1, true);

      if(!fit) {
          throw std::runtime_error("Could not initialize 'Fit'-algorithm.");
      }

      fit->setProperty("Function", boost::dynamic_pointer_cast<IFunction>(mdFunction));
      fit->setProperty("InputWorkspace", matrixWorkspace);
      fit->setProperty("CreateOutput", true);
      fit->setProperty("MaxIterations", 0);
      fit->setProperty("Minimizer", "Levenberg-MarquardtMD");

      fit->execute();

      MatrixWorkspace_sptr outputWs = fit->getProperty("OutputWorkspace");

      return outputWs;
  }

  /**
   * Constructs a PoldiTimeTransformer from given instrument and calls setTimeTransformer.
   *
   * @param poldiInstrument :: PoldiInstrumentAdapter with valid components
   */
  void PoldiCalculateSpectrum2D::setTimeTransformerFromInstrument(const PoldiInstrumentAdapter_sptr &poldiInstrument)
  {
      setTimeTransformer(boost::make_shared<PoldiTimeTransformer>(poldiInstrument));
  }

  /**
   * Sets the time transformer object that is used for all calculations.
   *
   * @param poldiTimeTransformer
   */
  void PoldiCalculateSpectrum2D::setTimeTransformer(const PoldiTimeTransformer_sptr &poldiTimeTransformer)
  {
      m_timeTransformer = poldiTimeTransformer;
  }

  /**
   * Extracts time bin width from workspace parameter
   *
   * The method uses the difference between first and second x-value of the first spectrum as
   * time bin width. If the workspace does not contain proper data (0 spectra or less than
   * 2 x-values), the method throws an std::invalid_argument-exception. Otherwise it calls setDeltaT.
   *
   * @param matrixWorkspace :: MatrixWorkspace with at least one spectrum with at least two x-values.
   */
  void PoldiCalculateSpectrum2D::setDeltaTFromWorkspace(const MatrixWorkspace_sptr &matrixWorkspace)
  {
      if(matrixWorkspace->getNumberHistograms() < 1) {
          throw std::invalid_argument("MatrixWorkspace does not contain any data.");
      }

      MantidVec xData = matrixWorkspace->readX(0);

      if(xData.size() < 2) {
          throw std::invalid_argument("Cannot process MatrixWorkspace with less than 2 x-values.");
      }

      // difference between first and second x-value is assumed to be the bin width.
      setDeltaT(matrixWorkspace->readX(0)[1] - matrixWorkspace->readX(0)[0]);
  }

  /**
   * Assigns delta t, throws std::invalid_argument on invalid value (determined by isValidDeltaT).
   *
   * @param newDeltaT :: Value to be used as delta t for calculations.
   */
  void PoldiCalculateSpectrum2D::setDeltaT(double newDeltaT)
  {
      if(!isValidDeltaT(newDeltaT)) {
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
  bool PoldiCalculateSpectrum2D::isValidDeltaT(double deltaT) const
  {
      return deltaT > 0.0;
  }

  /**
   * Tries to construct a PoldiPeakCollection from the supplied table.
   *
   * @param peakTable :: TableWorkspace with POLDI peak data.
   * @return PoldiPeakCollection with the data from the table workspace.
   */
  PoldiPeakCollection_sptr PoldiCalculateSpectrum2D::getPeakCollection(const TableWorkspace_sptr &peakTable) const
  {
      try {
          return boost::make_shared<PoldiPeakCollection>(peakTable);
      } catch(...) {
          throw std::runtime_error("Could not initialize peak collection.");
      }
  }

  /**
   * Return peak collection with integrated peaks
   *
   * This method takes a PoldiPeakCollection where the intensity is represented by the maximum. Then
   * it takes the profile function stored in the peak collection, which must be the name of a registered
   * IPeakFunction-implementation. The parameters height and fwhm are assigned, centre is set to 0 to
   * avoid problems with the parameter transformation for the integration from -inf to inf. The profiles are
   * integrated using a PeakFunctionIntegrator to the precision of 1e-10.
   *
   * The original peak collection is not modified, a new instance is created.
   *
   * @param rawPeakCollection :: PoldiPeakCollection
   * @return PoldiPeakCollection with integrated intensities
   */
  PoldiPeakCollection_sptr PoldiCalculateSpectrum2D::getIntegratedPeakCollection(const PoldiPeakCollection_sptr &rawPeakCollection) const
  {
      if(!rawPeakCollection) {
          throw std::invalid_argument("Cannot proceed with invalid PoldiPeakCollection.");
      }

      if(!isValidDeltaT(m_deltaT)) {
          throw std::invalid_argument("Cannot proceed with invalid time bin size.");
      }

      if(!m_timeTransformer) {
          throw std::invalid_argument("Cannot proceed with invalid PoldiTimeTransformer.");
      }

      if(rawPeakCollection->intensityType() == PoldiPeakCollection::Integral) {
          /* Intensities are integral already - don't need to do anything,
           * except cloning the collection, to make behavior consistent, since
           * integrating also results in a new peak collection.
           */
          return rawPeakCollection->clone();
      }

      /* If no profile function is specified, it's not possible to get integrated
       * intensities at all and we need to abort at this point.
       */
      if(!rawPeakCollection->hasProfileFunctionName()) {
          throw std::runtime_error("Cannot integrate peak profiles without profile function.");
      }

      PeakFunctionIntegrator peakIntegrator(1e-10);

      PoldiPeakCollection_sptr integratedPeakCollection = boost::make_shared<PoldiPeakCollection>(PoldiPeakCollection::Integral);
      integratedPeakCollection->setProfileFunctionName(rawPeakCollection->getProfileFunctionName());

      for(size_t i = 0; i < rawPeakCollection->peakCount(); ++i) {
          PoldiPeak_sptr peak = rawPeakCollection->peak(i);

          /* The integration is performed in time dimension,
           * so the fwhm needs to be transformed.
           */
          double fwhmTime = m_timeTransformer->dToTOF(peak->fwhm(PoldiPeak::AbsoluteD));

          IPeakFunction_sptr profileFunction = boost::dynamic_pointer_cast<IPeakFunction>(FunctionFactory::Instance().createFunction(rawPeakCollection->getProfileFunctionName()));
          profileFunction->setHeight(peak->intensity());
          profileFunction->setFwhm(fwhmTime);

          /* Because the integration is running from -inf to inf, it is necessary
           * to set the centre to 0. Otherwise the transformation performed by
           * the integration routine will create problems.
           */
          profileFunction->setCentre(0.0);

          IntegrationResult integration = peakIntegrator.integrateInfinity(profileFunction);

          if(!integration.success) {
              throw std::runtime_error("Problem during peak integration. Aborting.");
          }

          PoldiPeak_sptr integratedPeak = peak->clone();
          /* Integration is performed in the time domain and later everything is normalized
           * by deltaT. In the original code this is done at this point, so this behavior is kept
           * for now.
           */
          integratedPeak->setIntensity(UncertainValue(integration.result / m_deltaT));
          integratedPeakCollection->addPeak(integratedPeak);
      }

      return integratedPeakCollection;
  }

  /**
   * Normalized the intensities of the given integrated peaks
   *
   * This function normalizes the peak intensities according to the source spectrum, the number of
   * chopper slits and the number of detector elements.
   *
   * @param peakCollection :: PoldiPeakCollection with integrated intensities
   * @return PoldiPeakCollection with normalized intensities
   */
  PoldiPeakCollection_sptr PoldiCalculateSpectrum2D::getNormalizedPeakCollection(const PoldiPeakCollection_sptr &peakCollection) const
  {
      if(!peakCollection) {
          throw std::invalid_argument("Cannot proceed with invalid PoldiPeakCollection.");
      }

      if(!m_timeTransformer) {
          throw std::invalid_argument("Cannot proceed without PoldiTimeTransformer.");
      }

      PoldiPeakCollection_sptr normalizedPeakCollection = boost::make_shared<PoldiPeakCollection>(PoldiPeakCollection::Integral);
      normalizedPeakCollection->setProfileFunctionName(peakCollection->getProfileFunctionName());

      for(size_t i = 0; i < peakCollection->peakCount(); ++i) {
          PoldiPeak_sptr peak = peakCollection->peak(i);
          double calculatedIntensity = m_timeTransformer->calculatedTotalIntensity(peak->d());

          PoldiPeak_sptr normalizedPeak = peak->clone();
          normalizedPeak->setIntensity(peak->intensity() / calculatedIntensity);

          normalizedPeakCollection->addPeak(normalizedPeak);
      }

      return normalizedPeakCollection;
  }


} // namespace Poldi
} // namespace Mantid
