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

namespace Mantid
{
namespace Poldi
{
  // Register the algorithm into the AlgorithmFactory
  DECLARE_ALGORITHM(PoldiCalculateSpectrum2D)

  using namespace API;
  using namespace Kernel;
  using namespace DataObjects;


  //----------------------------------------------------------------------------------------------
  /** Constructor
   */
  PoldiCalculateSpectrum2D::PoldiCalculateSpectrum2D():
      Algorithm(),
      m_timeTransformer(),
      m_deltaT(0.0)
  {
  }

  //----------------------------------------------------------------------------------------------
  /** Destructor
   */
  PoldiCalculateSpectrum2D::~PoldiCalculateSpectrum2D()
  {
  }


  //----------------------------------------------------------------------------------------------
  /// Algorithm's name for identification. @see Algorithm::name
  const std::string PoldiCalculateSpectrum2D::name() const { return "PoldiCalculateSpectrum2D";}

  /// Algorithm's version for identification. @see Algorithm::version
  int PoldiCalculateSpectrum2D::version() const { return 1;}

  /// Algorithm's category for identification. @see Algorithm::category
  const std::string PoldiCalculateSpectrum2D::category() const { return "SINQ\\Poldi\\PoldiSet";}

  const std::string PoldiCalculateSpectrum2D::summary() const
  {
      return "Calculates a POLDI 2D-spectrum.";
  }

  //----------------------------------------------------------------------------------------------
  /** Initialize the algorithm's properties.
   */
  void PoldiCalculateSpectrum2D::init()
  {
    declareProperty(new WorkspaceProperty<MatrixWorkspace>("InputWorkspace","",Direction::Input), "Measured POLDI 2D-spectrum.");
    declareProperty(new WorkspaceProperty<TableWorkspace>("PoldiPeakWorkspace", "", Direction::Input), "Table workspace with peak information.");
    declareProperty(new WorkspaceProperty<MatrixWorkspace>("OutputWorkspace","",Direction::Output), "Calculated POLDI 2D-spectrum");
  }

  //----------------------------------------------------------------------------------------------
  /** Execute the algorithm.
   */
  boost::shared_ptr<Poldi2DFunction> PoldiCalculateSpectrum2D::getFunctionFromPeakCollection(PoldiPeakCollection_sptr peakCollection)
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

  void PoldiCalculateSpectrum2D::setTimeTransformer(PoldiTimeTransformer_sptr poldiTimeTransformer)
  {
      m_timeTransformer = poldiTimeTransformer;
  }

  void PoldiCalculateSpectrum2D::exec()
  {
      TableWorkspace_sptr peakTable = getProperty("PoldiPeakWorkspace");
      if(!peakTable) {
          throw std::runtime_error("Cannot proceed without peak workspace.");
      }

      MatrixWorkspace_sptr ws = getProperty("InputWorkspace");
      setDeltaTFromWorkspace(ws);

      setTimeTransformerFromInstrument(PoldiInstrumentAdapter_sptr(new PoldiInstrumentAdapter(ws)));

      PoldiPeakCollection_sptr peakCollection = getPeakCollection(peakTable);

      setProperty("OutputWorkspace", calculateSpectrum(peakCollection, ws));
  }

  MatrixWorkspace_sptr PoldiCalculateSpectrum2D::calculateSpectrum(PoldiPeakCollection_sptr peakCollection, MatrixWorkspace_sptr matrixWorkspace)
  {
      PoldiPeakCollection_sptr integratedPeaks = getIntegratedPeakCollection(peakCollection);
      PoldiPeakCollection_sptr normalizedPeakCollection = getNormalizedPeakCollection(integratedPeaks);

      boost::shared_ptr<IFunction> mdFunction = getFunctionFromPeakCollection(normalizedPeakCollection);
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

  void PoldiCalculateSpectrum2D::setTimeTransformerFromInstrument(PoldiInstrumentAdapter_sptr poldiInstrument)
  {
      setTimeTransformer(PoldiTimeTransformer_sptr(new PoldiTimeTransformer(poldiInstrument)));
  }

  void PoldiCalculateSpectrum2D::setDeltaTFromWorkspace(MatrixWorkspace_sptr matrixWorkspace)
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

  void PoldiCalculateSpectrum2D::setDeltaT(double newDeltaT)
  {
      if(!isValidDeltaT(newDeltaT)) {
          throw std::invalid_argument("Time bin size must be larger than 0.");
      }

      m_deltaT = newDeltaT;
  }

  bool PoldiCalculateSpectrum2D::isValidDeltaT(double deltaT)
  {
      return deltaT > 0.0;
  }

  PoldiPeakCollection_sptr PoldiCalculateSpectrum2D::getPeakCollection(TableWorkspace_sptr peakTable)
  {
      try {
          return PoldiPeakCollection_sptr(new PoldiPeakCollection(peakTable));
      } catch(...) {
          throw std::runtime_error("Could not initialize peak collection.");
      }
  }

  PoldiPeakCollection_sptr PoldiCalculateSpectrum2D::getIntegratedPeakCollection(PoldiPeakCollection_sptr rawPeakCollection)
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

      PoldiPeakCollection_sptr integratedPeakCollection(new PoldiPeakCollection(PoldiPeakCollection::Integral));
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
          integratedPeak->setD(integratedPeak->d() - 0.0005);
          integratedPeak->setIntensity(UncertainValue(integration.result / m_deltaT));
          integratedPeakCollection->addPeak(integratedPeak);
      }

      return integratedPeakCollection;
  }

  PoldiPeakCollection_sptr PoldiCalculateSpectrum2D::getNormalizedPeakCollection(PoldiPeakCollection_sptr peakCollection)
  {
      if(!peakCollection) {
          throw std::invalid_argument("Cannot proceed with invalid PoldiPeakCollection.");
      }

      if(!m_timeTransformer) {
          throw std::invalid_argument("Cannot proceed without PoldiTimeTransformer.");
      }

      PoldiPeakCollection_sptr normalizedPeakCollection(new PoldiPeakCollection(PoldiPeakCollection::Integral));
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
