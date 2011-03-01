//----------------------------------------------------------------------
// Includes
//----------------------------------------------------------------------
#include "MantidDataHandling/SaveAscii.h"
#include "MantidKernel/UnitFactory.h"
#include "MantidKernel/ArrayProperty.h"
#include "MantidAPI/FileProperty.h"

#include <set>
#include <fstream>
#include <boost/tokenizer.hpp>

namespace Mantid
{
  namespace DataHandling
  {
    // Register the algorithm into the algorithm factory
    DECLARE_ALGORITHM(SaveAscii)

    using namespace Kernel;
    using namespace API;

    // Initialise the logger
    Logger& SaveAscii::g_log = Logger::get("SaveAscii");

    /// Initialisation method.
    void SaveAscii::init()
    {
      //this->setWikiSummary("Saves a 2D [[workspace]] to a Ascii file.");
      //this->setOptionalMessage("Saves a 2D workspace to a Ascii file.");

      std::vector<std::string> exts;
      exts.push_back(".dat");
      exts.push_back(".txt");
      exts.push_back(".csv");
      declareProperty(new FileProperty("Filename", "", FileProperty::Save, exts),
		      "A comma separated Ascii file that will be created");
      declareProperty(new WorkspaceProperty<>("Workspace",
        "",Direction::Input), "The name of the workspace that will be saved.");

      BoundedValidator<int> *mustBePositive = new BoundedValidator<int>();
      mustBePositive->setLower(1);
      declareProperty("WorkspaceIndexMin", 1, mustBePositive);
      declareProperty("WorkspaceIndexMax", EMPTY_INT(), mustBePositive->clone());
      declareProperty(new ArrayProperty<int>("SpectrumList"));
      declareProperty("Precision", EMPTY_INT(), mustBePositive->clone());
    }

    /** 
     *   Executes the algorithm.
     */
    void SaveAscii::exec()
    {
        // Get the workspace
        MatrixWorkspace_const_sptr ws = getProperty("Workspace");
        int nSpectra = ws->getNumberHistograms();
        int nBins = ws->blocksize();

        // Get the properties
        std::vector<int> spec_list = getProperty("SpectrumList");
        int spec_min = getProperty("WorkspaceIndexMin");
        int spec_max = getProperty("WorkspaceIndexMax");

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

        if (!idx.empty()) nSpectra = idx.size();

        if (nBins == 0 || nSpectra == 0) throw std::runtime_error("Trying to save an empty workspace");

        std::string filename = getProperty("Filename");
        std::ofstream file(filename.c_str());

        if (!file)
        {
          g_log.error("Unable to create file: " + filename);
          throw Exception::FileError("Unable to create file: " , filename);
        }

        // Write the column captions
        file << "X";
        if (idx.empty())
            for(int spec=0;spec<nSpectra;spec++)
            {
                file << " , Y" << spec << " , E" << spec;
            }
        else
            for(std::set<int>::const_iterator spec=idx.begin();spec!=idx.end();spec++)
            {
                file << " , Y" << *spec << " , E" << *spec;
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
                    file << " , " << ws->readY(spec)[bin] << " , " << ws->readE(spec)[bin];
                }
            else
                for(std::set<int>::const_iterator spec=idx.begin();spec!=idx.end();spec++)
                {
                    file << " , " << ws->readY(*spec)[bin] << " , " << ws->readE(*spec)[bin];
                }
            file << '\n';
            progress.report();
        }

    }

  } // namespace DataHandling
} // namespace Mantid
