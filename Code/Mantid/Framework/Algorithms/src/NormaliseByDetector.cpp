/*WIKI*
Normalise a workspace by the detector efficiency.
*WIKI*/

#include "MantidAlgorithms/NormaliseByDetector.h"
#include "MantidKernel/System.h"
#include "MantidAPI/FunctionFactory.h"
#include "MantidAPI/MatrixWorkspace.h"
#include "MantidGeometry/Instrument/ParameterMap.h"
#include "MantidGeometry/Instrument/Component.h"
#include "MantidGeometry/Instrument/DetectorGroup.h"
#include "MantidGeometry/Instrument/FitParameter.h"

using namespace Mantid::Kernel;
using namespace Mantid::API;
using namespace Mantid::Geometry;

namespace Mantid
{
namespace Algorithms
{

  // Register the algorithm into the AlgorithmFactory
  DECLARE_ALGORITHM(NormaliseByDetector)
  


  //----------------------------------------------------------------------------------------------
  /** Constructor
   */
  NormaliseByDetector::NormaliseByDetector()
  {
  }
    
  //----------------------------------------------------------------------------------------------
  /** Destructor
   */
  NormaliseByDetector::~NormaliseByDetector()
  {
  }
  

  //----------------------------------------------------------------------------------------------
  /// Algorithm's name for identification. @see Algorithm::name
  const std::string NormaliseByDetector::name() const { return "NormaliseByDetector";};
  
  /// Algorithm's version for identification. @see Algorithm::version
  int NormaliseByDetector::version() const { return 1;};
  
  /// Algorithm's category for identification. @see Algorithm::category
  const std::string NormaliseByDetector::category() const { return "General";}

  //----------------------------------------------------------------------------------------------
  /// Sets documentation strings for this algorithm
  void NormaliseByDetector::initDocs()
  {
    this->setWikiSummary("Normalise the input workspace by the detector efficiency.");
    this->setOptionalMessage("Normalise the input workspace by the detector efficiency.");
  }

  //----------------------------------------------------------------------------------------------
  /** Initialize the algorithm's properties.
   */
  void NormaliseByDetector::init()
  {
    declareProperty(new WorkspaceProperty<MatrixWorkspace>("InputWorkspace","",Direction::Input), "An input workspace.");
    declareProperty(new WorkspaceProperty<MatrixWorkspace>("OutputWorkspace","",Direction::Output), "An output workspace.");
  }

  //----------------------------------------------------------------------------------------------
  /** Execute the algorithm.
   */
  void NormaliseByDetector::exec()
  {
    MatrixWorkspace_sptr inWS = getProperty("InputWorkspace");

    const Geometry::ParameterMap& paramMap = inWS->instrumentParameters();

    for(size_t wsIndex = 0; wsIndex < inWS->getNumberHistograms(); ++wsIndex)
    {
      Geometry::IDetector_const_sptr det = inWS->getDetector( wsIndex );
      const std::string type = "fitting";
      Geometry::Parameter_sptr fittingParam = paramMap.getRecursiveByType(&(*det), type);
      if(fittingParam == NULL)
      {
        std::stringstream stream;
        stream << det->getName() << " and all of it's parent components, have no fitting type parameters. This algorithm cannot be run without fitting parameters.";
        this->g_log.warning(stream.str());
        throw std::invalid_argument(stream.str());
      }
    }

    setProperty("OutputWorkspace", inWS); //HACK
  }

} // namespace Mantid
} // namespace Algorithms