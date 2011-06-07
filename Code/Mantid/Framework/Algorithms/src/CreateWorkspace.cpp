#include "MantidAlgorithms/CreateWorkspace.h"

#include "MantidKernel/ArrayProperty.h"
#include "MantidKernel/PropertyWithValue.h"
#include "MantidKernel/UnitFactory.h"
#include "MantidAPI/SpectraDetectorMap.h"
#include "MantidAPI/SpectraAxis.h"
#include "MantidAPI/NumericAxis.h"
#include "MantidAPI/TextAxis.h"

namespace Mantid
{
namespace Algorithms
{

using namespace Kernel;
using namespace API;

DECLARE_ALGORITHM(CreateWorkspace)

/// Sets documentation strings for this algorithm
void CreateWorkspace::initDocs()
{
  this->setWikiSummary("This algorithm constructs a [[MatrixWorkspace]] when passed a vector for each of the X, Y, and E data values. The unit for the X Axis can optionally be specified as any of the units in the Kernel's UnitFactory.  Multiple spectra may be created by supplying the NSpec Property (integer, default 1). When this is provided the vectors are split into equal-sized spectra (all X, Y, E values must still be in a single vector for input). ");
  this->setOptionalMessage("This algorithm constructs a MatrixWorkspace when passed a vector for each of the X, Y, and E data values. The unit for the X Axis can optionally be specified as any of the units in the Kernel's UnitFactory.  Multiple spectra may be created by supplying the NSpec Property (integer, default 1). When this is provided the vectors are split into equal-sized spectra (all X, Y, E values must still be in a single vector for input).");
}


/// Default (empty) constructor
CreateWorkspace::CreateWorkspace() : Algorithm()
{}

/// Default (empty) destructor
CreateWorkspace::~CreateWorkspace()
{}

/// Init function
void CreateWorkspace::init()
{

  std::vector<std::string> unitOptions = UnitFactory::Instance().getKeys();
  unitOptions.push_back("SpectraNumber");
  unitOptions.push_back("Text");

  
  declareProperty(new WorkspaceProperty<>("OutputWorkspace", "", Direction::Output),
    "Name to be given to the created workspace.");
  declareProperty(new ArrayProperty<double>("DataX", new MandatoryValidator<std::vector<double> >),
    "X-axis data values for workspace.");
  declareProperty(new ArrayProperty<double>("DataY", new MandatoryValidator<std::vector<double> >),
    "Y-axis data values for workspace (measures).");
  declareProperty(new ArrayProperty<double>("DataE"),
    "Error values for workspace. Optional.");
  declareProperty(new PropertyWithValue<int>("NSpec", 1),
    "Number of spectra to divide data into.");
  declareProperty("UnitX","", "The unit to assign to the XAxis");
  
  declareProperty("VerticalAxisUnit","SpectraNumber",new ListValidator(unitOptions),
      "The unit to assign to the second Axis (leave blank for default Spectra number)");
  declareProperty(new ArrayProperty<std::string>("VerticalAxisValues"),
    "Values for the VerticalAxis.");

  declareProperty(new PropertyWithValue<bool>("Distribution", false),
    "Whether OutputWorkspace should be marked as a distribution.");
  declareProperty("YUnitLabel", "", "Label for Y Axis");

  declareProperty("WorkspaceTitle", "", "Title for Workspace");
}

/// Exec function
void CreateWorkspace::exec()
{
  // Contortions to get at the vector in the property without copying it
  const Property * const dataXprop = getProperty("DataX");
  const Property * const dataYprop = getProperty("DataY");
  const Property * const dataEprop = getProperty("DataE");
  const std::vector<double>& dataX = *dynamic_cast<const ArrayProperty<double>*>(dataXprop);
  const std::vector<double>& dataY = *dynamic_cast<const ArrayProperty<double>*>(dataYprop);
  const std::vector<double>& dataE = *dynamic_cast<const ArrayProperty<double>*>(dataEprop);

  const int nSpec = getProperty("NSpec");
  const std::string xUnit = getProperty("UnitX");
  const std::string vUnit = getProperty("VerticalAxisUnit");
  const std::vector<std::string> vAxis = getProperty("VerticalAxisValues");

  if ( ( vUnit != "SpectraNumber" ) && ( static_cast<int>(vAxis.size()) != nSpec ) )
  {
    throw std::invalid_argument("Number of y-axis labels must match number of histograms.");
  }

  // Verify length of vectors makes sense with NSpec
  if ( ( dataY.size() % nSpec ) != 0 )
  {
    throw std::invalid_argument("Length of DataY must be divisible by NSpec");
  }
  const std::size_t ySize = dataY.size() / nSpec;

  // Check whether the X values provided are to be re-used for (are common to) every spectrum
  const bool commonX( dataX.size() == ySize || dataX.size() == ySize+1 );

  std::size_t xSize;
  MantidVecPtr XValues;
  if ( commonX )
  {
    xSize = dataX.size();
    XValues.access() = dataX;
  }
  else
  {
    if ( dataX.size() % nSpec != 0 )
    {
      throw std::invalid_argument("Length of DataX must be divisible by NSpec");
    }

    xSize = static_cast<int>(dataX.size()) / nSpec;
    if ( xSize < ySize || xSize > ySize + 1 )
    {
      throw std::runtime_error("DataX width must be as DataY or +1");
    }

  }

  const bool dataE_provided(dataE.size());
  if ( dataE_provided && dataY.size() != dataE.size() )
  {
    throw std::runtime_error("DataE (if provided) must be the same size as DataY");
  }

  // Create the OutputWorkspace
  MatrixWorkspace_sptr outputWS = WorkspaceFactory::Instance().create("Workspace2D", nSpec, xSize, ySize);

  Progress progress(this,0,1,nSpec);

  PARALLEL_FOR1(outputWS)
  for ( int i = 0; i < nSpec; i++ )
  {
    PARALLEL_START_INTERUPT_REGION

    const std::vector<double>::difference_type xStart = i*xSize;
    const std::vector<double>::difference_type xEnd = xStart + xSize;
    const std::vector<double>::difference_type yStart = i*ySize;
    const std::vector<double>::difference_type yEnd = yStart + ySize;

    // Just set the pointer if common X bins. Otherwise, copy in the right chunk (as we do for Y).
    if ( commonX )
    {
      outputWS->setX(i,XValues);
    }
    else
    {
      outputWS->dataX(i).assign(dataX.begin()+xStart,dataX.begin()+xEnd);
    }

    outputWS->dataY(i).assign(dataY.begin()+yStart,dataY.begin()+yEnd);

    if ( dataE_provided) outputWS->dataE(i).assign(dataE.begin()+yStart,dataE.begin()+yEnd);

    progress.report();
    PARALLEL_END_INTERUPT_REGION
  }
  PARALLEL_CHECK_INTERUPT_REGION

  // Set the Unit of the X Axis
  try
  {
    outputWS->getAxis(0)->unit() = UnitFactory::Instance().create(xUnit);
  }
  catch ( Exception::NotFoundError & )
  {
    outputWS->getAxis(0)->unit() = UnitFactory::Instance().create("Label");
    Unit_sptr unit = outputWS->getAxis(0)->unit();
    boost::shared_ptr<Units::Label> label = boost::dynamic_pointer_cast<Units::Label>(unit);
    label->setLabel(xUnit, xUnit);
  }

  // Populate the VerticalAxis
  if ( vUnit != "SpectraNumber" )
  {
    if ( vUnit == "Text" )
    {
      TextAxis* const newAxis = new TextAxis(vAxis.size());
      outputWS->replaceAxis(1, newAxis);
      for ( size_t i = 0; i < vAxis.size(); i++ )
      {
        newAxis->setLabel(i, vAxis[i]);
      }
    }
    else
    {
      NumericAxis* const newAxis = new NumericAxis(vAxis.size());
      newAxis->unit() = UnitFactory::Instance().create(vUnit);
      outputWS->replaceAxis(1, newAxis);
      for ( size_t i = 0; i < vAxis.size(); i++ )
      {
        try
        {
          newAxis->setValue(i, boost::lexical_cast<double, std::string>(vAxis[i]) );
        }
        catch ( boost::bad_lexical_cast & )
        {
          throw std::invalid_argument("CreateWorkspace - YAxisValues property could not be converted to a double.");
        }
      }
    }
  }
  else
  {
    dynamic_cast<SpectraAxis*>(outputWS->getAxis(1))->populateSimple(nSpec);
  }

  // Set distribution flag
  outputWS->isDistribution(getProperty("Distribution"));

  // Set Y Unit label
  outputWS->setYUnitLabel(getProperty("YUnitLabel"));

  // Set Workspace Title
  outputWS->setTitle(getProperty("WorkspaceTitle"));

  // Set OutputWorkspace property
  setProperty("OutputWorkspace", outputWS);
}

} // Algorithms
} // Mantid
