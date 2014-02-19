/*WIKI* 

The workspace data are stored in the file in columns: the first column contains the X-values, followed by pairs of Y and E values. Columns are separated by commas. The resulting file can normally be loaded into a workspace by the [[LoadAscii2]] algorithm.

==== Limitations ====
The algorithm assumes that the workspace has common X values for all spectra (i.e. is not a [[Ragged Workspace|ragged workspace]]). Only the X values from the first spectrum in the workspace are saved out. 

*WIKI*/
//----------------------------------------------------------------------
// Includes
//----------------------------------------------------------------------
#include "MantidDataHandling/SaveILLCosmosAscii.h"
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
    DECLARE_ALGORITHM(SaveILLCosmosAscii)

    /// Sets documentation strings for this algorithm
    void SaveILLCosmosAscii::initDocs()
    {
      this->setWikiSummary("Saves a 2D [[workspace]] to a comma separated ascii file. ");
      this->setOptionalMessage("Saves a 2D workspace to a ascii file.");
    }

    using namespace Kernel;
    using namespace API;

    // Initialise the logger
    Logger& SaveILLCosmosAscii::g_log = Logger::get("SaveILLCosmosAscii");

    /// Empty constructor
    SaveILLCosmosAscii::SaveILLCosmosAscii() : m_separatorIndex()
    {
    }

    /// Initialisation method.
    void SaveILLCosmosAscii::init()
    {
      declareProperty(new WorkspaceProperty<>("InputWorkspace",
        "",Direction::Input), "The name of the workspace containing the data you want to save to a ANSTO file.");
      std::vector<std::string> exts;
      exts.push_back(".mft");
      declareProperty(new FileProperty("Filename", "", FileProperty::Save, exts),
        "The filename of the output ANSTO file.");
      declareProperty(new ArrayProperty<std::string>("LogList"),"List of logs to write to file.");
      declareProperty("Usercontact", "", "Text to be written to the User-local contact field");
      m_sep = '\t';
    }

    /** 
    *   Executes the algorithm.
    */
    void SaveILLCosmosAscii::exec()
    {
      std::string filename = getProperty("Filename");
      std::ofstream file(filename.c_str());
      if (!file)
      {
        g_log.error("Unable to create file: " + filename);
        throw Exception::FileError("Unable to create file: " , filename);
      }
      m_ws = getProperty("InputWorkspace");
      g_log.information("FILENAME: " + filename);

      auto title ='#'+ m_ws->getTitle();
      const std::vector<double> & x1 = m_ws->readX(0);
      const size_t xlength = x1.size() - 1;
      std::vector<double> X1(xlength, 0);
      for (size_t i = 0; i < xlength; ++i)
      {
        X1[i]=(x1[i]+x1[i+1])/2.0;
      }
      const std::vector<double> & y1 = m_ws->readY(0);
      const std::vector<double> & e1 = m_ws->readE(0);
      double qres = (X1[1]-X1[0])/X1[1];

      g_log.information("Constant dq/q from file: " + boost::lexical_cast<std::string>(qres));

      file << std::scientific;
      file << "MFT" << std::endl;
      file << "Instrument: "<< m_ws->getInstrument()->getName() << std::endl;
      file << "User-local contact: " << std::endl; //add optional porperty
      file << "Title: "<< m_ws->getTitle() << std::endl;
      auto samp = m_ws->run();
      file << "Subtitle: " << samp.getLogData("run_title")->value() << std::endl;
      file << "Start date + time: " << samp.getLogData("run_start")->value() << std::endl;
      file << "End date + time: " << samp.getLogData("run_end")->value() << std::endl;

      const std::vector<std::string> logList = getProperty("LogList");
      ///logs
      for (auto log = logList.begin(); log != logList.end(); ++log)
      {
        file << boost::lexical_cast<std::string>(*log) << ": " << boost::lexical_cast<std::string>(samp.getLogData(*log)->value()) << std::endl;
      }

      file << "Number of file format: 2" << std::endl;
      file << "Number of data points:" << m_sep << xlength << std::endl;
      file << std::endl;

      file << m_sep << "q" << m_sep << "refl" << m_sep << "refl_err" << m_sep << "q_res" << std::endl;

      for (size_t i = 0; i < xlength; ++i)
      {
        double dq = X1[i]*qres;
        outputval(X1[i], file);
        outputval(y1[i], file);
        outputval(e1[i], file);
        outputval(dq, file);
        file << std::endl;
      }
      file.close();
    }

    void SaveILLCosmosAscii::outputval (double val, std::ofstream & file, bool leadingSep)
    {
      bool nancheck = checkIfNan(val);
      bool infcheck = checkIfInfinite(val);
      if (leadingSep)
      {
        if (!nancheck && !infcheck)
        {
          file << m_sep << val;
        }
        else if (nancheck)
        {
          //not a number - output nan
          file << m_sep << "nan";
        }
        else if (infcheck)
        {
          //infinite - output 'inf'
          file << m_sep << "inf";
        }
        else
        {
          //not valid, nan or inf - so output 'und'
          file << m_sep << "und";
        }
      }
      else
      {
        if (!nancheck && !infcheck)
        {
          file << val;
        }
        else if (nancheck)
        {
          //not a number - output nan
          file << "nan";
        }
        else if (infcheck)
        {
          //infinite - output 'inf'
          file << "inf";
        }
        else
        {
          //not valid, nan or inf - so output 'und'
          file << "und";
        }
      }
    }

    bool SaveILLCosmosAscii::checkIfNan(const double& value) const
    {
      return (boost::math::isnan(value));
    }

    bool SaveILLCosmosAscii::checkIfInfinite(const double& value) const
    {
      return (std::abs(value) == std::numeric_limits<double>::infinity());
    }

  } // namespace DataHandling
} // namespace Mantid
