#include "MantidDataHandling/LoadEventPreNeXus.h"

// Register the algorithm into the AlgorithmFactory
DECLARE_ALGORITHM(LoadEventPreNeXus)

using namespace Mantid::Kernel;
using namespace Mantid::API;
using namespace Mantid::Geometry;

// A reference to the logger is provided by the base class, it is called g_log.
// It is used to print out information, warning and error messages

void LoadEventPreNeXus::init()
{
  // Put your initialisation code (e.g. declaring properties) here...

  // Virtually all algorithms will want an input and an output workspace as properties.
  // Here are the lines for this, so just uncomment them:
  //   declareProperty(new WorkspaceProperty<>("InputWorkspace","",Direction::Input));
  //   declareProperty(new WorkspaceProperty<>("OutputWorkspace","",Direction::Output));

}

void LoadEventPreNeXus::exec()
{
  // Put the algorithm execution code here... 

  // The first thing to do will almost certainly be to retrieve the input workspace.
  // Here's the line for that - just uncomment it:
  //   MatrixWorkspace_sptr inputWorkspace = getProperty("InputWorkspace");

}

