/*WIKI* 
SaveANSTOAscii is an export-only Acii-based save format with no associated loader. It is based on a python script by Maximilian Skoda, written for the ISIS Reflectometry GUI
==== Limitations ====
While Files saved with SaveANSTOAscii can be loaded back into mantid using LoadAscii, the resulting workspaces won't be usful as the data written by SaveANSTOAscii is not in the normal X,Y,E,DX format.
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
#include <boost/math/special_functions/fpclassify.hpp>

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
      const std::vector<double> & xTemp = m_ws->readX(0);
      const size_t xlength = xTemp.size() - 1;
      std::vector<double> XData(xlength, 0);
      for (size_t i = 0; i < xlength; ++i)
      {
        XData[i]=(xTemp[i]+xTemp[i+1])/2.0;
      }
      const std::vector<double> & yData = m_ws->readY(0);
      const std::vector<double> & eData = m_ws->readE(0);
      double qres = (XData[1]-XData[0])/XData[1];
      g_log.information("Constant dq/q from file: " + boost::lexical_cast<std::string>(qres));
      file << std::scientific;
      for (size_t i = 0; i < xlength; ++i)
      {
        double dq = XData[i]*qres;
        outputval(XData[i], file, false);
        outputval(yData[i], file);
        outputval(eData[i], file);
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
