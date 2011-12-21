#include "MantidDataHandling/RenameLog.h"
#include "MantidKernel/System.h"
#include "MantidKernel/Property.h"
#include "MantidKernel/TimeSeriesProperty.h"

using namespace Mantid::Kernel;
using namespace Mantid::API;

namespace Mantid
{
namespace DataHandling
{

  DECLARE_ALGORITHM(RenameLog)


  //----------------------------------------------------------------------------------------------
  /** Constructor
   */
  RenameLog::RenameLog()
  {
  }
    
  //----------------------------------------------------------------------------------------------
  /** Destructor
   */
  RenameLog::~RenameLog()
  {
  }
  
  void RenameLog::initDocs(){

    this->setWikiSummary("Merge 2 TimeSeries logs in a given Workspace. ");
    this->setOptionalMessage("Merge 2 TimeSeries logs in a given Workspace.");

    return;
  }

  void RenameLog::init(){

    declareProperty(new API::WorkspaceProperty<API::MatrixWorkspace>("Workspace", "Anonymous", Direction::InOut),
        "Workspace to have logs merged");
    declareProperty("OriginalLogName", "", "Log's original name.");
    declareProperty("NewLogName", "", "Log's new name.");

    return;
  }

  void RenameLog::exec(){

    // 1. Get value
    matrixWS = this->getProperty("Workspace");
    std::string origlogname = this->getProperty("OriginalLogName");
    std::string newlogname = this->getProperty("NewLogName");

    // 2. Checkc
    if (origlogname.size()==0 || newlogname.size()==0){
      g_log.error() << "Input original or new log's name cannot be left empty!" << std::endl;
      throw;
    }

    Kernel::Property* property = matrixWS->run().getLogData(origlogname);
    Kernel::TimeSeriesProperty<double> *timeprop = dynamic_cast<Kernel::TimeSeriesProperty<double>* >(property);

    // std::cout << "Remove log" << origlogname << std::endl;
    matrixWS->mutableRun().removeLogData(origlogname, false);

    if (!timeprop){
      g_log.error() << "After Log data is removed, TimeSeriesProperty " << origlogname << " is deleted from memory" << std::endl;
      throw;
    }

    // std::cout << "Change log name" << std::endl;
    timeprop->setName(newlogname);
    // std::cout << "Add log" << timeprop->name() << std::endl;
    // std::vector<Kernel::DateAndTime> newtimes = timeprop->timesAsVector();
    // std::cout << "Entries = " << newtimes.size() << std::endl;
    matrixWS->mutableRun().addProperty(timeprop);

    return;
  }




} // namespace Mantid
} // namespace DataHandling
