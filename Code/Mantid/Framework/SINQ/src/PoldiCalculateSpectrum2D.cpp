/*WIKI*
TODO: Enter a full wiki-markup description of your algorithm here. You can then use the Build/wiki_maker.py script to generate your full wiki page.
*WIKI*/

#include "MantidSINQ/PoldiCalculateSpectrum2D.h"

#include "MantidDataObjects/Workspace2D.h"
#include "MantidDataObjects/TableWorkspace.h"
#include "MantidAPI/FunctionFactory.h"
#include "MantidAPI/MultiDomainFunction.h"
#include "MantidSINQ/PoldiUtilities/PoldiSpectrumDomainFunction.h"

#include "MantidSINQ/PoldiUtilities/PoldiPeakCollection.h"
#include "MantidSINQ/PoldiUtilities/PoldiInstrumentAdapter.h"
#include "MantidSINQ/PoldiUtilities/PeakFunctionIntegrator.h"
#include "MantidAPI/IPeakFunction.h"

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
  boost::shared_ptr<MultiDomainFunction> PoldiCalculateSpectrum2D::getMultiDomainFunctionFromPeakCollection(PoldiPeakCollection_sptr peakCollection)
  {
      boost::shared_ptr<MultiDomainFunction> mdFunction(new MultiDomainFunction);

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

  void PoldiCalculateSpectrum2D::exec()
  {
      TableWorkspace_sptr peakTable = getProperty("PoldiPeakWorkspace");
      if(!peakTable) {
          throw std::runtime_error("Cannot proceed without peak workspace.");
      }

      MatrixWorkspace_sptr ws = getProperty("InputWorkspace");
      m_deltaT = ws->readX(0)[1] - ws->readX(0)[0];

      if(m_deltaT <= 0.0) {
          throw std::runtime_error("Invalid time bin size.");
      }

      PoldiInstrumentAdapter_sptr poldiInstrument(new PoldiInstrumentAdapter(ws->getInstrument(), ws->run()));
      m_timeTransformer = PoldiTimeTransformer_sptr(new PoldiTimeTransformer(poldiInstrument));

      PoldiPeakCollection_sptr peakCollection = getPeakCollection(peakTable);

      if(peakCollection->intensityType() != PoldiPeakCollection::Integral) {
          /* Intensities must be integral for fitting procedure, so they have to be integrated
           * and put into a new PoldiPeakCollection.
           */

          peakCollection = getIntegratedPeakCollection(peakCollection);
      }

      peakCollection = getNormalizedPeakCollection(peakCollection);

      boost::shared_ptr<MultiDomainFunction> mdFunction = getMultiDomainFunctionFromPeakCollection(peakCollection);

      size_t nspec = ws->getNumberHistograms();
      std::vector<size_t> wsi(nspec);
      for(size_t w = 0; w < wsi.size(); ++w) {
          wsi[w] = w;
      }

      mdFunction->setDomainIndices(0, wsi);

      IAlgorithm_sptr fit = createChildAlgorithm("Fit", -1, -1, true);
      fit->setProperty("Function", boost::dynamic_pointer_cast<IFunction>(mdFunction));
      fit->setProperty("InputWorkspace", ws);
      fit->setProperty("WorkspaceIndex", 0);

      for(size_t i = 1; i < nspec; ++i) {
          fit->setProperty("InputWorkspace_" + boost::lexical_cast<std::string>(i), ws);
          fit->setProperty("WorkspaceIndex_" + boost::lexical_cast<std::string>(i), static_cast<int>(i));
      }

      fit->setProperty("CreateOutput", true);
      fit->setProperty("MaxIterations", 0);
      fit->setProperty("CalcErrors", false);

      fit->execute();
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
          integratedPeak->setIntensity(UncertainValue(integration.result / m_deltaT));
          integratedPeakCollection->addPeak(integratedPeak);
      }

      return integratedPeakCollection;
  }

  PoldiPeakCollection_sptr PoldiCalculateSpectrum2D::getNormalizedPeakCollection(PoldiPeakCollection_sptr peakCollection)
  {
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
