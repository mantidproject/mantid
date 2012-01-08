/*WIKI* 

The workspace data are stored in the file in columns: the first column contains the X-values, followed by pairs of Y and E values. Columns are separated by commas.

==== Limitations ====
The algorithm assumes that the workspace has common X values for all spectra (i.e. is not a [[Ragged Workspace|ragged workspace]]). Only the X values from the first spectrum in the workspace are saved out.


*WIKI*/
//----------------------------------------------------------------------
// Includes
//----------------------------------------------------------------------
#include "MantidDataHandling/SaveAscii.h"
#include "MantidKernel/UnitFactory.h"
#include "MantidKernel/ArrayProperty.h"
#include "MantidAPI/FileProperty.h"

#include "MantidKernel/VisibleWhenProperty.h"

#include <set>
#include <fstream>
#include <boost/tokenizer.hpp>

namespace Mantid
{
  namespace DataHandling
  {
    // Register the algorithm into the algorithm factory
    DECLARE_ALGORITHM(SaveAscii)
    
    /// Sets documentation strings for this algorithm
    void SaveAscii::initDocs()
    {
      this->setWikiSummary("Saves a 2D [[workspace]] to a Ascii file. ");
      this->setOptionalMessage("Saves a 2D workspace to a Ascii file.");
    }
    

    using namespace Kernel;
    using namespace API;

    // Initialise the logger
    Logger& SaveAscii::g_log = Logger::get("SaveAscii");

    /// Empty constructor
    SaveAscii::SaveAscii() : m_separatorIndex()
    {
    }

    /// Initialisation method.
    void SaveAscii::init()
    {
      std::vector<std::string> exts;
      exts.push_back(".dat");
      exts.push_back(".txt");
      exts.push_back(".csv");
      declareProperty(new FileProperty("Filename", "", FileProperty::Save, exts),
		      "A comma separated Ascii file that will be created");
      declareProperty(new WorkspaceProperty<>("InputWorkspace",
        "",Direction::Input), "The name of the workspace that will be saved.");

      BoundedValidator<int> *mustBePositive = new BoundedValidator<int>();
      mustBePositive->setLower(1);
      declareProperty("WorkspaceIndexMin", 1, mustBePositive);
      declareProperty("WorkspaceIndexMax", EMPTY_INT(), mustBePositive->clone());
      declareProperty(new ArrayProperty<int>("SpectrumList"));
      declareProperty("Precision", EMPTY_INT(), mustBePositive->clone());
      declareProperty("WriteXError", false, "If true, the error on X with be written as the fourth column.");

      declareProperty("CommentIndicator", "", "Characters to put in front of comment lines.");
      
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
      
      declareProperty("Separator", "CSV", new ListValidator(sepOptions),
        "Characters to put as separator between X, Y, E values. (Default: CSV)");

      declareProperty(new PropertyWithValue<std::string>("CustomSeparator", "", Direction::Input),
        "If present, will overide any specified choice given to Separator.");

      setPropertySettings("CustomSeparator", new VisibleWhenProperty(this, "Separator", IS_EQUAL_TO, "UserDefined") );
    }

    /** 
     *   Executes the algorithm.
     */
    void SaveAscii::exec()
    {
        // Get the workspace
        MatrixWorkspace_const_sptr ws = getProperty("InputWorkspace");
        int nSpectra = static_cast<int>(ws->getNumberHistograms());
        int nBins = static_cast<int>(ws->blocksize());

        // Get the properties
        std::vector<int> spec_list = getProperty("SpectrumList");
        int spec_min = getProperty("WorkspaceIndexMin");
        int spec_max = getProperty("WorkspaceIndexMax");

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
                throw std::invalid_argument("Inconsistent spectra interval");
            for(int spec=spec_min;spec<=spec_max;spec++)
                    idx.insert(spec);
        }

        // Add spectra list into the index list
        if (!spec_list.empty())
            for(size_t i=0;i<spec_list.size();i++)
                if (spec_list[i] >= nSpectra) throw std::invalid_argument("Inconsistent spectra list");
                else
                    idx.insert(spec_list[i]);

        if (!idx.empty()) nSpectra = static_cast<int>(idx.size());

        if (nBins == 0 || nSpectra == 0) throw std::runtime_error("Trying to save an empty workspace");

        std::string filename = getProperty("Filename");
        std::ofstream file(filename.c_str());

        if (!file)
        {
          g_log.error("Unable to create file: " + filename);
          throw Exception::FileError("Unable to create file: " , filename);
        }

        // Write the column captions
        file << comment << "X";
        if (idx.empty())
            for(int spec=0;spec<nSpectra;spec++)
            {
                file << " , Y" << spec << " , E" << spec;
                if (write_dx) file << " , DX" << spec;
            }
        else
            for(std::set<int>::const_iterator spec=idx.begin();spec!=idx.end();++spec)
            {
                file << " , Y" << *spec << " , E" << *spec;
                if (write_dx) file << " , DX" << *spec;
            }
        file << '\n';

        bool isHistogram = ws->isHistogramData();

        // Set the number precision
        int prec = getProperty("Precision");
        if (prec != EMPTY_INT()) file.precision(prec);

        Progress progress(this,0,1,nBins);
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

            if (idx.empty())
                for(int spec=0;spec<nSpectra;spec++)
                {
                    file << sep;
                    file << ws->readY(spec)[bin];
                    file << sep;
                    file << ws->readE(spec)[bin];
                }
            else
                for(std::set<int>::const_iterator spec=idx.begin();spec!=idx.end();++spec)
                {
                    file << sep;
                    file << ws->readY(*spec)[bin]; 
                    file << sep;
                    file << ws->readE(*spec)[bin];
                }

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
            file << '\n';
            progress.report();
        }

    }

  } // namespace DataHandling
} // namespace Mantid
