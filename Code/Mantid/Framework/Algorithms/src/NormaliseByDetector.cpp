/*WIKI*

This algorithm is designed to normalise a workspace via detector efficiency functions. '''For this algorithm to work, the Instrument Defintion File [[IDF]] must have fitting functions on the component tree'''. The setup information for this, as well as some examples, are provided below.

At a high-level, the algorithm does this:

# Extract a detector efficiency function <math>e = f(\lambda)</math>
# Using the bin boundaries on the input workspace, calculate efficiency Y and E values and generate a new workspace from the results
# Divide the input workspace by the efficiency workspace
== Prerequisites ==

=== The Input Workspace ===
# The input workspace must be a MatrixWorkspace
# The input workspace must have X-units of Wavelength, run [[ConvertUnits]] on your input workspace if it is not already in Wavelength.
# The input workspace must be a histogram workspace run [[ConvertToHistogram]] on your input workspace if it is not already histogrammed.

=== The Instrument Definition File ===
==== Background ====
In brief, the components in the IDF file form a tree structure. Detectors and Instruments are both types of component. Detectors are ultimately children of Instruments in the tree structure. For a more complete description see [[IDF]]. The tree structure of the components, mean that fitting functions do not necessarily have to be assigned on a detector-by-detector basis. Applying a fit function to the instrument, will ensure that all subcomponents (including detectors), pick-up that function. However, functions assigned to lower-level components (such as detectors) take precidence over and exising functions that might exist on parent components (such as the instrument). You can even, have some parameters for a function provided against the detector, and pick up defaults from the bank, or instrument if they have been specified there. 
==== Recommended Working ====

The IDF is intended to be a definitive description of the components in the instrument at any time. This should be the most generic form of the instrument setup possible. To complement this, you may provide additional Instrument Parameter files, which can be used to overload settings in the IDF for purposes of configuration and calibration. '''We strongly recommend that fitting functions are provided via Instrument Parameter Files rather than directly in the IDF'''. This will give you more flexibility to change your fitting functions without the problems of synchronising the IDF across Mantid, and possible corruption resulting from ad-hoc changes. 

Instrument Parameter Files that take the form {InstrumentName}_Parameters.xml and live in the Instrument directory of Mantid are automatically loaded along with the IDF when a workspace is loaded into Mantid. However, you can apply any number of additional parameter files over the top of an existing workspace using [[LoadParameterFile]]. 

==== Examples ====

Applying a LinearFunction to the whole instrument, hard-coded with A1=2 and A0=1. Fictional instrument is called basic_rect.

<div style="border:1pt dashed black; background:#f9f9f9;padding: 1em 0;">
<source lang="xml">
  <parameter-file instrument = "basic_rect" date = "2012-01-31T00:00:00">
    <component-link name="basic_rect">
    <parameter name="LinearBackground:A0" type="fitting">
      <formula eq="1" result-unit="Wavelength"/>
      <fixed />
    </parameter>
    <parameter name="LinearBackground:A1" type="fitting">
      <formula eq="2" result-unit="Wavelength"/>
      <fixed />
    </parameter>
    </component-link>
  </parameter-file>
</source></div>

Applying the same LinearFunction to two different detectors, with different coefficients is shown below:

<div style="border:1pt dashed black; background:#f9f9f9;padding: 1em 0;">
<source lang="xml">
<parameter-file instrument = "basic_rect" date = "2012-01-31T00:00:00">
<component-link name="bank1(0,0)">
   <parameter name="LinearBackground:A0" type="fitting">
       <formula eq="0" result-unit="Wavelength"/>
       <fixed />
   </parameter>
   <parameter name="LinearBackground:A1" type="fitting">
       <formula eq="1" result-unit="Wavelength"/>
       <fixed />
   </parameter>
</component-link>
<component-link name="bank2(0,0)">
   <parameter name="LinearBackground:A0" type="fitting">
       <formula eq="1" result-unit="Wavelength"/>
       <fixed />
   </parameter>
   <parameter name="LinearBackground:A1" type="fitting">
       <formula eq="1" result-unit="Wavelength"/>
       <fixed />
   </parameter>
</component-link>
</parameter-file>
</source></div>

In the following the LinearFunction A0 coefficient is set globally for all detectors at the instrument level, while the
A1 coefficient is provided for each detector. In this way the Algorithm sees a complete definition for the Linear function (both A1 and A0) from two incomplete definitions on different components in the tree.

<div style="border:1pt dashed black; background:#f9f9f9;padding: 1em 0;">
<source lang="xml">
<parameter-file instrument = "basic_rect" date = "2012-01-31T00:00:00">
<component-link name="basic_rect">
<parameter name="LinearBackground:A0" type="fitting">
   <formula eq="3" result-unit="Wavelength"/>
   <fixed />
</parameter>
</component-link>
<component-link name="bank1(0,0)">
<parameter name="LinearBackground:A1" type="fitting">
   <formula eq="0" result-unit="Wavelength"/>
   <fixed />
</parameter>
</component-link>
<component-link name="bank2(0,0)">
<parameter name="LinearBackground:A1" type="fitting">
   <formula eq="1" result-unit="Wavelength"/>
   <fixed />
</parameter>
</component-link>
</parameter-file>
</source></div>

*WIKI*/

#include "MantidAlgorithms/NormaliseByDetector.h"
#include "MantidKernel/System.h"
#include "MantidKernel/UnitFactory.h"
#include "MantidAPI/FunctionFactory.h"
#include "MantidAPI/MatrixWorkspace.h"
#include "MantidAPI/IFunction.h"
#include "MantidAPI/FunctionDomain1D.h" 
#include "MantidAPI/FunctionValues.h" 
#include "MantidAPI/WorkspaceValidators.h"
#include "MantidGeometry/Instrument/ParameterMap.h"
#include "MantidGeometry/Instrument/Component.h"
#include "MantidGeometry/Instrument/DetectorGroup.h"
#include "MantidGeometry/Instrument/FitParameter.h"
#include "MantidGeometry/muParser_Silent.h"

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
    NormaliseByDetector::NormaliseByDetector(bool parallelExecution) : m_parallelExecution(parallelExecution)
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
      auto compositeValidator = boost::make_shared<CompositeValidator>();
      compositeValidator->add(boost::make_shared<API::WorkspaceUnitValidator>("Wavelength"));
      compositeValidator->add(boost::make_shared<API::HistogramValidator>());

      declareProperty(new WorkspaceProperty<MatrixWorkspace>("InputWorkspace","",Direction::Input, compositeValidator),
        "An input workspace in wavelength");

      declareProperty(new WorkspaceProperty<MatrixWorkspace>("OutputWorkspace","",Direction::Output), "An output workspace.");
    }

    const Geometry::FitParameter NormaliseByDetector::tryParseFunctionParameter(Geometry::Parameter_sptr parameter, Geometry::IDetector_const_sptr det)
    {
      if(parameter == NULL)
      {
        std::stringstream stream;
        stream << det->getName() << " and all of it's parent components, have no fitting type parameters. This algorithm cannot be run without fitting parameters. See wiki help for details on setup.";
        this->g_log.warning(stream.str());
        throw std::runtime_error(stream.str());
      }
      return parameter->value<Geometry::FitParameter>();
    }


    /**
    Process each histogram of the input workspace, extracting the detector/component and looking up the efficiency function.
    Efficiency functions are then executed against the X data of the input workspace to generate new Y and E outputs for the denominatorWS.
    @param wsIndex: The index of the histogram in the input workspace to process.
    @param denominatorWS : Workspace that will become the denominator in the normalisation routine.
    @param inputWorkspace: Workspace input. Contains instrument to use as well as X data to use.
    */
    void NormaliseByDetector::processHistogram(size_t wsIndex, MatrixWorkspace_sptr denominatorWS, MatrixWorkspace_const_sptr inWS)
    {
      const Geometry::ParameterMap& paramMap = inWS->instrumentParameters();
      Geometry::IDetector_const_sptr det = inWS->getDetector( wsIndex );
      const std::string type = "fitting";
      Geometry::Parameter_sptr foundParam = paramMap.getRecursiveByType(&(*det), type);

      const Geometry::FitParameter& foundFittingParam = tryParseFunctionParameter(foundParam, det);

      std::string fitFunctionName = foundFittingParam.getFunction();
      IFunction_sptr function = FunctionFactory::Instance().createFunction(fitFunctionName);
      typedef std::vector<std::string> ParamNames;
      ParamNames allParamNames = function->getParameterNames();

      // Lookup each parameter name.
      for(ParamNames::iterator it = allParamNames.begin(); it !=  allParamNames.end(); ++it)
      {
        Geometry::Parameter_sptr param = paramMap.getRecursive(&(*det), (*it), type);

        const Geometry::FitParameter& fitParam = tryParseFunctionParameter(param, det);

        if ( fitParam.getFormula().compare("") == 0 )
        {
          throw std::runtime_error("A Forumla has not been provided for a fit function");
        }
        else
        {
          std::string resultUnitStr = fitParam.getResultUnit();
          if ( !resultUnitStr.empty() && resultUnitStr.compare("Wavelength") != 0)
          {
            throw std::runtime_error("Units for function parameters must be in Wavelength");
          }  
        } 
        mu::Parser p;
        p.SetExpr(fitParam.getFormula());
        double paramValue = p.Eval();
        //Set the function coeffiecents.
        function->setParameter(fitParam.getName(), paramValue);
      }

      auto wavelengths = inWS->readX(wsIndex);
      const size_t nInputBins =  wavelengths.size() -1;
      std::vector<double> centerPointWavelength(nInputBins);
      std::vector<double> outIntensity(nInputBins);
      for(size_t binIndex = 0; binIndex < nInputBins; ++binIndex)
      {
        centerPointWavelength[binIndex] = 0.5*(wavelengths[binIndex] + wavelengths[binIndex+1]);
      }
      FunctionDomain1DVector domain(centerPointWavelength);
      FunctionValues values(domain);
      function->function(domain,values);
      for(size_t i = 0; i < domain.size(); ++i)
      {
        outIntensity[i] = values[i];
      }
      denominatorWS->dataY(wsIndex) = outIntensity;
      denominatorWS->dataE(wsIndex) = MantidVec(nInputBins, 0);
    }

    /**
    Controlling function. Processes the histograms either in parallel or sequentially.
    @param denominatorWS : Workspace that will become the denominator in the normalisation routine.
    @param inputWorkspace: Workspace input. Contains instrument to use as well as X data to use.
    */
    MatrixWorkspace_sptr NormaliseByDetector::processHistograms(MatrixWorkspace_sptr inWS)
    {
      // Clone the input workspace to create a template for the denominator workspace.
      IAlgorithm_sptr cloneAlg = this->createSubAlgorithm("CloneWorkspace", 0.0, 1.0, true);
      cloneAlg->setProperty("InputWorkspace", inWS);
      cloneAlg->setPropertyValue("OutputWorkspace", "temp");
      cloneAlg->executeAsSubAlg();
      Workspace_sptr temp = cloneAlg->getProperty("OutputWorkspace");
      MatrixWorkspace_sptr denominatorWS = boost::dynamic_pointer_cast<MatrixWorkspace>(temp);

      // Choose between parallel execution and sequential execution then, process histograms accordingly.
      const size_t nHistograms = inWS->getNumberHistograms();
      if(m_parallelExecution == true)
      {
          PARALLEL_FOR2(inWS, denominatorWS)
          for(int wsIndex = 0; wsIndex < static_cast<int>(nHistograms); ++wsIndex)
          {
            PARALLEL_START_INTERUPT_REGION
              this->processHistogram(wsIndex, denominatorWS, inWS);
            PARALLEL_END_INTERUPT_REGION
          }
          PARALLEL_CHECK_INTERUPT_REGION
      }
      else
      {
        for(size_t wsIndex = 0; wsIndex < nHistograms; ++wsIndex)
        {
          this->processHistogram(wsIndex, denominatorWS, inWS);
        }
      }

      return denominatorWS;
    };

    //----------------------------------------------------------------------------------------------
    /** Execute the algorithm.
    */
    void NormaliseByDetector::exec()
    {
      MatrixWorkspace_sptr inWS = getProperty("InputWorkspace");

      // Do the work of extracting functions and applying them to each bin on each histogram. The denominator workspace is mutable.
      MatrixWorkspace_sptr denominatorWS = processHistograms(inWS);

      // Perform the normalisation.
      IAlgorithm_sptr divideAlg = this->createSubAlgorithm("Divide", 0.0, 1.0, true);
      divideAlg->setRethrows(true);
      divideAlg->setProperty("LHSWorkspace", inWS);
      divideAlg->setProperty("RHSWorkspace", denominatorWS);
      divideAlg->executeAsSubAlg();
      MatrixWorkspace_sptr outputWS = divideAlg->getProperty("OutputWorkspace");
      setProperty("OutputWorkspace", outputWS); 
    }

  } // namespace Mantid
} // namespace Algorithms