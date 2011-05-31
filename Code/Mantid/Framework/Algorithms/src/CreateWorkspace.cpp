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

DECLARE_ALGORITHM(CreateWorkspace)

/// Sets documentation strings for this algorithm
void CreateWorkspace::initDocs()
{
  this->setWikiSummary("This algorithm constructs a [[MatrixWorkspace]] when passed a vector for each of the X, Y, and E data values. The unit for the X Axis can optionally be specified as any of the units in the Kernel's UnitFactory.  Multiple spectra may be created by supplying the NSpec Property (integer, default 1). When this is provided the vectors are split into equal-sized spectra (all X, Y, E values must still be in a single vector for input). ");
  this->setOptionalMessage("This algorithm constructs a MatrixWorkspace when passed a vector for each of the X, Y, and E data values. The unit for the X Axis can optionally be specified as any of the units in the Kernel's UnitFactory.  Multiple spectra may be created by supplying the NSpec Property (integer, default 1). When this is provided the vectors are split into equal-sized spectra (all X, Y, E values must still be in a single vector for input).");
}


/// Default (empty) constructor
CreateWorkspace::CreateWorkspace() : API::Algorithm()
{}

/// Default (empty) destructor
CreateWorkspace::~CreateWorkspace()
{}

/// Init function
void CreateWorkspace::init()
{

  std::vector<std::string> unitOptions = Mantid::Kernel::UnitFactory::Instance().getKeys();
  unitOptions.push_back("SpectraNumber");
  unitOptions.push_back("Text");

  
  declareProperty(new Mantid::API::WorkspaceProperty<>("OutputWorkspace", "", Kernel::Direction::Output),
    "Name to be given to the created workspace.");
  declareProperty(new Kernel::ArrayProperty<double>("DataX"),
    "X-axis data values for workspace.");
  declareProperty(new Kernel::ArrayProperty<double>("DataY"),
    "Y-axis data values for workspace (measures).");
  declareProperty(new Kernel::ArrayProperty<double>("DataE"),
    "Error values for workspace.");
  declareProperty(new Kernel::PropertyWithValue<int>("NSpec", 1),
    "Number of spectra to divide data into.");
  declareProperty("UnitX","", "The unit to assign to the XAxis");
  
  declareProperty("VerticalAxisUnit","SpectraNumber",new Mantid::Kernel::ListValidator(unitOptions),
      "The unit to assign to the second Axis (leave blank for default Spectra number)");
  declareProperty(new Kernel::ArrayProperty<std::string>("VerticalAxisValues"),
    "Values for the VerticalAxis.");

  declareProperty(new Kernel::PropertyWithValue<bool>("Distribution", false),
    "Whether OutputWorkspace should be marked as a distribution.");
  declareProperty("YUnitLabel", "", "Label for Y Axis");

  declareProperty("WorkspaceTitle", "", "Title for Workspace");
}

/// Exec function
void CreateWorkspace::exec()
{
  const std::vector<double> dataX = getProperty("DataX");
  const std::vector<double> dataY = getProperty("DataY");
  const std::vector<double> dataE = getProperty("DataE");
  const int nSpec = getProperty("NSpec");
  const std::string xUnit = getProperty("UnitX");
  const std::string vUnit = getProperty("VerticalAxisUnit");
  const std::vector<std::string> vAxis = getProperty("VerticalAxisValues");

  if ( ( vUnit != "SpectraNumber" ) && ( static_cast<int>(vAxis.size()) != nSpec ) )
  {
    throw std::invalid_argument("Number of y-axis labels must match number of histograms.");
  }

  // Verify length of vectors makes sense with NSpec
  if ( dataX.size() % nSpec != 0 )
  {
    throw std::invalid_argument("Length of DataX must be divisible by NSpec");
  }
  if ( ( dataY.size() % nSpec ) != 0 )
  {
    throw std::invalid_argument("Length of DataY must be divisible by NSpec");
  }
  if ( dataY.size() != dataE.size() )
  {
    throw std::runtime_error("DataY and DataE must have the same dimensions");
  }

  int ySize = static_cast<int>(dataY.size()) / nSpec;
  int xSize = static_cast<int>(dataX.size()) / nSpec;

  if ( xSize < ySize || xSize > ySize + 1 )
  {
    throw std::runtime_error("DataX width must be as DataY or +1");
  }
  
  // Create the OutputWorkspace
  Mantid::API::MatrixWorkspace_sptr outputWS = Mantid::API::WorkspaceFactory::Instance().create("Workspace2D", nSpec, xSize, ySize);

  for ( int i = 0; i < nSpec; i++ )
  {
//    std::vector<double> specX, specY, specE;
//    for ( int j = 0; j < ySize; j++ )
//    {
//      specY.push_back(dataY[(i*ySize)+j]);
//      specE.push_back(dataE[(i*ySize)+j]);
//      specX.push_back(dataX[(i*xSize)+j]);
//    }
//    if ( ySize != xSize )
//    {
//      specX.push_back(dataX[(i*xSize)+(xSize-1)]);
//    }
//
//    outputWS->dataX(i) = specX;
//    outputWS->dataY(i) = specY;
//    outputWS->dataE(i) = specE;
    const std::vector<double>::difference_type xStart = i*xSize;
    const std::vector<double>::difference_type xEnd = xStart + xSize;
    const std::vector<double>::difference_type yStart = i*ySize;
    const std::vector<double>::difference_type yEnd = yStart + ySize;
    outputWS->dataX(i).assign(dataX.begin()+xStart,dataX.begin()+xEnd);
    outputWS->dataY(i).assign(dataY.begin()+yStart,dataY.begin()+yEnd);
    outputWS->dataE(i).assign(dataE.begin()+yStart,dataE.begin()+yEnd);
  }

  // Set the Unit of the X Axis
  try
  {
    outputWS->getAxis(0)->unit() = Mantid::Kernel::UnitFactory::Instance().create(xUnit);
  }
  catch ( Mantid::Kernel::Exception::NotFoundError & )
  {
    outputWS->getAxis(0)->unit() = Mantid::Kernel::UnitFactory::Instance().create("Label");
    Mantid::Kernel::Unit_sptr unit = outputWS->getAxis(0)->unit();
    boost::shared_ptr<Mantid::Kernel::Units::Label> label = boost::dynamic_pointer_cast<Mantid::Kernel::Units::Label>(unit);
    label->setLabel(xUnit, xUnit);
  }

  // Populate the VerticalAxis
  if ( vUnit != "SpectraNumber" )
  {
    if ( vUnit == "Text" )
    {
      Mantid::API::TextAxis* const newAxis = new Mantid::API::TextAxis(vAxis.size());
      outputWS->replaceAxis(1, newAxis);
      for ( size_t i = 0; i < vAxis.size(); i++ )
      {
        newAxis->setLabel(i, vAxis[i]);
      }
    }
    else
    {
      Mantid::API::NumericAxis* const newAxis = new Mantid::API::NumericAxis(vAxis.size());
      newAxis->unit() = Mantid::Kernel::UnitFactory::Instance().create(vUnit);
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
    dynamic_cast<Mantid::API::SpectraAxis*>(outputWS->getAxis(1))->populateSimple(nSpec);
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
