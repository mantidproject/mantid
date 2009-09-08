#include "MantidAlgorithms/DiagScriptInput.h"
#include "MantidAPI/WorkspaceValidators.h"
#include "MantidKernel/BoundedValidator.h"
#include "MantidKernel/MandatoryValidator.h"
#include "MantidKernel/FileProperty.h"
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
  declareProperty( new FileProperty("OutputFile","", FileProperty::Load),
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
  declareProperty("HighAbsolute", 1e8,
    "Spectra whose total number of counts are above or equal to this value will be\n"
    "marked bad (default 1x10^8)" );
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
  // optional second WBV, it is OK if they add nothing here, if they enter a workspace name it is checked at run time
  declareProperty("WBVanadium2", "",
    "Name of a matching second white beam vanadium run from the same\n"
    "instrument" );
  declareProperty("Variation", -2.0, mustBePositive->clone(),
    "The ratio of total counts between equivalent spectra in the\n"
    "two white vanadiums are compared to the median variation. If the\n"
    "ratio is different by more than this fraction of the median that\n"
    "spectrum and its associated detectors are marked bad");
  declareProperty( "Experimental", "",
    "An experimental run that will be used in the background test to find\n"
    "bad detectors" );
  declareProperty( "RemoveZero", false,
    "Identify histograms and their detectors that contain no counts in\n"
    "the background region" );
  declareProperty( "MaskExper", false,
    "Write to the experiment workspace filling all spectra identified\n"
    "with bad detectors with zeros and write to the detector mask map" );
  declareProperty( "BackgroundAccept", -2.0, mustBePositive->clone(),
    "Accept spectra whose white beam normalised background count is no\n"
    "more than this factor of the median count and identify the others\n"
    "as bad" );
  declareProperty( "RangeLower", EMPTY_DBL(),
    "Marks the start of background region.  It is non-inclusive, the bin\n"
    "that contains this x value wont be used  (default: the start of each\n"
    "histogram)" );
  declareProperty( "RangeUpper", EMPTY_DBL(),
    "Marks the end of background region.  It is non-inclusive, the bin\n"
    "that contains this x value wont be used (default: the end of each\n"
    "histogram)" );
}

void DiagScriptInput::exec()
{
}

}
}
