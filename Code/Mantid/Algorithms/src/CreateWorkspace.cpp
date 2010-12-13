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

/// Default (empty) constructor
CreateWorkspace::CreateWorkspace() : API::Algorithm()
{}

/// Default (empty) destructor
CreateWorkspace::~CreateWorkspace()
{}

/// Init function
void CreateWorkspace::init()
{
  declareProperty(new Kernel::ArrayProperty<double>("DataX"),
    "X-axis data values for workspace.");
  declareProperty(new Kernel::ArrayProperty<double>("DataY"),
    "Y-axis data values for workspace.");
  declareProperty(new Kernel::ArrayProperty<double>("DataE"),
    "Error values for workspace.");
  declareProperty(new Kernel::PropertyWithValue<int>("NSpec", 1),
    "Number of spectra to divide data into.");
  std::vector<std::string> unitOptions = Mantid::Kernel::UnitFactory::Instance().getKeys();
  unitOptions.push_back("");
  declareProperty("UnitX","",new Mantid::Kernel::ListValidator(unitOptions),
      "The unit to assign to the XAxis");
  unitOptions.push_back("Text");
  declareProperty("UnitY","",new Mantid::Kernel::ListValidator(unitOptions),
      "The unit to assign to the YAxis");
  declareProperty(new Kernel::ArrayProperty<std::string>("YAxisValues"),
    "Values for labels of Y Axis."); // This property taken as strings to allow for Text Axis.
  declareProperty(new Mantid::API::WorkspaceProperty<>("OutputWorkspace", "", Kernel::Direction::Output),
    "Name to be given to the created workspace.");
}

/// Exec function
void CreateWorkspace::exec()
{
  const std::vector<double> dataX = getProperty("DataX");
  const std::vector<double> dataY = getProperty("DataY");
  const std::vector<double> dataE = getProperty("DataE");
  const int nSpec = getProperty("NSpec");
  const std::string xUnit = getProperty("UnitX");
  const std::string yUnit = getProperty("UnitY");
  const std::vector<std::string> yAxis = getProperty("YAxisValues");

  if ( ( yUnit != "" ) && ( static_cast<int>(yAxis.size()) != nSpec ) )
  {
    throw std::invalid_argument("Number of y-axis labels must match number of histograms.");
  }

  // Verify length of vectors makes sense with NSpec
  if ( dataX.size() % nSpec != 0 )
  {
    throw std::invalid_argument("Length of DataX must be divisable by NSpec");
  }
  if ( ( dataY.size() % nSpec ) != 0 )
  {
    throw std::invalid_argument("Length of DataY must be divisable by NSpec");
  }
  int ySize = dataY.size() / nSpec;
  int xSize = dataX.size() / nSpec;
  if ( ySize != ( static_cast<int>(dataE.size()) / nSpec ) )
  {
    throw std::runtime_error("DataY and DataE must have the same dimensions");
  }
  if ( xSize < ySize || xSize > ySize + 1 )
  {
    throw std::runtime_error("DataX width must be as DataY or +1");
  }
  Mantid::API::MatrixWorkspace_sptr outputWS = Mantid::API::WorkspaceFactory::Instance().create("Workspace2D", nSpec, xSize, ySize);

  for ( int i = 0; i < nSpec; i++ )
  {
    std::vector<double> specX, specY, specE;
    for ( int j = 0; j < ySize; j++ )
    {
      specY.push_back(dataY[(i*ySize)+j]);
      specE.push_back(dataE[(i*ySize)+j]);
      specX.push_back(dataX[(i*xSize)+j]);
    }
    if ( ySize != xSize )
    {
      specX.push_back(dataX[(i*xSize)+(xSize-1)]);
    }

    outputWS->dataX(i) = specX;
    outputWS->dataY(i) = specY;
    outputWS->dataE(i) = specE;
  }

  if ( xUnit != "" )
  {
    outputWS->getAxis(0)->unit() = Mantid::Kernel::UnitFactory::Instance().create(xUnit);
  }
  if ( yUnit != "" )
  {
    if ( yUnit == "Text" )
    {
      Mantid::API::TextAxis* const newAxis = new Mantid::API::TextAxis(yAxis.size());
      outputWS->replaceAxis(1, newAxis);
      for ( size_t i = 0; i < yAxis.size(); i++ )
      {
        newAxis->setLabel(i, yAxis[i]);
      }
    }
    else
    {
      Mantid::API::NumericAxis* const newAxis = new Mantid::API::NumericAxis(yAxis.size());
      newAxis->unit() = Mantid::Kernel::UnitFactory::Instance().create(yUnit);
      outputWS->replaceAxis(1, newAxis);
      for ( size_t i = 0; i < yAxis.size(); i++ )
      {
        try
        {
          newAxis->setValue(i, boost::lexical_cast<double, std::string>(yAxis[i]) );
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
  setProperty("OutputWorkspace", outputWS);
}

} // Algorithms
} // Mantid
