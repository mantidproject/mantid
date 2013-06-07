/*WIKI*
This algorithm allows users to adjust the axes of a workspace by a user defined math formula.  
It will NOT adjust or rearrange the data values (other than in one case the X values) of a workspace.  
Therefore alterations that will rearrange the order of the axes are not recommended.
This only works for MatrixWorkspaces, so will not work on Multi Dimensional Workspaces or Table Workspaces.
Like the [[ConvertSpectrumAxis]] algorithm the result of this algorithm will have custom units defined for the axis you have altered, and as such may not work in all other algorithms. 

The algorithm can operate on the X or Y axis, but cannot alter the values of a spectrum axis (the axis used as the Y axis on newly loaded Raw data).  If you wish to alter this axis use he [[ConvertSpectrumAxis]] algorithm first.

The formula is defined in a simple math syntax, please look at the usage examples to some ideas of what is possible, 
a full list of the functions available can be found at the muparser website [http://muparser.beltoforion.de/mup_features.html#idDef2].
*WIKI*/
/*WIKI_USAGE*
Squaring the X axis (assuming it is in Q)
result = ConvertAxisByFormula(InputsWorkspace="input", Axis="X", Formula="x*x",AxisTitle="QSquared",AxisUnit="Q2")

Other examples of the Math format are as follows:
* Squaring - x^2
* Square root - sqrt(x)
* Cubing - x^3
* Basic addition - y + 3
* Brackets - (y+1)/20
* Natural Log - ln(x)
* Log 10 - log(x)
* exponent - exp(y)
* round to nearest integer - rint(y/10)
* absolute value - abs(y-100)
Use can use x or y interchangeably to refer to the current axis value.
*WIKI_USAGE*/
#include "MantidAlgorithms/ConvertAxisByFormula.h"
#include "MantidAPI/WorkspaceValidators.h"
#include "MantidKernel/ListValidator.h"
#include "MantidGeometry/muParser_Silent.h"
#include "MantidAPI/RefAxis.h"
#include "MantidKernel/UnitFactory.h"

#include <boost/shared_ptr.hpp>
#include <boost/make_shared.hpp>
#include <sstream>

namespace Mantid
{
  namespace Algorithms
  {

    using namespace Kernel;
    using namespace API;

    // Register the class into the algorithm factory
    DECLARE_ALGORITHM(ConvertAxisByFormula)

    //----------------------------------------------------------------------------------------------
    /** Constructor
    */
    ConvertAxisByFormula::ConvertAxisByFormula()
    {
    }

    //----------------------------------------------------------------------------------------------
    /** Destructor
    */
    ConvertAxisByFormula::~ConvertAxisByFormula()
    {
    }

    const std::string ConvertAxisByFormula::name() const
    {
      return ("ConvertAxisByFormula");
    }

    int ConvertAxisByFormula::version() const
    {
      return (1);
    }

    const std::string  ConvertAxisByFormula::category() const
    {
      return "Transforms\\Axes";
    }

    /// Sets documentation strings for this algorithm
    void ConvertAxisByFormula::initDocs()
    {
      this->setWikiSummary("Converts the X or Y axis of a [[MatrixWorkspace]] via a user defined math formula.");
      this->setOptionalMessage("Converts the X or Y axis of a MatrixWorkspace via a user defined math formula.");
    }

    /** Initialisation method. Declares properties to be used in algorithm.
    *
    */
    void ConvertAxisByFormula::init()
    {
      declareProperty(new WorkspaceProperty<MatrixWorkspace>("InputWorkspace","",Direction::Input),
        "Name of the input workspace");
      declareProperty(new WorkspaceProperty<MatrixWorkspace>("OutputWorkspace","",Direction::Output),
        "Name of the output workspace");

      std::vector<std::string> axisOptions;
      axisOptions.push_back("X");
      axisOptions.push_back("Y");
      declareProperty("Axis","X",boost::make_shared<StringListValidator>(axisOptions),
        "The axis to modify (default: X)");

      declareProperty("Formula", "", "The formula to use to convert the values, x or y may be used to refer to the axis values");
      declareProperty("AxisTitle", "", "The label of he new axis. If not set then the title will not change.");
      declareProperty("AxisUnits", "", "The units of the new axis. If not set then the unit will not change");

    }

    /** Execution of the algorithm
    *
    */
    void ConvertAxisByFormula::exec()
    {
      //get the property values
      MatrixWorkspace_sptr inputWs=getProperty("InputWorkspace");
      std::string axis = getProperty("Axis");
      std::string formula = getProperty("Formula");
      std::string axisTitle = getProperty("AxisTitle");
      std::string axisUnits = getProperty("AxisUnits");


      // Just overwrite if the change is in place
      MatrixWorkspace_sptr outputWs = getProperty("OutputWorkspace");
      if (outputWs != inputWs)
      {
        IAlgorithm_sptr duplicate = createChildAlgorithm("CloneWorkspace",0.0,0.6);
        duplicate->initialize();
        duplicate->setProperty<Workspace_sptr>("InputWorkspace", boost::dynamic_pointer_cast<Workspace>(inputWs));
        duplicate->execute();
        Workspace_sptr temp = duplicate->getProperty("OutputWorkspace");
        outputWs = boost::dynamic_pointer_cast<MatrixWorkspace>(temp);

        setProperty("OutputWorkspace", outputWs);
      }

      //Get the axis
      int axisIndex = 0; //assume X
      if (axis=="Y")
      {
        axisIndex=1;
      }
      Axis* axisPtr = outputWs->getAxis(axisIndex);

      if (!axisPtr->isNumeric())
      {
        throw std::invalid_argument("This algorithm only operates on numeric axes");
      }

      bool isRefAxis = false;
      RefAxis* refAxisPtr = dynamic_cast<RefAxis*>(axisPtr);
      if (refAxisPtr != NULL)
      {
        CommonBinsValidator sameBins;
        if (sameBins.isValid(outputWs) != "")
        {
          throw std::invalid_argument("Axes must have common bins for this algorithm to work - try Rebin first");
        }
        isRefAxis = true;
      }

      double axisValue(0);
      //Create muparser
      try
      {
        mu::Parser p;
        //set parameter lookups for the axis value, allow both cases
        p.DefineVar("y", &axisValue);
        p.DefineVar("x", &axisValue);
        p.DefineVar("Y", &axisValue);
        p.DefineVar("X", &axisValue);
        p.SetExpr(formula);
        try
        {
          if(isRefAxis)
          {
            int64_t numberOfSpectra_i = static_cast<int64_t>(outputWs->getNumberHistograms()); // cast to make openmp happy
            // Calculate the new (common) X values
            MantidVec::iterator iter;
            for (iter = outputWs->dataX(0).begin(); iter != outputWs->dataX(0).end(); ++iter)
            {
              axisValue = *iter;
              double result = p.Eval();
              *iter = result;
            }

            MantidVecPtr xVals;
            xVals.access() = outputWs->dataX(0);
            Progress prog(this,0.6,1.0,numberOfSpectra_i);
            PARALLEL_FOR1(outputWs)
              for (int64_t j = 1; j < numberOfSpectra_i; ++j)
              {
                PARALLEL_START_INTERUPT_REGION
                  outputWs->setX(j,xVals);
                prog.report();
                PARALLEL_END_INTERUPT_REGION
              }
              PARALLEL_CHECK_INTERUPT_REGION
          }
          else
          {
            size_t axisLength = axisPtr->length();
            for (size_t i=0;i<axisLength;++i)
            {
              axisValue = axisPtr->getValue(i);
              double result = p.Eval();
              axisPtr->setValue(i,result);
            }
          }
        }
        catch (mu::Parser::exception_type &e)
        {
          std::stringstream ss;
          ss << "Failed while processing axis values"  << ". Muparser error message is: " << e.GetMsg();
          throw std::invalid_argument(ss.str());
        }
      }
      catch (mu::Parser::exception_type &e)
      {
        std::stringstream ss;
        ss << "Cannot process the formula"  << ". Muparser error message is: " << e.GetMsg();
        throw std::invalid_argument(ss.str());
      }

      if ((axisUnits!="") || (axisTitle!=""))
      {
        if (axisTitle=="")
        {
          axisTitle = axisPtr->unit()->caption();
        }        
        if (axisUnits=="")
        {
          axisUnits = axisPtr->unit()->label();
        }
        axisPtr->unit() = boost::shared_ptr<Unit>(new Units::CustomUnit(axisTitle,axisUnits));
      }

    }


  } // namespace Algorithms
} // namespace Mantid
