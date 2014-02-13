/*WIKI* 

==== Limitations ====

*WIKI*/
//----------------------------------------------------------------------
// Includes
//----------------------------------------------------------------------
#include "MantidDataHandling/SaveANSTOAscii.h"
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
    DECLARE_ALGORITHM(SaveANSTOAscii)

    /// Sets documentation strings for this algorithm
    void SaveANSTOAscii::initDocs()
    {
      this->setWikiSummary("Saves a 2D [[workspace]] to a comma separated ascii file. ");
      this->setOptionalMessage("Saves a 2D workspace to a ascii file.");
    }

    using namespace Kernel;
    using namespace API;

    // Initialise the logger
    Logger& SaveANSTOAscii::g_log = Logger::get("SaveANSTOAscii");

    /// Empty constructor
    SaveANSTOAscii::SaveANSTOAscii()
    {
    }

    /// Initialisation method.
    void SaveANSTOAscii::init()
    {
      declareProperty(new WorkspaceProperty<>("InputWorkspace",
        "",Direction::Input), "The name of the workspace containing the data you want to save to a ANSTO file.");

      std::vector<std::string> exts;
      exts.push_back(".txt");
      declareProperty(new FileProperty("Filename", "", FileProperty::Save, exts),
        "The filename of the output ANSTO file.");
      m_sep = '\t';
    }

    /** 
    *   Executes the algorithm.
    */
    void SaveANSTOAscii::exec()
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
      std::vector<double> X1;
      X1.resize(xlength, 0);
      for (size_t i = 0; i < xlength; ++i)
      {
        X1[i]=(x1[i]+x1[i+1])/2.0;
      }
      const std::vector<double> & y1 = m_ws->readY(0);
      const std::vector<double> & e1 = m_ws->readE(0);
      double qres = (X1[1]-X1[0])/X1[1];
      g_log.information("Constant dq/q from file: " + boost::lexical_cast<std::string>(qres));
      file << std::scientific;
      for (size_t i = 0; i < xlength; ++i)
      {
        double dq = X1[i]*qres;
        outputval(X1[i], file, false);
        outputval(y1[i], file);
        outputval(e1[i], file);
        outputval(dq, file);
        file << std::endl;
      }
      file.close();
    }

    void SaveANSTOAscii::outputval (double val, std::ofstream & file, bool leadingSep)
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

    bool SaveANSTOAscii::checkIfNan(const double& value) const
    {
      return (boost::math::isnan(value));
    }

    bool SaveANSTOAscii::checkIfInfinite(const double& value) const
    {
      return (std::abs(value) == std::numeric_limits<double>::infinity());
    }
  } // namespace DataHandling
} // namespace Mantid
