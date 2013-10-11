/*WIKI* 

The workspace data are stored in the file in columns: the first column contains the X-values, followed by pairs of Y and E values. Columns are separated by commas. The resulting file can normally be loaded into a workspace by the [[LoadAscii2]] algorithm.

==== Limitations ====
The algorithm assumes that the workspace has common X values for all spectra (i.e. is not a [[Ragged Workspace|ragged workspace]]). Only the X values from the first spectrum in the workspace are saved out. 

*WIKI*/
//----------------------------------------------------------------------
// Includes
//----------------------------------------------------------------------
#include "MantidDataHandling/SaveAscii2.h"
#include "MantidKernel/UnitFactory.h"
#include "MantidKernel/ArrayProperty.h"
#include "MantidAPI/FileProperty.h"
#include "MantidKernel/BoundedValidator.h"
#include "MantidKernel/VisibleWhenProperty.h"
#include "MantidKernel/ListValidator.h"

#include <set>
#include <fstream>
#include <boost/tokenizer.hpp>

namespace Mantid
{
  namespace DataHandling
  {
    // Register the algorithm into the algorithm factory
    DECLARE_ALGORITHM(SaveAscii2)

    /// Sets documentation strings for this algorithm
    void SaveAscii2::initDocs()
    {
      this->setWikiSummary("Saves a 2D [[workspace]] to a comma separated ascii file. ");
      this->setOptionalMessage("Saves a 2D workspace to a ascii file.");
    }


    using namespace Kernel;
    using namespace API;

    // Initialise the logger
    Logger& SaveAscii2::g_log = Logger::get("SaveAscii2");

    /// Empty constructor
    SaveAscii2::SaveAscii2() : m_separatorIndex()
    {
    }

    /// Initialisation method.
    void SaveAscii2::init()
    {
      declareProperty(new WorkspaceProperty<>("InputWorkspace",
        "",Direction::Input), "The name of the workspace containing the data you want to save to a Ascii file.");

      std::vector<std::string> exts;
      exts.push_back(".dat");
      exts.push_back(".txt");
      exts.push_back(".csv");
      declareProperty(new FileProperty("Filename", "", FileProperty::Save, exts),
        "The filename of the output Ascii file.");

      auto mustBePositive = boost::make_shared<BoundedValidator<int> >();
      mustBePositive->setLower(1);
      declareProperty("WorkspaceIndexMin", 1, mustBePositive,"The starting workspace index.");
      declareProperty("WorkspaceIndexMax", EMPTY_INT(), mustBePositive,"The ending workspace index.");
      declareProperty(new ArrayProperty<int>("SpectrumList"),"List of workspace indices to save.");
      declareProperty("Precision", EMPTY_INT(), mustBePositive,"Precision of output double values.");
      declareProperty("WriteXError", false, "If true, the error on X will be written as the fourth column.");

      declareProperty("CommentIndicator", "", "Character(s) to put in front of comment lines.");

      // For the ListValidator
      std::string spacers[6][2] = { {"CSV", ","}, {"Tab", "\t"}, {"Space", " "}, 
      {"Colon", ":"}, {"SemiColon", ";"}, {"UserDefined", "UserDefined"} };
      std::vector<std::string> sepOptions;
      for( size_t i = 0; i < 6; ++i )
      {
        std::string option = spacers[i][0];
        m_separatorIndex.insert(std::pair<std::string,std::string>(option, spacers[i][1]));
        sepOptions.push_back(option);
      }

      declareProperty("Separator", "CSV", boost::make_shared<StringListValidator>(sepOptions),
        "Character(s) to put as separator between X, Y, E values.");

      declareProperty(new PropertyWithValue<std::string>("CustomSeparator", "", Direction::Input),
        "If present, will override any specified choice given to Separator.");

      setPropertySettings("CustomSeparator", new VisibleWhenProperty("Separator", IS_EQUAL_TO, "UserDefined") );

      declareProperty("ColumnHeader", true, "If true, put column headers into file. ");

    }

    /** 
    *   Executes the algorithm.
    */
    void SaveAscii2::exec()
    {
      // Get the workspace
      MatrixWorkspace_const_sptr ws = getProperty("InputWorkspace");
      int nSpectra = static_cast<int>(ws->getNumberHistograms());
      int nBins = static_cast<int>(ws->blocksize());
      bool isHistogram = ws->isHistogramData();

      // Get the properties
      std::vector<int> spec_list = getProperty("SpectrumList");
      int spec_min = getProperty("WorkspaceIndexMin");
      int spec_max = getProperty("WorkspaceIndexMax");
      bool writeHeader = getProperty("ColumnHeader");

      // Check whether we need to write the fourth column
      bool write_dx = getProperty("WriteXError");
      std::string choice = getPropertyValue("Separator");
      std::string custom = getPropertyValue("CustomSeparator");
      std::string sep;
      // If the custom separator property is not empty, then we use that under any circumstance.
      if(custom != "")
      {
        sep = custom;
      }
      // Else if the separator drop down choice is not UserDefined then we use that.
      else if(choice != "UserDefined")
      {
        std::map<std::string, std::string>::iterator it = m_separatorIndex.find(choice);
        sep = it->second;
      }
      // If we still have nothing, then we are forced to use a default.
      if(sep.empty())
      {
        g_log.notice() << "\"UserDefined\" has been selected, but no custom separator has been entered.  Using default instead.";
        sep = " , ";
      }
      std::string comment = getPropertyValue("CommentIndicator");

      // Create an spectra index list for output
      std::set<int> idx;

      // Add spectra interval into the index list
      if (spec_max != EMPTY_INT() && spec_min != EMPTY_INT())
      {
        if (spec_min >= nSpectra || spec_max >= nSpectra || spec_min > spec_max)
        {
          throw std::invalid_argument("Inconsistent spectra interval");
        }
        for(int i=spec_min;i<=spec_max;i++)
        {
          idx.insert(i);
        }
      }

      // Add spectra list into the index list
      if (!spec_list.empty())
      {
        for(size_t i=0;i<spec_list.size();i++)
        {
          if (spec_list[i] >= nSpectra)
          {
            throw std::invalid_argument("Inconsistent spectra list");
          }
          else
          {
            idx.insert(spec_list[i]);
          }
        }
      }
      if (!idx.empty())
      {
        nSpectra = static_cast<int>(idx.size());
      }

      if (nBins == 0 || nSpectra == 0)
      {
        throw std::runtime_error("Trying to save an empty workspace");
      }
      std::string filename = getProperty("Filename");
      std::ofstream file(filename.c_str());

      if (!file)
      {
        g_log.error("Unable to create file: " + filename);
        throw Exception::FileError("Unable to create file: " , filename);
      }
      // Set the number precision
      int prec = getProperty("Precision");
      if (prec != EMPTY_INT())
      {
        file.precision(prec);
      }




      if (idx.empty())
      {
        Progress progress(this,0,1,nSpectra);
        for(int i=0;i<nSpectra;i++)
        {
          auto spec = ws->getSpectrum(i);
          auto specNo = spec->getSpectrumNo();
          file << specNo << std::endl;
          if( writeHeader)
          {
            file << comment << "X, Y, E";
            if (write_dx) file << " , DX";
            file << std::endl;
          }

          for(int bin=0;bin<nBins;bin++)
          {
            if (isHistogram) // bin centres
            {
              file << ( ws->readX(0)[bin] + ws->readX(0)[bin+1] )/2;
            }
            else // data points
            {
              file << ws->readX(0)[bin];
            }
            file << sep;
            file << ws->readY(i)[bin];
            file << sep;
            file << ws->readE(i)[bin];
            if (write_dx)
            {
              if (isHistogram) // bin centres
              {
                file << sep;
                file << ( ws->readDx(0)[bin] + ws->readDx(0)[bin+1] )/2;
              }
              else // data points
              {
                file << sep;
                file << ws->readDx(0)[bin];
              }
            }
            file << std::endl;
          }
          file << std::endl;

          progress.report();
        }
      }
      else
      {
        Progress progress(this,0,1,idx.size());
        for(std::set<int>::const_iterator i=idx.begin();i!=idx.end();++i)
        {
          auto spec = ws->getSpectrum(*i);
          auto specNo = spec->getSpectrumNo();
          file << specNo << std::endl;
          if( writeHeader)
          {
            file << comment << "X, Y, E";
            if (write_dx) file << " , DX";
            file << std::endl;
          }

          for(int bin=0;bin<nBins;bin++)                                                                                                                                                                                                                                                                     
          {

            if (isHistogram) // bin centres
            {
              file << ( ws->readX(0)[bin] + ws->readX(0)[bin+1] )/2;
            }
            else // data points
            {
              file << ws->readX(0)[bin];
            }
            file << sep;
            file << ws->readY(*i)[bin];                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                 
            file << sep;
            std::cerr<< sep;
            file << ws->readE(*i)[bin];
            std::cerr<< ws->readE(*i)[bin];
            file << std::endl;
            if (write_dx)
            {
              if (isHistogram) // bin centres
              {
                file << sep;
                std::cerr<< sep;
                file << ( ws->readDx(0)[bin] + ws->readDx(0)[bin+1] )/2;
                std::cerr<< ( ws->readDx(0)[bin] + ws->readDx(0)[bin+1] )/2;
              }
              else // data points
              {
                file << sep;
                std::cerr<< sep;
                file << ws->readDx(0)[bin];
                std::cerr<< ws->readDx(0)[bin];
              }
            }
            std::cerr<< std::endl;
          }


          file << std::endl;
          std::cerr<< std::endl;

          progress.report();
        }
      }




      /*
      for(int bin=0;bin<nBins;bin++)
      {
      if (isHistogram) // bin centres
      {
      file << ( ws->readX(0)[bin] + ws->readX(0)[bin+1] )/2;
      std::cerr<< ( ws->readX(0)[bin] + ws->readX(0)[bin+1] )/2;
      }
      else // data points
      {
      file << ws->readX(0)[bin];
      std::cerr<< ws->readX(0)[bin];
      }

      if (idx.empty())
      {
      for(int spec=0;spec<nSpectra;spec++)
      {
      file << sep;
      std::cerr<< sep;
      file << ws->readY(spec)[bin];
      std::cerr<< ws->readY(spec)[bin];
      file << sep;
      std::cerr<< sep;
      file << ws->readE(spec)[bin];
      std::cerr<< ws->readE(spec)[bin];
      }
      }
      else
      {
      for(std::set<int>::const_iterator spec=idx.begin();spec!=idx.end();++spec)
      {
      file << sep;
      std::cerr<< sep;
      file << ws->readY(*spec)[bin]; 
      std::cerr<< ws->readY(*spec)[bin]; 
      file << sep;
      std::cerr<< sep;
      file << ws->readE(*spec)[bin];
      std::cerr<< ws->readE(*spec)[bin];
      }
      }
      if (write_dx)
      {
      if (isHistogram) // bin centres
      {
      file << sep;
      std::cerr<< sep;
      file << ( ws->readDx(0)[bin] + ws->readDx(0)[bin+1] )/2;
      std::cerr<< ( ws->readDx(0)[bin] + ws->readDx(0)[bin+1] )/2;
      }
      else // data points
      {
      file << sep;
      std::cerr<< sep;
      file << ws->readDx(0)[bin];
      std::cerr<< ws->readDx(0)[bin];
      }
      }
      file << std::endl;
      std::cerr<< std::endl;
      }



      // Write the column captions
      if( writeHeader) {
      file << comment << "X";
      std::cerr << comment << "X";
      if (idx.empty())
      {
      for(int spec=0;spec<nSpectra;spec++)
      {
      file << " , Y" << spec << " , E" << spec;
      std::cerr<< " , Y" << spec << " , E" << spec;
      if (write_dx) file << " , DX" << spec;
      if (write_dx) std::cerr<< " , DX" << spec;
      }
      }
      else
      {
      for(std::set<int>::const_iterator spec=idx.begin();spec!=idx.end();++spec)
      {
      file << " , Y" << *spec << " , E" << *spec;
      std::cerr<< " , Y" << *spec << " , E" << *spec;
      if (write_dx) file << " , DX" << *spec;
      if (write_dx) std::cerr<< " , DX" << *spec;
      }
      }
      file << std::endl;
      std::cerr<< std::endl;
      }




      Progress progress(this,0,1,nBins);
      */

    }

  } // namespace DataHandling
} // namespace Mantid
