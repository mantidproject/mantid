/*WIKI*
 This algorithm ...

 The output [[TableWorkspace]] contains the following columns...

 ==== Subalgorithms used ====
 ...

 ==== Treating weak peaks vs. high background ====
 FindPeaks uses a more complicated approach to fit peaks if '''HighBackground''' is flagged. In this case, FindPeak will fit the background first, and then do a Gaussian fit the peak with the fitted background removed.  This procedure will be repeated for a couple of times with different guessed peak widths.  And the parameters of the best result is selected.  The last step is to fit the peak with a combo function including background and Gaussian by using the previously recorded best background and peak parameters as the starting values.

 ==== Criteria To Validate Peaks Found ====
 FindPeaks finds peaks by fitting a Guassian with background to a certain range in the input histogram.  [[Fit]] may not give a correct result even if chi^2 is used as criteria alone.  Thus some other criteria are provided as options to validate the result
 1. Peak position.  If peak positions are given, and trustful, then the fitted peak position must be within a short distance to the give one.
 2. Peak height.  In the certain number of trial, peak height can be used to select the best fit among various starting sigma values.

 ==== Fit Window ====
 If FitWindows is defined, then a peak's range to fit (i.e., x-min and x-max) is confined by this window.

 If FitWindows is defined, starting peak centres are NOT user's input, but found by highest value within peak window. (Is this correct???)


 *WIKI*/
//----------------------------------------------------------------------
// Includes
//----------------------------------------------------------------------
#include "MantidAlgorithms/FitPeak.h"
#include "MantidAPI/FunctionProperty.h"
#include "MantidAPI/MatrixWorkspace.h"
#include "MantidAPI/WorkspaceProperty.h"
#include "MantidDataObjects/TableWorkspace.h"
#include "MantidKernel/ArrayProperty.h"
#include "MantidKernel/BoundedValidator.h"

using namespace Mantid;
using namespace Mantid::API;
using namespace Mantid::DataObjects;
using namespace Mantid::Kernel;

using namespace std;

namespace Mantid
{
namespace Algorithms
{

  DECLARE_ALGORITHM(FitPeak)

  //----------------------------------------------------------------------------------------------
  /** Constructor
   */
  FitPeak::FitPeak()
  {
  }
    
  //----------------------------------------------------------------------------------------------
  /** Destructor
   */
  FitPeak::~FitPeak()
  {
  }

  //----------------------------------------------------------------------------------------------
  /** Document
   */
  void FitPeak::initDocs()
  {
    setWikiSummary("");
    setOptionalMessage("");

    return;
  }

  //----------------------------------------------------------------------------------------------
  /** Declare properties
   */
  void FitPeak::init()
  {
    declareProperty(new WorkspaceProperty<MatrixWorkspace>("InputWorkspace", "", Direction::Input),
                    "Name of the input workspace for peak fitting.");

    declareProperty(new WorkspaceProperty<MatrixWorkspace>("OutputWorkspace", "", Direction::Output),
                    "Name of the output workspace containing fitted peak.");

    declareProperty(new WorkspaceProperty<TableWorkspace>("ParameterTableWorkspace", "", Direction::Output),
                    "Name of the table workspace containing the fitted parameters. ");

    boost::shared_ptr<BoundedValidator<int> > mustBePositive = boost::make_shared<BoundedValidator<int> >();
    mustBePositive->setLower(0);
    declareProperty("WorkspaceIndex", 0, mustBePositive, "Workspace index ");

    declareProperty(new FunctionProperty("PeakFunction"),
                    "Peak function parameters defining the fitting function and its initial values");

    declareProperty(new FunctionProperty("BackgroundFunction"),
                    "Background function parameters defining the fitting function and its initial values");

    declareProperty(new ArrayProperty<double>("FitWindow"),
                    "Enter a comma-separated list of the expected X-position of windows to fit. "
                    "The number of values must be 2.");

    declareProperty(new ArrayProperty<double>("PeakRange"),
                    "Enter a comma-separated list of expected x-position as peak range. "
                    "The number of values must be 2.");

    declareProperty("FitBackgroundFirst", true, "If true, then the algorithm will fit background first. "
                    "And then the peak. ");

    declareProperty("RawParams", true, "If true, then the output table workspace contains the raw profile parameter. "
                    "Otherwise, the effective parameters will be written. ");

    return;
  }

  //----------------------------------------------------------------------------------------------
  /** Declare properties
   */
  void FitPeak::exec()
  {



    return;
  }

  


} // namespace Algorithms
} // namespace Mantid
