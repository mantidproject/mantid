/*WIKI* 

Creates a 2D workspace that contains a single value and an optional error value. This is useful if, for example, there is a need to multiply (or divide etc) a workspace by a single value.

=== Usage ===
 LoadRaw("C:/Data/testfile.raw","rawspace")
 CreateSingleValuedWorkspace("scalar", "3")
 Multiply("rawspace", "scalar", "rawspace")


*WIKI*/
//------------------------
//Includes
//------------------------
#include "MantidAlgorithms/CreateSingleValuedWorkspace.h"
#include "MantidKernel/BoundedValidator.h"
#include "MantidDataObjects/WorkspaceSingleValue.h"
#include "MantidAPI/WorkspaceFactory.h"
#include "MantidKernel/UnitFactory.h"

using namespace Mantid::Algorithms;

// Register the algorithm into the AlgorithmFactory
DECLARE_ALGORITHM(CreateSingleValuedWorkspace)

/// Sets documentation strings for this algorithm
void CreateSingleValuedWorkspace::initDocs()
{
  this->setWikiSummary("Creates a 2D workspace with a single value contained in it. ");
  this->setOptionalMessage("Creates a 2D workspace with a single value contained in it.");
}


void CreateSingleValuedWorkspace::init()
{
  using namespace Mantid::Kernel;
  using namespace Mantid::API;
  declareProperty(new WorkspaceProperty<>("OutputWorkspace","",Direction::Output),
    "Name to use for the output workspace" );
  declareProperty("DataValue", 0.0, "The value to place in the workspace");
  auto mustBePositive = boost::make_shared<BoundedValidator<double> >();
  mustBePositive->setLower(0.0);
  declareProperty("ErrorValue", 0.0, mustBePositive,
    "The error value to place in the workspace (default 0.0)" );
}

void CreateSingleValuedWorkspace::exec()
{
  //Get the property values
  double dataValue = getProperty("DataValue");
  double errorValue = getProperty("ErrorValue");

  using namespace Mantid::DataObjects;
  using namespace Mantid::API;
  using namespace Mantid::Kernel;
  // Create the workspace
  MatrixWorkspace_sptr singleValued = WorkspaceFactory::Instance().create("WorkspaceSingleValue", 1, 1, 1);
  
  singleValued->dataX(0)[0] = 0.0;
  singleValued->dataY(0)[0] = dataValue;
  singleValued->dataE(0)[0] = errorValue;
  
  setProperty("OutputWorkspace", singleValued);
  //Done :)
}

