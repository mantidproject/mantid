/*WIKI*
Takes an input TOF spectrum and converts it to Y-space using the [[ConvertToYSpace]] algorithm. The result is
then fitted using the ComptonPeakProfile function to produce an estimate of the peak area. The input data is
normalised by this value.
 *WIKI*/

#include "MantidCurveFitting/NormaliseByPeakArea.h"
#include "MantidAPI/WorkspaceValidators.h"
#include "MantidKernel/BoundedValidator.h"

#include <boost/make_shared.hpp>

namespace Mantid
{
  namespace CurveFitting
  {

    // Register the algorithm into the AlgorithmFactory
    DECLARE_ALGORITHM(NormaliseByPeakArea)

    using namespace API;
    using namespace Kernel;

    //----------------------------------------------------------------------------------------------
    /** Constructor
     */
    NormaliseByPeakArea::NormaliseByPeakArea()
      : API::Algorithm(), m_inputWS(), m_mass(0.0)
    {
    }

    //----------------------------------------------------------------------------------------------
    /// Algorithm's name for identification. @see Algorithm::name
    const std::string NormaliseByPeakArea::name() const { return "NormaliseByPeakArea"; }

    /// Algorithm's version for identification. @see Algorithm::version
    int NormaliseByPeakArea::version() const { return 1; }

    /// Algorithm's category for identification. @see Algorithm::category
    const std::string NormaliseByPeakArea::category() const { return "Corrections"; }

    //----------------------------------------------------------------------------------------------
    /// Sets documentation strings for this algorithm
    void NormaliseByPeakArea::initDocs()
    {
      this->setWikiSummary("Normalises the input data by the area of of peak defined by the input mass value.");
      this->setOptionalMessage("Normalises the input data by the area of of peak defined by the input mass value.");
    }

    //----------------------------------------------------------------------------------------------
    /** Initialize the algorithm's properties.
     */
    void NormaliseByPeakArea::init()
    {
      auto wsValidator = boost::make_shared<CompositeValidator>();
      wsValidator->add<HistogramValidator>(false); // point data
      wsValidator->add<InstrumentValidator>();
      wsValidator->add<WorkspaceUnitValidator>("TOF");
      declareProperty(new WorkspaceProperty<>("InputWorkspace","",Direction::Input,wsValidator),
                      "An input workspace.");

      auto mustBePositive = boost::make_shared<BoundedValidator<double> >();
      mustBePositive->setLower(0.0);
      mustBePositive->setLowerExclusive(true); //strictly greater than 0.0
      declareProperty("Mass",-1.0,mustBePositive,"The mass, in AMU, defining the recoil peak to fit");

      declareProperty(new WorkspaceProperty<>("OutputWorkspace","",Direction::Output),
                      "Input workspace normalised by the fitted peak area");
    }

    //----------------------------------------------------------------------------------------------
    /** Execute the algorithm.
     */
    void NormaliseByPeakArea::exec()
    {
      retrieveInputs();
      createOutputWorkspace();

      // -- Convert to Y-space --
      auto yspace = convertInputToY();

      setProperty("OutputWorkspace", yspace);
    }

    /*
     * Returns a workspace converted to Y-space coordinates. @see ConvertToYSpace
     */
    MatrixWorkspace_sptr NormaliseByPeakArea::convertInputToY()
    {
      auto alg = createChildAlgorithm("ConvertToYSpace",0.0, 0.33,false);
      alg->setProperty("InputWorkspace", m_inputWS);
      alg->setProperty("Mass", m_mass);
      alg->execute();
      return alg->getProperty("OutputWorkspace");
    }

    /**
     * Caches input details for the peak information
     */
    void NormaliseByPeakArea::retrieveInputs()
    {
      m_inputWS = getProperty("InputWorkspace");
      m_mass = getProperty("Mass");
    }

    /**
     * Create & cache output workspaces
     */
    void NormaliseByPeakArea::createOutputWorkspace()
    {
      m_outputWS = WorkspaceFactory::Instance().create(m_inputWS);
    }


  } // namespace CurveFitting
} // namespace Mantid
