/*WIKI* 

==== Limitations ====

*WIKI*/
//----------------------------------------------------------------------
// Includes
//----------------------------------------------------------------------
#include "MantidDataHandling/SaveANSTO.h"
#include "MantidKernel/UnitFactory.h"
#include "MantidKernel/ArrayProperty.h"
#include "MantidAPI/FileProperty.h"
#include "MantidKernel/BoundedValidator.h"
#include "MantidKernel/VisibleWhenProperty.h"
#include "MantidKernel/ListValidator.h"
#include <set>
#include <fstream>
#include <boost/tokenizer.hpp>
#include <boost/regex.hpp>

namespace Mantid
{
  namespace DataHandling
  {
    // Register the algorithm into the algorithm factory
    DECLARE_ALGORITHM(SaveANSTO)

    /// Sets documentation strings for this algorithm
    void SaveANSTO::initDocs()
    {
      this->setWikiSummary("Saves a 2D [[workspace]] to a comma separated ascii file. ");
      this->setOptionalMessage("Saves a 2D workspace to a ascii file.");
    }

    using namespace Kernel;
    using namespace API;

    // Initialise the logger
    Logger& SaveANSTO::g_log = Logger::get("SaveANSTO");

    /// Empty constructor
    SaveANSTO::SaveANSTO() : m_separatorIndex()
    {
    }

    /// Initialisation method.
    void SaveANSTO::init()
    {
    }

    /** 
    *   Executes the algorithm.
    */
    void SaveANSTO::exec()
    {
    }
  } // namespace DataHandling
} // namespace Mantid
