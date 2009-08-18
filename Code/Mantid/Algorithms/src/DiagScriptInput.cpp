#include "MantidAlgorithms/DiagScriptInput.h"
#include "MantidAPI/WorkspaceValidators.h"
#include "MantidKernel/BoundedValidator.h"
#include "MantidKernel/MandatoryValidator.h"
#include <string>

namespace Mantid
{
namespace Algorithms
{
// Register the class into the algorithm factory
DECLARE_ALGORITHM(DiagScriptInput)

using namespace Kernel;
using namespace API;

void DiagScriptInput::init()
{
  declareProperty( "OutputFile","",
    "A filename to which to write the list of dead detector UDETs" );
  BoundedValidator<double> *mustBePositive = new BoundedValidator<double>();
  mustBePositive->setLower(0);
  declareProperty("SignificanceTest", 3.3, mustBePositive->clone(),
    "Set this to a nonzero value and detectors in spectra with a total\n"
    "number of counts is within this number of standard deviations from the\n"
    "median will not be labelled bad (default 3.3)" );
  HistogramValidator<MatrixWorkspace> *val =
    new HistogramValidator<MatrixWorkspace>;
  declareProperty(
    new WorkspaceProperty<>("WBVanadium1","", Direction::Input, val),
    "Name of a white beam vanadium workspace" );
  // optional second WBV, it is OK if they add nothing here, if they enter a workspace name it is checked at run time
/*  // if this is changed from an empty string it must be the name of a workspace from the same instrument as the WBV workspaces
  declareProperty("ExperimentWorkspace", "",
    "The workspace that contains the experimental data that you\n"
    "want to correct" );*/
  declareProperty("HighAbsolute", 0.0,
    "Spectra whose total number of counts are above or equal to this value will be\n"
    "marked bad" );
  declareProperty("LowAbsolute",0.0,
    "Spectra whose total number of counts are below or equal to this value will be\n"
    "marked bad (default 0)" );
  declareProperty("HighMedian", 1.5, mustBePositive->clone(),
    "Detectors corresponding to spectra with total counts more than this\n"
    "number of the median would be labelled as reading badly (default 1.5)" );
  declareProperty("LowMedian", 0.1, mustBePositive->clone(),
    "Detectors corresponding to spectra with total counts less than this\n"
    "proportion of the median number of counts would be labelled as reading\n"
    "badly (default 0.1)" );
  declareProperty("WBVanadium2", "",
    "Name of a matching second white beam vanadium run from the same\n"
    "instrument" );
  declareProperty("Variation", -2.0, mustBePositive->clone(),
    "The ratio of total counts between equivalent histograms in the\n"
    "two white vanadiums are compared to the median variation. If the\n"
    "ratio is different by more than this fraction of the median that\n"
    "spectrum and associated detectors are marked bad");
  declareProperty( "Experimental", "",
    "An experimental run that will be used in the background test to find\n"
    "bad detectors" );
  declareProperty( "RemoveZero", false, "" );
  declareProperty( "MaskExper", false, "" );
  declareProperty( "BackgroundAccept", EMPTY_DBL(), "" );
  declareProperty( "RangeLower", EMPTY_DBL(),
    "No bin with a boundary at an x value less than this will be used\n"
    "in the summation that decides if a detector is 'bad' (default: the\n"
    "start of each histogram)" );
  declareProperty( "RangeUpper", EMPTY_DBL(),
    "No bin with a boundary at an x value higher than this value will\n"
    "be used in the summation that decides if a detector is 'bad'\n"
    "(default: the end of each histogram)" );
}

void DiagScriptInput::exec()
{
}

}
}