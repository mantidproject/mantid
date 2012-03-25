/*WIKI*

This algorithm outputs a table workspace containing summary data about each box within an IMDWorkspace.
The table workspace can be used as a basis for plotting within MantidPlot.

== Format ==
* Column 1: Signal (double)
* Column 2: Error (double)
* Column 3: Number of Events (integer)
* Column 4: Coords of box center (string)

*WIKI*/

#include "MantidMDEvents/QueryMDWorkspace.h"
#include "MantidKernel/System.h"
#include "MantidAPI/IMDEventWorkspace.h"
#include "MantidAPI/IMDIterator.h"
#include "MantidAPI/ITableWorkspace.h"
#include "MantidAPI/WorkspaceFactory.h"
#include "MantidAPI/TableRow.h"
#include "MantidKernel/EnabledWhenProperty.h"
#include "MantidKernel/BoundedValidator.h"

using namespace Mantid::Kernel;
using namespace Mantid::API;

namespace Mantid
{
namespace MDEvents
{

  // Register the class into the algorithm factory
  DECLARE_ALGORITHM(QueryMDWorkspace)

  //----------------------------------------------------------------------------------------------
  /** Constructor
   */
  QueryMDWorkspace::QueryMDWorkspace()
  {
  }
    
  //----------------------------------------------------------------------------------------------
  /** Destructor
   */
  QueryMDWorkspace::~QueryMDWorkspace()
  {
  }


  /// Documentation initalisation 
  void QueryMDWorkspace::initDocs()
  {
    this->setWikiSummary("Query the IMDWorkspace in order to extract summary information.");
  }

  /// Initialise the properties
  void QueryMDWorkspace::init()
  {
    declareProperty(new WorkspaceProperty<IMDWorkspace>("InputWorkspace","",Direction::Input), "An input MDWorkspace.");

    declareProperty(new WorkspaceProperty<ITableWorkspace>("OutputWorkspace","",Direction::Output),
        "The output Tableworkspace"
        "with columns containing key summary information about the MDWorkspace.");

    declareProperty("LimitRows", true, "Limit the report output to a maximum number of rows");

    declareProperty(new PropertyWithValue<int>("MaximumRows", 100000, boost::make_shared<BoundedValidator<int>>(), Direction::Input), "The number of neighbours to utilise. Defaults to 100000.");
    setPropertySettings("MaximumRows", new EnabledWhenProperty(this, "LimitRows", IS_DEFAULT));
  }

  /// Run the algorithm
  void QueryMDWorkspace::exec()
  {
    // Define a table workspace with a specific column schema.
    ITableWorkspace_sptr output = WorkspaceFactory::Instance().createTable();
    output->addColumn("double", "Signal");
    output->addColumn("double", "Error");
    output->addColumn("int", "Number of Events");
    output->addColumn("str", "Center");

    IMDWorkspace_sptr input = getProperty("InputWorkspace");
    IMDIterator* it = input->createIterator();

    bool bLimitRows = getProperty("LimitRows");
    int maxRows = 0;
    if(bLimitRows)
    {
      maxRows = getProperty("MaximumRows");
    }
   
    // Use the iterator to loop through each IMDBox and create a row for each entry.
    int rowCounter = 0;

    Progress progress(this, 0, 1, int64_t(input->getNPoints()));
    while(true)
    {
      output->appendRow();
      output->cell<double>(rowCounter, 0) = it->getNormalizedSignal();
      output->cell<double>(rowCounter, 1) = std::sqrt(it->getNormalizedError());
      output->cell<int>(rowCounter, 2) = int(it->getNumEvents());
      VMD center = it->getCenter();
      output->cell<std::string>(rowCounter, 3) = center.toString(",");
      progress.report();
      if(!it->next() || (bLimitRows && ((rowCounter+1) >= maxRows)))
      {
        break;
      }
      rowCounter++;
    }
    setProperty("OutputWorkspace", output);
    delete it;
  }



} // namespace Mantid
} // namespace MDEvents