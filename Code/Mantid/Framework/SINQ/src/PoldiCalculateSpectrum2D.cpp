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

namespace Mantid
{
namespace Poldi
{
  // Register the algorithm into the AlgorithmFactory
  DECLARE_ALGORITHM(PoldiCalculateSpectrum2D)
  
  using namespace API;


  //----------------------------------------------------------------------------------------------
  /** Constructor
   */
  PoldiCalculateSpectrum2D::PoldiCalculateSpectrum2D()
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

  //----------------------------------------------------------------------------------------------
  /// Sets documentation strings for this algorithm
  void PoldiCalculateSpectrum2D::initDocs()
  {
    this->setWikiSummary("TODO: Enter a quick description of your algorithm.");
    this->setOptionalMessage("TODO: Enter a quick description of your algorithm.");
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

      PoldiPeakCollection_sptr peakCollection = getPeakCollection(peakTable);

      if(peakCollection->intensityType() != PoldiPeakCollection::Integral) {
          throw std::runtime_error("Peaks intensities need to be integral.");
      }

      boost::shared_ptr<MultiDomainFunction> mdFunction = getMultiDomainFunctionFromPeakCollection(peakCollection);

      MatrixWorkspace_sptr ws = getProperty("InputWorkspace");

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
  // TODO Auto-generated execute stub



} // namespace Poldi
} // namespace Mantid
