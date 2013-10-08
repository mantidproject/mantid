/*WIKI*
TODO: Enter a full wiki-markup description of your algorithm here. You can then use the Build/wiki_maker.py script to generate your full wiki page.
*WIKI*/


#include "MantidAlgorithms/RadiusSum.h"
#include "MantidKernel/VisibleWhenProperty.h"
#include "MantidKernel/ArrayLengthValidator.h"
#include "MantidKernel/BoundedValidator.h"
#include "MantidKernel/ArrayProperty.h"

#include <limits>

using namespace Mantid::Kernel;

namespace Mantid
{
namespace Algorithms
{

  // Register the algorithm into the AlgorithmFactory
  DECLARE_ALGORITHM(RadiusSum)
  


  //----------------------------------------------------------------------------------------------
  /** Constructor
   */
  RadiusSum::RadiusSum()
  {
  }
    
  //----------------------------------------------------------------------------------------------
  /** Destructor
   */
  RadiusSum::~RadiusSum()
  {
  }
  

  //----------------------------------------------------------------------------------------------
  /// Algorithm's name for identification. 
  const std::string RadiusSum::name() const { return "RadiusSum";}
  
  /// Algorithm's version for identification. 
  int RadiusSum::version() const { return 1;}
  
  /// Algorithm's category for identification.
  const std::string RadiusSum::category() const { return "Transforms";}

  //----------------------------------------------------------------------------------------------
  /// Sets documentation strings for this algorithm
  void RadiusSum::initDocs()
  {
    this->setWikiSummary("Sum of all the counts inside a ring against the scattering angle for each Radius.");
    this->setOptionalMessage("Sum of all the counts inside a ring against the scattering angle for each Radius.");
  }

  //----------------------------------------------------------------------------------------------
  /** Initialize the algorithm's properties.
   */
  void RadiusSum::init()
  {
    declareProperty(new API::WorkspaceProperty<API::MatrixWorkspace>("InputWorkspace","",Direction::Input), "An input workspace.");
    declareProperty(new API::WorkspaceProperty<>("OutputWorkspace","",Direction::Output), "An output workspace.");
    
    auto twoOrThreeElements = boost::make_shared<ArrayLengthValidator<double> >(2,3);
    std::vector<double> myInput(3,0);
    declareProperty( new ArrayProperty<double>("Centre", myInput, twoOrThreeElements),
                     "Coordinate of the centre of the ring");
    auto nonNegative = boost::make_shared<BoundedValidator<double> >(); 
    nonNegative->setLower(0); 
    
    declareProperty("MinRadius",0.0, nonNegative, "Lenght of the inner ring. Default=0"); 
    declareProperty(new PropertyWithValue<double>("MaxRadius", std::numeric_limits<double>::max(), nonNegative), 
                    "Lenght of the outer ring. Default=ImageSize.");
    
    auto nonNegativeInt = boost::make_shared<BoundedValidator<int> >(); 
    nonNegativeInt->setLower(1); 
    declareProperty("NumBins", 100, nonNegativeInt, "Number of slice bins for the output. Default=100"); 
    
    const char * normBy = "NormalizeByRadius"; 
    declareProperty(normBy, false, "Divide the sum of each ring by the radius powered by Normalization Order"); 
    
    const char * normOrder = "NormalizationOrder"; 
    declareProperty(normOrder, 1.0, "If 2, the normalization will be divided by the quadratic value of the ring for each radius.");
    setPropertySettings(normOrder, new VisibleWhenProperty(normBy, IS_EQUAL_TO, "1"));
    const char * groupNorm = "Normalization"; 
    setPropertyGroup(normBy, groupNorm); 
    setPropertyGroup(normOrder, groupNorm);        
  }

  //----------------------------------------------------------------------------------------------
  /** Execute the algorithm.
   */
  void RadiusSum::exec()
  {
    // TODO Auto-generated execute stub
  }



} // namespace Algorithms
} // namespace Mantid
