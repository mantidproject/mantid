#include <string>
#include <vector>
#include "MantidDataHandling/LoadEventPreNeXus.h"
#include "MantidKernel/ArrayProperty.h"
#include "MantidKernel/FileProperty.h"
#include "MantidKernel/System.h"

// Register the algorithm into the AlgorithmFactory
DECLARE_ALGORITHM(LoadEventPreNeXus)

using namespace Mantid::Kernel;
using namespace Mantid::API;
using namespace Mantid::Geometry;
using std::string;
using std::vector;

// constants for locating the parameters to use in execution
static const string EVENT_PARAM("Event filename");
static const string PULSEID_PARAM("Pulseid filename");
static const string MAP_PARAM("Mapping filename");
static const string PID_MIN_PARAM("Spectrum Min");
static const string PID_MAX_PARAM("Spectrum Max");
static const string PID_PARAM("Spectrum List");
static const string PERIOD_PARAM("Period List");
static const string OUT_PARAM("OutputWorkspace");

static Property * createLoadProperty(const string name, const string ext)
{
  vector<string> exts;
  exts.push_back(ext);
  return new FileProperty(name, "", FileProperty::Load, exts);
}

// A reference to the logger is provided by the base class, it is called g_log.
// It is used to print out information, warning and error messages

void LoadEventPreNeXus::init()
{
  // Put your initialisation code (e.g. declaring properties) here...

  // Virtually all algorithms will want an input and an output workspace as properties.
  // Here are the lines for this, so just uncomment them:
  //   declareProperty(new WorkspaceProperty<>("InputWorkspace","",Direction::Input));

  // which files to use
  this->declareProperty(createLoadProperty(EVENT_PARAM, "event.dat"),
                        "A preNeXus neutron event file");
  this->declareProperty(createLoadProperty(PULSEID_PARAM, "pulseid.dat"),
                        "A preNeXus pulseid file. Used only if specified.");
  this->declareProperty(createLoadProperty(MAP_PARAM, "dat"),
                        "TS mapping file converting detector id to pixel id. Used only if specified.");

  // which pixels to load
  BoundedValidator<int> *mustBePositive = new BoundedValidator<int> ();
  mustBePositive->setLower(1);
  this->declareProperty(PID_MIN_PARAM, 1, mustBePositive,
      "The index number of the first spectrum to read. Only used if the spectrum max is set.");
  this->declareProperty(PID_MAX_PARAM, Mantid::EMPTY_INT(), mustBePositive->clone(),
      "The index number of the last spectrum to read. Used only if specifed");
  this->declareProperty(new ArrayProperty<int>(PID_PARAM),
      "A comma separated list of individual spectra to read. Only used if set.");

  // which states to load
  this->declareProperty(new ArrayProperty<int>(PERIOD_PARAM),
      "A comma separated list of periods to read. Only used if set.");

  // the output workspace name
  this->declareProperty(new WorkspaceProperty<>(OUT_PARAM,"",Direction::Output));

}

void LoadEventPreNeXus::exec()
{
  // Put the algorithm execution code here... 

  // The first thing to do will almost certainly be to retrieve the input workspace.
  // Here's the line for that - just uncomment it:
  //   MatrixWorkspace_sptr inputWorkspace = getProperty("InputWorkspace");

}

