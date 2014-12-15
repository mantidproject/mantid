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
#include <boost/regex.hpp>

namespace Mantid
{
  namespace DataHandling
  {
    // Register the algorithm into the algorithm factory
    DECLARE_ALGORITHM(SaveAscii2)

    using namespace Kernel;
    using namespace API;

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
      auto mustBeZeroGreater = boost::make_shared<BoundedValidator<int> >();
      mustBeZeroGreater->setLower(0);
      declareProperty("WorkspaceIndexMin", EMPTY_INT(), mustBeZeroGreater,"The starting workspace index.");
      declareProperty("WorkspaceIndexMax", EMPTY_INT(), mustBeZeroGreater,"The ending workspace index.");
      declareProperty(new ArrayProperty<int>("SpectrumList"),"List of workspace indices to save.");
      declareProperty("Precision", EMPTY_INT(), mustBePositive,"Precision of output double values.");
      declareProperty("ScientificFormat", false, "If true, the values will be written to the file in scientific notation.");
      declareProperty("WriteXError", false, "If true, the error on X will be written as the fourth column.");
      declareProperty("WriteSpectrumID", true, "If false, the spectrum ID will not be written for single-spectrum workspaces. "
          "It is always written for workspaces with multiple spectra.");

      declareProperty("CommentIndicator", "#", "Character(s) to put in front of comment lines.");
      
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
        "The separator between data columns in the data file. The possible values are \"CSV\", \"Tab\", "
        "\"Space\", \"SemiColon\", \"Colon\" or \"UserDefined\".");

      declareProperty(new PropertyWithValue<std::string>("CustomSeparator", "", Direction::Input),
        "If present, will override any specified choice given to Separator.");

      setPropertySettings("CustomSeparator", new VisibleWhenProperty("Separator", IS_EQUAL_TO, "UserDefined") );

      declareProperty("ColumnHeader", true, "If true, put column headers into file. ");

      declareProperty("AppendToFile", false, "If true, don't overwrite the file. Append to the end of it. ");

      declareProperty("RaggedWorkspace", true, "If true, ensure that more than one xspectra is used. "); //in testing
    }

    /** 
    *   Executes the algorithm.
    */
    void SaveAscii2::exec()
    {
      // Get the workspace
      m_ws = getProperty("InputWorkspace");
      int nSpectra = static_cast<int>(m_ws->getNumberHistograms());
      m_nBins = static_cast<int>(m_ws->blocksize());
      m_isHistogram = m_ws->isHistogramData();
      m_isCommonBins = m_ws->isCommonBins(); //checking for ragged workspace
      m_writeID = getProperty("WriteSpectrumID");
      if (nSpectra != 1) m_writeID = true;

      // Get the properties
      std::vector<int> spec_list = getProperty("SpectrumList");
      int spec_min = getProperty("WorkspaceIndexMin");
      int spec_max = getProperty("WorkspaceIndexMax");
      bool writeHeader = getProperty("ColumnHeader");
      bool appendToFile = getProperty("AppendToFile");

      // Check whether we need to write the fourth column
      m_writeDX = getProperty("WriteXError");
      std::string choice = getPropertyValue("Separator");
      std::string custom = getPropertyValue("CustomSeparator");
      // If the custom separator property is not empty, then we use that under any circumstance.
      if(custom != "")
      {
        m_sep = custom;
      }
      // Else if the separator drop down choice is not UserDefined then we use that.
      else if(choice != "UserDefined")
      {
        std::map<std::string, std::string>::iterator it = m_separatorIndex.find(choice);
        m_sep = it->second;
      }
      // If we still have nothing, then we are forced to use a default.
      if(m_sep.empty())
      {
        g_log.notice() << "\"UserDefined\" has been selected, but no custom separator has been entered."
          " Using default instead.";
        m_sep = ",";
      }

      // e + and - are included as they're part of the scientific notation
      if (!boost::regex_match(m_sep.begin(), m_sep.end(), boost::regex("[^0-9e+-]+", boost::regex::perl)))
      {
        throw std::invalid_argument("Separators cannot contain numeric characters, plus signs, hyphens or 'e'");
      }

      std::string comment = getPropertyValue("CommentIndicator");

      if (comment.at(0) == m_sep.at(0)||!boost::regex_match(comment.begin(), comment.end(), boost::regex("[^0-9e" + m_sep + "+-]+", boost::regex::perl)))
      {
        throw std::invalid_argument("Comment markers cannot contain numeric characters, plus signs, hyphens,"
          " 'e' or the selected separator character");
      }

      // Create an spectra index list for output
      std::set<int> idx;

      // Add spectra interval into the index list
      if (spec_max != EMPTY_INT() && spec_min != EMPTY_INT())
      {
        if (spec_min >= nSpectra || spec_max >= nSpectra || spec_min < 0 || spec_max < 0 || spec_min > spec_max)
        {
          throw std::invalid_argument("Inconsistent spectra interval");
        }
        for(int i=spec_min;i<=spec_max;i++)
        {
          idx.insert(i);
        }
      }
      //figure out how to read in readX and have them be seperate lists

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

      if (m_nBins == 0 || nSpectra == 0)
      {
        throw std::runtime_error("Trying to save an empty workspace");
      }
      std::string filename = getProperty("Filename");
      std::ofstream file(filename.c_str(), (appendToFile ? std::ios::app : std::ios::out));

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
      bool scientific = getProperty("ScientificFormat");
      if (scientific)
      {
        file << std::scientific;
      }
      if( writeHeader)
      {
        file << comment << " X "<< m_sep << " Y "<< m_sep <<" E";
        if (m_writeDX)
        {
          file << " "<< m_sep <<" DX";
        }
        file << std::endl;
      }

      if (idx.empty())
      {
        Progress progress(this,0,1,nSpectra);
        for(int i=0;i<nSpectra;i++)
        {
          writeSpectra(i, file);
          progress.report();
        }
      }
      else
      {
        Progress progress(this,0,1,idx.size());
        for(std::set<int>::const_iterator i=idx.begin();i!=idx.end();++i)
        {
          writeSpectra(i, file);
          progress.report();
        }
      }

      file.unsetf(std::ios_base::floatfield);
      file.close();
    }

    /**writes a spectra to the file using an iterator
    @param spectraItr :: a set<int> iterator pointing to a set of workspace IDs to be saved
    @param file :: the file writer object
    */
    void SaveAscii2::writeSpectra(const std::set<int>::const_iterator & spectraItr, std::ofstream & file) 
    {
      auto spec = m_ws->getSpectrum(*spectraItr);
      auto specNo = spec->getSpectrumNo();
      if (m_writeID) file << specNo << std::endl;
      
      for(int bin=0;bin<m_nBins;bin++)                                                                                                                                                                                                                                                                     
      {
        if (!m_isCommonBins) //checking for ragged workspace
        {
          file << (m_ws->readX(*spectraItr)[bin] + m_ws->readX(*spectraItr)[bin+1] )/2;
        }

        else if (m_isHistogram & m_isCommonBins) // bin centres,  
        {
          file << ( m_ws->readX(0)[bin] + m_ws->readX(0)[bin+1] )/2; 
        }

        else 
        {
          file << m_ws->readX(0)[bin];
        }
        file << m_sep;
        file << m_ws->readY(*spectraItr)[bin];

        file << m_sep;
        file << m_ws->readE(*spectraItr)[bin];
        if (m_writeDX)
        {
          if (m_isHistogram) // bin centres
          {
            file << m_sep;
            file << ( m_ws->readDx(0)[bin] + m_ws->readDx(0)[bin+1] )/2;
          }
          else // data points
          {
            file << m_sep;
            file << m_ws->readDx(0)[bin];
          }
        }
        file << std::endl;
      }
    }

    /**writes a spectra to the file using a workspace ID
    @param spectraIndex :: an integer relating to a workspace ID
    @param file :: the file writer object
    */
    void SaveAscii2::writeSpectra(const int & spectraIndex, std::ofstream & file) 
    {
      auto spec = m_ws->getSpectrum(spectraIndex);
      auto specNo = spec->getSpectrumNo();
      if (m_writeID) file << specNo << std::endl;

      for(int bin=0;bin<m_nBins;bin++)
      {
        if (m_isHistogram & m_isCommonBins) // bin centres, 
        {
          file << ( m_ws->readX(0)[bin] + m_ws->readX(0)[bin+1] )/2;
        }
        else if (!m_isCommonBins) //checking for ragged workspace
        {
         file << ( m_ws->readX(spectraIndex)[bin] + m_ws->readX(spectraIndex)[bin+1] )/2;
        }
        else // data points
        {
          file << m_ws->readX(0)[bin];
        }
        file << m_sep;
        file << m_ws->readY(spectraIndex)[bin];

        file << m_sep;
        file << m_ws->readE(spectraIndex)[bin];
        if (m_writeDX)
        {
          if (m_isHistogram) // bin centres
          {
            file << m_sep;
            file << ( m_ws->readDx(0)[bin] + m_ws->readDx(0)[bin+1] )/2;
          }
          else // data points
          {
            file << m_sep;
            file << m_ws->readDx(0)[bin];
          }
        }
        file << std::endl;
      }
    }
  } // namespace DataHandling
} // namespace Mantid
