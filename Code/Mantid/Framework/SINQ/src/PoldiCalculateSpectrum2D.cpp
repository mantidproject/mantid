/*WIKI*
TODO: Enter a full wiki-markup description of your algorithm here. You can then use the Build/wiki_maker.py script to generate your full wiki page.
*WIKI*/

#include "MantidSINQ/PoldiCalculateSpectrum2D.h"

#include "MantidDataObjects/Workspace2D.h"
#include "MantidAPI/FunctionFactory.h"
#include "MantidAPI/MultiDomainFunction.h"
#include "MantidSINQ/PoldiUtilities/PoldiSpectrumDomainFunction.h"

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
    declareProperty(new WorkspaceProperty<MatrixWorkspace>("InputWorkspace","",Direction::Input), "An input workspace.");
    declareProperty(new WorkspaceProperty<MatrixWorkspace>("OutputWorkspace","",Direction::Output), "An output workspace.");
  }

  //----------------------------------------------------------------------------------------------
  /** Execute the algorithm.
   */
  void PoldiCalculateSpectrum2D::exec()
  {
      IFunction_sptr peakFunction = FunctionFactory::Instance().createFunction("PoldiSpectrumDomainFunction");
      peakFunction->setParameter("Area", 1.9854805);
      peakFunction->setParameter("Fwhm", 0.00274463167);
      peakFunction->setParameter("Centre", 1.1086444);

      boost::shared_ptr<MultiDomainFunction> mdFunction(new MultiDomainFunction);
      mdFunction->addFunction(peakFunction);

      MatrixWorkspace_sptr ws = getProperty("InputWorkspace");

      std::vector<size_t> wsi(400);
      for(size_t w = 0; w < wsi.size(); ++w) {
          wsi[w] = w;
      }

      mdFunction->setDomainIndices(0, wsi);
      mdFunction->setLocalAttribute(0, "domains", API::IFunction::Attribute("All"));

      IAlgorithm_sptr fit = createChildAlgorithm("Fit", -1, -1, true);
      fit->setProperty("Function", boost::dynamic_pointer_cast<IFunction>(mdFunction));
      fit->setProperty("InputWorkspace", ws);

      for(size_t i = 1; i < 400; ++i) {
          fit->setProperty("InputWorkspace_" + boost::lexical_cast<std::string>(i), ws);
          //fit->setProperty("WorkspaceIndex_" + boost::lexical_cast<std::string>(i + 1), i);
      }

      fit->setProperty("CreateOutput", true);
      fit->setProperty("MaxIterations", 0);

      fit->execute();

      setProperty("OutputWorkspace", fit->getPropertyValue("OutputWorkspace"));
  }
  // TODO Auto-generated execute stub



} // namespace Poldi
} // namespace Mantid
