#include "MantidAlgorithms/FixGSASInstrumentFile.h"

#include "MantidDataHandling/LoadFullprofResolution.h"
#include "MantidAPI/FileProperty.h"
#include "MantidKernel/ArrayProperty.h"
#include "MantidAPI/WorkspaceProperty.h"
#include "MantidAPI/TableRow.h"

#include <boost/algorithm/string/trim.hpp>
#include <boost/algorithm/string.hpp>
#include <boost/algorithm/string/iter_find.hpp>
#include <boost/algorithm/string/finder.hpp>
#include <boost/algorithm/string/predicate.hpp>

namespace Mantid
{
namespace Algorithms
{


  //----------------------------------------------------------------------------------------------
  /** Constructor
   */
  FixGSASInstrumentFile::FixGSASInstrumentFile()
  {
  }
    
  //----------------------------------------------------------------------------------------------
  /** Destructor
   */
  FixGSASInstrumentFile::~FixGSASInstrumentFile()
  {
  }
  
  //----------------------------------------------------------------------------------------------
  /** Sets documentation strings for this algorithm
    */
  void LoadFullprofResolution::initDocs()
  {
    setWikiSummary("Load Fullprof's resolution (.irf) file to one or multiple TableWorkspace(s).");
    setOptionalMessage("Load Fullprof's resolution (.irf) file to one or multiple TableWorkspace(s).");

    return;
  }

  //----------------------------------------------------------------------------------------------
  /** Implement abstract Algorithm methods
   */
  void LoadFullprofResolution::init()
  {
    // Input file name
    vector<std::string> exts;
    exts.push_back(".irf");
    declareProperty(new FileProperty("Filename", "", FileProperty::Load, exts),
        "Path to an Fullprof .irf file to load.");

    // Output workspace
    auto wsprop = new WorkspaceProperty<TableWorkspace>("OutputWorkspace", "", Direction::Output);
    declareProperty(wsprop, "Name of the output TableWorkspace containing profile parameters or bank information. ");

    // Bank to import
    declareProperty(new ArrayProperty<int>("Banks"), "ID(s) of specified bank(s) to load. "
                    "Default is all banks contained in input .irf file.");

    // declareProperty("Bank", EMPTY_INT(), "ID of a specific bank to load. Default is all banks in .irf file.");

    return;
  }



} // namespace Algorithms
} // namespace Mantid
