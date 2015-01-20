#include "MantidMDEvents/QueryMDWorkspace.h"
#include "MantidKernel/System.h"
#include "MantidAPI/IMDEventWorkspace.h"
#include "MantidAPI/IMDIterator.h"
#include "MantidAPI/ITableWorkspace.h"
#include "MantidAPI/WorkspaceFactory.h"
#include "MantidAPI/TableRow.h"
#include "MantidKernel/EnabledWhenProperty.h"
#include "MantidKernel/BoundedValidator.h"
#include "MantidKernel/ListValidator.h"
#include "../inc/MantidMDEvents/MDEventWorkspace.h"
#include "../inc/MantidMDEvents/MDEventFactory.h"

using namespace Mantid::Kernel;
using namespace Mantid::API;

namespace Mantid {
namespace MDEvents {

// Register the class into the algorithm factory
DECLARE_ALGORITHM(QueryMDWorkspace)

/**
Non-member defining the no normalisation option.
@return no normalisation option.
*/
std::string noNormalisationOption() { return "none"; }

/**
Non-member defining the volume normalisation option.
@return volume normalisation option.
*/
std::string volumeNormalisationOption() { return "volume"; }

/**
Non-member defining the number of events normalisation option.
@return number of events normalisation.
*/
std::string numberOfEventsNormalisationOption() { return "number of events"; }

/**
Non-member method to interpret an normalisation option.
@param strNormalisation: string normalisation property value.
@return MDNormalisation option.
*/
Mantid::API::MDNormalization
whichNormalisation(const std::string &strNormalisation) {
  Mantid::API::MDNormalization requestedNormalisation =
      Mantid::API::NoNormalization;
  if (strNormalisation == noNormalisationOption()) {
    requestedNormalisation = Mantid::API::NoNormalization;
    ;
  } else if (strNormalisation == volumeNormalisationOption()) {
    requestedNormalisation = Mantid::API::VolumeNormalization;
  } else {
    requestedNormalisation = Mantid::API::NumEventsNormalization;
  }
  return requestedNormalisation;
}

//----------------------------------------------------------------------------------------------
/** Constructor
 */
QueryMDWorkspace::QueryMDWorkspace() {}

//----------------------------------------------------------------------------------------------
/** Destructor
 */
QueryMDWorkspace::~QueryMDWorkspace() {}

/// Initialise the properties
void QueryMDWorkspace::init() {
  declareProperty(new WorkspaceProperty<IMDWorkspace>("InputWorkspace", "",
                                                      Direction::Input),
                  "An input MDWorkspace.");

  declareProperty(
      new WorkspaceProperty<ITableWorkspace>("OutputWorkspace", "",
                                             Direction::Output),
      "The output Tableworkspace "
      "with columns containing key summary information about the MDWorkspace.");

  declareProperty("LimitRows", true,
                  "Limit the report output to a maximum number of rows");

  declareProperty(
      new PropertyWithValue<int>("MaximumRows", 100000,
                                 boost::make_shared<BoundedValidator<int>>(),
                                 Direction::Input),
      "The number of neighbours to utilise. Defaults to 100000.");
  setPropertySettings("MaximumRows",
                      new EnabledWhenProperty("LimitRows", IS_DEFAULT));

  std::vector<std::string> propOptions;
  propOptions.push_back(noNormalisationOption());
  propOptions.push_back(volumeNormalisationOption());
  propOptions.push_back(numberOfEventsNormalisationOption());

  declareProperty("Normalisation", "none",
                  boost::make_shared<StringListValidator>(propOptions),
                  "What normalisation do you wish to apply"
                  "  none: No normalisation.\n"
                  "  volume: Normalise by the volume.\n"
                  "  number of events: Normalise by the number of events.");

  declareProperty(
      new WorkspaceProperty<ITableWorkspace>(
          "BoxDataTable", "", Direction::Output,
          Mantid::API::PropertyMode::Optional),
      "Optional output data table with MDEventWorkspace-specific box data.");
}

//----------------------------------------------------------------------------------------------
/** Make a table of box data
 * @param ws ::  MDEventWorkspace being added to
 */
template <typename MDE, size_t nd>
void QueryMDWorkspace::getBoxData(
    typename Mantid::MDEvents::MDEventWorkspace<MDE, nd>::sptr ws) {
  if (this->getPropertyValue("BoxDataTable").empty())
    return;

  ITableWorkspace_sptr output = WorkspaceFactory::Instance().createTable();
  output->addColumn("int", "RecursionDepth");
  output->addColumn("int", "NumBoxes");
  output->addColumn("int", "NumWithEvents");
  output->addColumn("double", "PctWithEvents");
  output->addColumn("int", "TotalEvents");
  output->addColumn("double", "AvgEventsPer");
  output->addColumn("double", "TotalWeight");
  output->addColumn("double", "TotalSignal");
  output->addColumn("double", "TotalErrorSquared");
  for (size_t d = 0; d < nd; d++)
    output->addColumn("double", "Dim" + Strings::toString(d));

  size_t depth = ws->getBoxController()->getMaxDepth() + 1;
  std::vector<int> NumBoxes(depth, 0);
  std::vector<int> NumWithEvents(depth, 0);
  std::vector<int> TotalEvents(depth, 0);
  std::vector<double> TotalWeight(depth, 0);
  std::vector<double> TotalSignal(depth, 0);
  std::vector<double> TotalErrorSquared(depth, 0);
  std::vector<std::vector<double>> Dims(depth, std::vector<double>(nd, 0.0));

  std::vector<API::IMDNode *> boxes;
  ws->getBox()->getBoxes(boxes, depth, true);
  for (size_t i = 0; i < boxes.size(); i++) {
    MDBoxBase<MDE, nd> *box = dynamic_cast<MDBoxBase<MDE, nd> *>(boxes[i]);
    if (!box)
      throw(std::runtime_error("Can not cast IMDNode to any type of boxes"));
    size_t d = box->getDepth();
    NumBoxes[d] += 1;
    if (box->getNPoints() > 0)
      NumWithEvents[d] += 1;
    TotalEvents[d] += static_cast<int>(box->getNPoints());
    TotalWeight[d] += box->getTotalWeight();
    TotalSignal[d] += box->getSignal();
    TotalErrorSquared[d] += box->getErrorSquared();
    for (size_t dim = 0; dim < nd; dim++)
      Dims[d][dim] = double(box->getExtents(dim).getSize());
  }

  int rowCounter = 0;
  for (size_t d = 0; d < depth; d++) {
    int col = 0;
    output->appendRow();
    output->cell<int>(rowCounter, col++) = int(d);
    output->cell<int>(rowCounter, col++) = NumBoxes[d];
    output->cell<int>(rowCounter, col++) = NumWithEvents[d];
    output->cell<double>(rowCounter, col++) =
        100.0 * double(NumWithEvents[d]) / double(NumBoxes[d]);
    output->cell<int>(rowCounter, col++) = TotalEvents[d];
    output->cell<double>(rowCounter, col++) =
        double(TotalEvents[d]) / double(NumBoxes[d]);
    output->cell<double>(rowCounter, col++) = TotalWeight[d];
    output->cell<double>(rowCounter, col++) = TotalSignal[d];
    output->cell<double>(rowCounter, col++) = TotalErrorSquared[d];
    for (size_t dim = 0; dim < nd; dim++)
      output->cell<double>(rowCounter, col++) = Dims[d][dim];
    rowCounter++;
  }

  setProperty("BoxDataTable", output);
}

//----------------------------------------------------------------------------------------------
/// Run the algorithm
void QueryMDWorkspace::exec() {
  // Extract the required normalisation.
  std::string strNormalisation = getPropertyValue("Normalisation");
  MDNormalization requestedNormalisation = whichNormalisation(strNormalisation);

  IMDWorkspace_sptr input = getProperty("InputWorkspace");
  // Define a table workspace with a specific column schema.
  ITableWorkspace_sptr output = WorkspaceFactory::Instance().createTable();
  const std::string signalColumnName = "Signal/" + strNormalisation;
  const std::string errorColumnName = "Error/" + strNormalisation;
  output->addColumn("double", signalColumnName);
  output->addColumn("double", errorColumnName);
  output->addColumn("int", "Number of Events");

  const size_t ndims = input->getNumDims();
  for (size_t index = 0; index < ndims; ++index) {
    Mantid::Geometry::IMDDimension_const_sptr dim = input->getDimension(index);
    std::string dimInUnit = dim->getName() + "/" + dim->getUnits().ascii();
    output->addColumn("double", dimInUnit);
    // Magic numbers required to configure the X axis.
    output->getColumn(dimInUnit)->setPlotType(1);
  }

  // Magic numbers required to configure the Y axis.
  output->getColumn(signalColumnName)->setPlotType(2);
  output->getColumn(errorColumnName)->setPlotType(5);

  IMDIterator *it = input->createIterator();
  it->setNormalization(requestedNormalisation);

  bool bLimitRows = getProperty("LimitRows");
  int maxRows = 0;
  if (bLimitRows) {
    maxRows = getProperty("MaximumRows");
  }

  // Use the iterator to loop through each MDBoxBase and create a row for each
  // entry.
  int rowCounter = 0;

  Progress progress(this, 0, 1, int64_t(input->getNPoints()));
  while (true) {
    size_t cellIndex = 0;
    output->appendRow();
    output->cell<double>(rowCounter, cellIndex++) = it->getNormalizedSignal();
    output->cell<double>(rowCounter, cellIndex++) = it->getNormalizedError();
    output->cell<int>(rowCounter, cellIndex++) = int(it->getNumEvents());
    VMD center = it->getCenter();
    const size_t numberOriginal = input->getNumberTransformsToOriginal();
    if (numberOriginal > 0) {
      const size_t index = numberOriginal - 1;
      CoordTransform *transform = input->getTransformToOriginal(index);
      VMD temp = transform->applyVMD(center);
      center = temp;
    }

    for (size_t index = 0; index < ndims; ++index) {
      output->cell<double>(rowCounter, cellIndex++) = center[index];
    }

    progress.report();
    if (!it->next() || (bLimitRows && ((rowCounter + 1) >= maxRows))) {
      break;
    }
    rowCounter++;
  }
  setProperty("OutputWorkspace", output);
  delete it;

  //
  IMDEventWorkspace_sptr mdew;
  CALL_MDEVENT_FUNCTION(this->getBoxData, input);
}

} // namespace Mantid
} // namespace MDEvents
