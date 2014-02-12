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
    SaveANSTO::SaveANSTO()
    {
    }

    /// Initialisation method.
    void SaveANSTO::init()
    {
      declareProperty(new WorkspaceProperty<>("InputWorkspace",
        "",Direction::Input), "The name of the workspace containing the data you want to save to a ANSTO file.");

      std::vector<std::string> exts;
      exts.push_back(".txt");
      declareProperty(new FileProperty("Filename", "", FileProperty::Save, exts),
        "The filename of the output ANSTO file.");
    }

    /** 
    *   Executes the algorithm.
    */
    void SaveANSTO::exec()
    {
      std::string filename = getProperty("Filename");
      std::ofstream file(filename.c_str());
      m_ws = getProperty("InputWorkspace");
      g_log.information("FILENAME: " + filename);
      auto title ='#'+ m_ws->getTitle();
      auto x1 = m_ws->readX(0);
      const size_t xlength = x1.size() - 1;
      std::vector<double> X1;
      X1.resize(xlength, 0);
      for (int i = 0; i < xlength; ++i)
      {
        X1[i]=(x1[i]+x1[i+1])/2.0;
      }
      auto y1 = m_ws->readY(0);
      auto e1 = m_ws->readE(0);
      char sep = '\t';
      double qres = (X1[1]-X1[0])/X1[1];
      g_log.information("Constant dq/q from file: " + boost::lexical_cast<std::string>(qres));
      file << std::scientific;
      for (int i = 0; i < xlength; ++i)
      {
        double dq = X1[i]*qres;
        file << X1[i] << sep << y1[i] << sep << e1[i] << sep << dq << std::endl;
      }
      file.close();
    }
  } // namespace DataHandling
} // namespace Mantid
