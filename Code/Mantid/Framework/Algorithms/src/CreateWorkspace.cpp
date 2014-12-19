#include "MantidAlgorithms/CreateWorkspace.h"

#include "MantidKernel/ArrayProperty.h"
#include "MantidKernel/PropertyWithValue.h"
#include "MantidKernel/UnitFactory.h"
#include "MantidAPI/BinEdgeAxis.h"
#include "MantidAPI/NumericAxis.h"
#include "MantidAPI/SpectraAxis.h"
#include "MantidAPI/TextAxis.h"
#include "MantidKernel/ListValidator.h"
#include "MantidKernel/MandatoryValidator.h"

namespace Mantid {
namespace Algorithms {

using namespace Kernel;
using namespace API;

DECLARE_ALGORITHM(CreateWorkspace)

/// Default (empty) constructor
CreateWorkspace::CreateWorkspace() : Algorithm() {}

/// Default (empty) destructor
CreateWorkspace::~CreateWorkspace() {}

/// Init function
void CreateWorkspace::init() {

  std::vector<std::string> unitOptions = UnitFactory::Instance().getKeys();
  unitOptions.push_back("SpectraNumber");
  unitOptions.push_back("Text");

  declareProperty(
      new WorkspaceProperty<>("OutputWorkspace", "", Direction::Output),
      "Name to be given to the created workspace.");

  auto required = boost::make_shared<MandatoryValidator<std::vector<double>>>();
  declareProperty(new ArrayProperty<double>("DataX", required),
                  "X-axis data values for workspace.");
  declareProperty(new ArrayProperty<double>("DataY", required),
                  "Y-axis data values for workspace (measures).");
  declareProperty(new ArrayProperty<double>("DataE"),
                  "Error values for workspace. Optional.");
  declareProperty(new PropertyWithValue<int>("NSpec", 1),
                  "Number of spectra to divide data into.");
  declareProperty("UnitX", "", "The unit to assign to the XAxis");

  declareProperty("VerticalAxisUnit", "SpectraNumber",
                  boost::make_shared<StringListValidator>(unitOptions),
                  "The unit to assign to the second Axis (leave blank for "
                  "default Spectra number)");
  declareProperty(new ArrayProperty<std::string>("VerticalAxisValues"),
                  "Values for the VerticalAxis.");

  declareProperty(
      new PropertyWithValue<bool>("Distribution", false),
      "Whether OutputWorkspace should be marked as a distribution.");
  declareProperty("YUnitLabel", "", "Label for Y Axis");

  declareProperty("WorkspaceTitle", "", "Title for Workspace");

  declareProperty(new WorkspaceProperty<>("ParentWorkspace", "",
                                          Direction::Input,
                                          PropertyMode::Optional),
                  "Name of a parent workspace.");
}

/// Input validation
std::map<std::string, std::string> CreateWorkspace::validateInputs() {
  std::map<std::string, std::string> issues;

  const std::string vUnit = getProperty("VerticalAxisUnit");
  const std::vector<std::string> vAxis = getProperty("VerticalAxisValues");

  if (vUnit == "SpectraNumber" && vAxis.size() > 0)
    issues["VerticalAxisValues"] =
        "Axis values cannot be provided when using a spectra axis";

  return issues;
}

/// Exec function
void CreateWorkspace::exec() {
  // Contortions to get at the vector in the property without copying it
  const Property *const dataXprop = getProperty("DataX");
  const Property *const dataYprop = getProperty("DataY");
  const Property *const dataEprop = getProperty("DataE");
  const std::vector<double> &dataX =
      *dynamic_cast<const ArrayProperty<double> *>(dataXprop);
  const std::vector<double> &dataY =
      *dynamic_cast<const ArrayProperty<double> *>(dataYprop);
  const std::vector<double> &dataE =
      *dynamic_cast<const ArrayProperty<double> *>(dataEprop);

  const int nSpec = getProperty("NSpec");
  const std::string xUnit = getProperty("UnitX");
  const std::string vUnit = getProperty("VerticalAxisUnit");
  const std::vector<std::string> vAxis = getProperty("VerticalAxisValues");

  if ((vUnit != "SpectraNumber") && (static_cast<int>(vAxis.size()) != nSpec)) {
    throw std::invalid_argument(
        "Number of y-axis labels must match number of histograms.");
  }

  // Verify length of vectors makes sense with NSpec
  if ((dataY.size() % nSpec) != 0) {
    throw std::invalid_argument("Length of DataY must be divisible by NSpec");
  }
  const std::size_t ySize = dataY.size() / nSpec;

  // Check whether the X values provided are to be re-used for (are common to)
  // every spectrum
  const bool commonX(dataX.size() == ySize || dataX.size() == ySize + 1);

  std::size_t xSize;
  MantidVecPtr XValues;
  if (commonX) {
    xSize = dataX.size();
    XValues.access() = dataX;
  } else {
    if (dataX.size() % nSpec != 0) {
      throw std::invalid_argument("Length of DataX must be divisible by NSpec");
    }

    xSize = static_cast<int>(dataX.size()) / nSpec;
    if (xSize < ySize || xSize > ySize + 1) {
      throw std::runtime_error("DataX width must be as DataY or +1");
    }
  }

  const bool dataE_provided = !dataE.empty();
  if (dataE_provided && dataY.size() != dataE.size()) {
    throw std::runtime_error(
        "DataE (if provided) must be the same size as DataY");
  }

  // Create the OutputWorkspace
  MatrixWorkspace_const_sptr parentWS = getProperty("ParentWorkspace");
  MatrixWorkspace_sptr outputWS;
  if (parentWS) {
    // if parent is defined use it to initialise the workspace
    outputWS =
        WorkspaceFactory::Instance().create(parentWS, nSpec, xSize, ySize);
  } else {
    // otherwise create a blank workspace
    outputWS =
        WorkspaceFactory::Instance().create("Workspace2D", nSpec, xSize, ySize);
  }

  Progress progress(this, 0, 1, nSpec);

  PARALLEL_FOR1(outputWS)
  for (int i = 0; i < nSpec; i++) {
    PARALLEL_START_INTERUPT_REGION

    const std::vector<double>::difference_type xStart = i * xSize;
    const std::vector<double>::difference_type xEnd = xStart + xSize;
    const std::vector<double>::difference_type yStart = i * ySize;
    const std::vector<double>::difference_type yEnd = yStart + ySize;

    // Just set the pointer if common X bins. Otherwise, copy in the right chunk
    // (as we do for Y).
    if (commonX) {
      outputWS->setX(i, XValues);
    } else {
      outputWS->dataX(i).assign(dataX.begin() + xStart, dataX.begin() + xEnd);
    }

    outputWS->dataY(i).assign(dataY.begin() + yStart, dataY.begin() + yEnd);

    if (dataE_provided)
      outputWS->dataE(i).assign(dataE.begin() + yStart, dataE.begin() + yEnd);

    progress.report();
    PARALLEL_END_INTERUPT_REGION
  }
  PARALLEL_CHECK_INTERUPT_REGION

  // Set the Unit of the X Axis
  try {
    outputWS->getAxis(0)->unit() = UnitFactory::Instance().create(xUnit);
  } catch (Exception::NotFoundError &) {
    outputWS->getAxis(0)->unit() = UnitFactory::Instance().create("Label");
    Unit_sptr unit = outputWS->getAxis(0)->unit();
    boost::shared_ptr<Units::Label> label =
        boost::dynamic_pointer_cast<Units::Label>(unit);
    label->setLabel(xUnit, xUnit);
  }

  // Populate the VerticalAxis. A spectra one is there by default with a 1->N
  // mapping
  if (vUnit != "SpectraNumber") {
    if (vUnit == "Text") {
      TextAxis *const newAxis = new TextAxis(vAxis.size());
      outputWS->replaceAxis(1, newAxis);
      for (size_t i = 0; i < vAxis.size(); i++) {
        newAxis->setLabel(i, vAxis[i]);
      }
    } else {
      const size_t vAxisLength = vAxis.size();
      NumericAxis *newAxis(NULL);
      if (vAxisLength == static_cast<size_t>(nSpec))
        newAxis = new NumericAxis(vAxisLength); // treat as points
      else if (vAxisLength == static_cast<size_t>(nSpec + 1))
        newAxis = new BinEdgeAxis(vAxisLength); // treat as bin edges
      else
        throw std::range_error("Invalid vertical axis length. It must be the "
                               "same length as NSpec or 1 longer.");

      newAxis->unit() = UnitFactory::Instance().create(vUnit);
      outputWS->replaceAxis(1, newAxis);
      for (size_t i = 0; i < vAxis.size(); i++) {
        try {
          newAxis->setValue(i,
                            boost::lexical_cast<double, std::string>(vAxis[i]));
        } catch (boost::bad_lexical_cast &) {
          throw std::invalid_argument("CreateWorkspace - YAxisValues property "
                                      "could not be converted to a double.");
        }
      }
    }
  }

  // Set distribution flag
  outputWS->isDistribution(getProperty("Distribution"));

  // Set Y Unit label
  if (!parentWS || !getPropertyValue("YUnitLabel").empty()) {
    outputWS->setYUnitLabel(getProperty("YUnitLabel"));
  }

  // Set Workspace Title
  if (!parentWS || !getPropertyValue("WorkspaceTitle").empty()) {
    outputWS->setTitle(getProperty("WorkspaceTitle"));
  }

  // Set OutputWorkspace property
  setProperty("OutputWorkspace", outputWS);
}

} // Algorithms
} // Mantid
