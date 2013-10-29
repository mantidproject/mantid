/*WIKI* 

Saves a focused data set into a three column GSAS format containing X_i, Y_i*step, and E_I*step. Exclusively for the crystallography package [http://www.ccp14.ac.uk/solution/gsas/index.html GSAS] and data needs to be in time-of-flight. For data where the focusing routine has generated several spectra (for example, multi-bank instruments), the option is provided for saving all spectra into a single file, separated by headers, or into several files that will be named "workspaceName_"+workspace_index_number.

From the GSAS manual a description of the format options:
* If BINTYP is 'SLOG' then the neutron TOF data was collected in constant ∆T/T steps. BCOEF(1) is the initial TOF in μsec, and BCOEF(3) is the value of ∆T/T used in the data collection. BCOEF(2) is a maximum TOF for the data set. BCOEF(4) is zero and ignored.
* If BINTYP equals 'RALF' then the data was collected at one of the TOF neutron diffractometers at the ISIS Facility, Rutherford-Appleton Laboratory. The width of the time bins is constant for a section of the data at small values of TOF and then varies (irregularly) in pseudoconstant ∆T/T steps. In this case BCOEF(1) is the starting TOF in μsec*32, BCOEF(2) is the width of the first step in μsec*32, BCOEF(3) is the start of the log scaled step portion of the data in μsec*32 and BCOEF(4) is the resolution to be used in approximating the size of each step beyond BCOEF(3).

The format is limited to saving 99 spectra in total. Trying to save more will generate an error.


*WIKI*/
//---------------------------------------------------
// Includes
//---------------------------------------------------
#include "MantidDataHandling/SaveGSS.h"
#include "MantidAPI/FileProperty.h"
#include "MantidAPI/WorkspaceValidators.h"
#include "MantidAPI/AlgorithmHistory.h"
#include "MantidKernel/TimeSeriesProperty.h"
#include "MantidKernel/ListValidator.h"
#include <boost/math/special_functions/fpclassify.hpp>
#include <Poco/File.h>
#include <Poco/Path.h>
#include <fstream>
#include <iomanip>

namespace Mantid
{
  namespace DataHandling
  {

    using namespace API;

    // Register the algorithm into the AlgorithmFactory
    DECLARE_ALGORITHM(SaveGSS)

    /// Sets documentation strings for this algorithm
    void SaveGSS::initDocs()
    {
      this->setWikiSummary("Saves a focused data set into a three column GSAS format. ");
      this->setOptionalMessage("Saves a focused data set into a three column GSAS format.");
    }

    const std::string RALF("RALF");
    const std::string SLOG("SLOG");

    //---------------------------------------------------
    // Private member functions
    //---------------------------------------------------
    /**
     * Initialise the algorithm
     */
    void SaveGSS::init()
    {
      // Data must be in TOF
      declareProperty(new API::WorkspaceProperty<>("InputWorkspace", "", Kernel::Direction::Input,
                                                   boost::make_shared<API::WorkspaceUnitValidator>("TOF")),
          "The input workspace, which must be in time-of-flight");
      declareProperty(new API::FileProperty("Filename", "", API::FileProperty::Save),
          "The filename to use for the saved data");
      declareProperty("SplitFiles", true,
          "Whether to save each spectrum into a separate file ('true') or not ('false'). Note that this is a string, not a boolean property.");
      declareProperty("Append", true, "If true and Filename already exists, append, else overwrite ");
      declareProperty(
          "Bank",
          1,
          "The bank number to include in the file header for the first spectrum. This will increment for each spectrum or group member.");
      std::vector<std::string> formats;
      formats.push_back(RALF);
      formats.push_back(SLOG);
      declareProperty("Format", RALF, boost::make_shared<Kernel::StringListValidator>(formats), "GSAS format to save as");
      declareProperty("MultiplyByBinWidth", true,
          "Multiply the intensity (Y) by the bin width; default TRUE.");
      declareProperty("ExtendedHeader", false, "Add information to the header about iparm file and normalization");

      declareProperty("UseSpectrumNumberAsBankID", false, "If true, then each bank's bank ID is equal to the spectrum number; "
                      "otherwise, the continous bank IDs are applied. ");
    }

    /**
     * Determine the focused position for the supplied spectrum. The position
     * (l1, l2, tth) is returned via the references passed in.
     */
    void getFocusedPos(MatrixWorkspace_const_sptr wksp, const int spectrum, double &l1, double &l2,
        double &tth)
    {
      Geometry::Instrument_const_sptr instrument = wksp->getInstrument();
      if (instrument == NULL)
      {
        l1 = 0.;
        l2 = 0.;
        tth = 0.;
        return;
      }
      Geometry::IObjComponent_const_sptr source = instrument->getSource();
      Geometry::IObjComponent_const_sptr sample = instrument->getSample();
      if (source == NULL || sample == NULL)
      {
        l1 = 0.;
        l2 = 0.;
        tth = 0.;
        return;
      }
      l1 = source->getDistance(*sample);
      Geometry::IDetector_const_sptr det = wksp->getDetector(spectrum);
      l2 = det->getDistance(*sample);
      tth = wksp->detectorTwoTheta(det);
    }

    /**
     * Execute the algorithm
     */
    void SaveGSS::exec()
    {
      using namespace Mantid::API;
      //Retrieve the input workspace
      MatrixWorkspace_const_sptr inputWS = getProperty("InputWorkspace");
      const int nHist = static_cast<int> (inputWS->getNumberHistograms());

      // Check the number of histogram/spectra < 99
      if (nHist > 99)
      {
        g_log.error() << "Number of Spectra ("<< nHist<<") cannot be larger than 99 for GSAS file" << std::endl;
        throw new std::invalid_argument("Workspace has more than the 99 spectra, allowed by GSAS");
      }

      std::string filename = getProperty("Filename");

      const int bank = getProperty("Bank");
      const bool MultiplyByBinWidth = getProperty("MultiplyByBinWidth");
      const bool split = getProperty("SplitFiles");
      std::string outputFormat = getProperty("Format");

      m_useSpecAsBank = getProperty("UseSpectrumNumberAsBankID");

      std::ostringstream number;
      std::ofstream out;
      // Check whether to append to an already existing file or overwrite
      const bool append = getProperty("Append");
      using std::ios_base;
      ios_base::openmode mode = (append ? (ios_base::out | ios_base::app) : ios_base::out);
      Progress p(this, 0.0, 1.0, nHist);
      double l1, l2, tth;
      Geometry::Instrument_const_sptr instrument = inputWS->getInstrument();
      Geometry::IObjComponent_const_sptr source;
      Geometry::IObjComponent_const_sptr sample;
      if (instrument != NULL)
      {
        source = instrument->getSource();
        sample = instrument->getSample();
      }

      // Write GSAS file for each histogram (spectrum)
      for (int i = 0; i < nHist; i++)
      {
        if (instrument != NULL)
        {
          if (source != NULL && sample != NULL)
          {
            Geometry::IDetector_const_sptr det = inputWS->getDetector(static_cast<size_t> (i));
            if (det->isMasked())
              continue;
          }
        }
        getFocusedPos(inputWS, i, l1, l2, tth);
        g_log.information() << "L1 = " << l1 << "  L2 = " << l2 << "  2theta = " << tth << std::endl;

        // Write Header
        // If NOT split, only write to first histogram
        // If     split, write to each histogram
        if (!split && i == 0) // Assign only one file
        {
          const std::string file(filename);
          Poco::File fileobj(file);
          const bool exists = fileobj.exists();
          out.open(file.c_str(), mode);
          if (!exists || !append)
            writeHeaders(outputFormat, out, inputWS, l1);

        }
        else if (split)//Several files will be created with names: filename-i.ext
        {
          // New number
          number << "-" << i;

          Poco::Path path(filename);
          std::string basename = path.getBaseName(); // Filename minus extension
          std::string ext = path.getExtension();
          // Chop off filename
          path.makeParent();
          path.append(basename + number.str() + "." + ext);
          Poco::File fileobj(path);
          const bool exists = fileobj.exists();
          out.open(path.toString().c_str(), mode);
          number.str("");
          if (!exists || !append)
            writeHeaders(outputFormat, out, inputWS, l1);
        }

        { // New scope
          if (!out.is_open())
          {
            throw std::runtime_error("Could not open filename: " + filename);
          }

          if (l1 != 0. || l2 != 0. || tth != 0.)
          {
            out << "# Total flight path " << (l1 + l2) << "m, tth " << (tth * 180. / M_PI)
                << "deg, DIFC " << ((2.0 * PhysicalConstants::NeutronMass * sin(tth / 2.0) * (l1 + l2))
                / (PhysicalConstants::h * 1e4)) << "\n";
          }
          out << "# Data for spectrum :" << i << std::endl;

          int bankid;
          if (m_useSpecAsBank)
          {
            bankid = static_cast<int>(inputWS->getSpectrum(i)->getSpectrumNo());
          }
          else
          {
            bankid = bank + i;
          }

          if (RALF.compare(outputFormat) == 0)
          {
            this->writeRALFdata(bankid, MultiplyByBinWidth, out, inputWS->readX(i), inputWS->readY(i),
                inputWS->readE(i));
          }
          else if (SLOG.compare(outputFormat) == 0)
          {
            this->writeSLOGdata(bankid, MultiplyByBinWidth, out, inputWS->readX(i), inputWS->readY(i),
                inputWS->readE(i));
          }
          else
          {
            throw std::runtime_error("Cannot write to the unknown " +  outputFormat + "output format"); 
          }

        } // End separate scope

        //Close at each iteration
        if (split)
        {
          out.close();
        }
        p.report();
      } // for nHist

      // Close if single file
      if (!split)
      {
        out.close();
      }
      return;
    }

    /** Ensures that when a workspace group is passed as output to this workspace
     *  everything is saved to one file and the bank number increments for each
     *  group member.
     *  @param alg ::           Pointer to the algorithm
     *  @param propertyName ::  Name of the property
     *  @param propertyValue :: Value  of the property
     *  @param periodNum ::     Effectively a counter through the group members
     */
    void SaveGSS::setOtherProperties(IAlgorithm* alg, const std::string& propertyName,
        const std::string& propertyValue, int periodNum)
    {
      // We want to append subsequent group members to the first one
      if (propertyName == "Append")
      {
        if (periodNum != 1)
        {
          alg->setPropertyValue(propertyName, "1");
        }
        else
          alg->setPropertyValue(propertyName, propertyValue);
      }
      // We want the bank number to increment for each member of the group
      else if (propertyName == "Bank")
      {
        alg->setProperty("Bank", atoi(propertyValue.c_str()) + periodNum - 1);
      }
      else
        Algorithm::setOtherProperties(alg, propertyName, propertyValue, periodNum);
    }

    void writeValue(std::ostream &os, const Run& runinfo, const std::string& name, const std::string& defValue="UNKNOWN")
    {
      if (!runinfo.hasProperty(name))
      {
        os << defValue;
        return;
      }
      Kernel::Property* prop = runinfo.getProperty(name);
      if (prop == NULL)
      {
        os << defValue;
        return;
      }
      Kernel::TimeSeriesProperty<double> *log =
          dynamic_cast<Kernel::TimeSeriesProperty<double> *> (prop);
      if (log != NULL)
      {
        os << log->getStatistics().mean;
      }
      else
      {
        os << prop->value();
      }
      std::string units = prop->units();
      if (!units.empty())
        os << " " << units;
    }

    /**
     * Write the header information for the given workspace
     * @param format :: The string containing the header formatting
     * @param os :: The stream to use to write the information
     * @param workspace :: A shared pointer to MatrixWorkspace
     * @param primaryflightpath :: Value for the moderator to sample distance
     */
    void SaveGSS::writeHeaders(const std::string &format, std::ostream& os,
        Mantid::API::MatrixWorkspace_const_sptr& workspace, double primaryflightpath) const
    {
      const Run& runinfo = workspace->run();
      if (format.compare(SLOG) == 0)
      {
        os << "Sample Run: ";
        writeValue(os, runinfo, "run_number");
        os << " Vanadium Run: ";
        writeValue(os, runinfo, "van_number");
        os << " Wavelength: ";
        writeValue(os, runinfo, "LambdaRequest");
        os << "\n";
      }

      if (this->getProperty("ExtendedHeader"))
      {
        // the instrument parameter file
        if (runinfo.hasProperty("iparm_file"))
        {
          Kernel::Property* prop = runinfo.getProperty("iparm_file");
          if (prop != NULL && (!prop->value().empty()))
          {
            std::stringstream line;
            line << "#Instrument parameter file: "
                 << prop->value();
            os << std::setw(80) << std::left << line.str() << "\n";
          }
        }

        // write out the gsas monitor counts
        os << "Monitor: ";
        if (runinfo.hasProperty("gsas_monitor"))
        {
          writeValue(os, runinfo, "gsas_monitor");
        }
        else
        {
          writeValue(os, runinfo, "gd_prtn_chrg", "1");
        }
        os << "\n";
      }

      if (format.compare(SLOG) == 0)
      {
        os << "# "; // make the next line a comment
      }
      os << workspace->getTitle() << "\n";
      os << "# " << workspace->getNumberHistograms() << " Histograms\n";
      os << "# File generated by Mantid:\n";
      os << "# Instrument: " << workspace->getInstrument()->getName() << "\n";
      os << "# From workspace named : " << workspace->getName() << "\n";
      if (getProperty("MultiplyByBinWidth"))
        os << "# with Y multiplied by the bin widths.\n";
      os << "# Primary flight path " << primaryflightpath << "m \n";
      if (format.compare(SLOG) == 0)
      {
        os << "# Sample Temperature: ";
        writeValue(os, runinfo, "SampleTemp");
        os << " Freq: ";
        writeValue(os, runinfo, "SpeedRequest1");
        os << " Guide: ";
        writeValue(os, runinfo, "guide");
        os << "\n";

        // print whether it is normalized by monitor or pcharge
        bool norm_by_current = false;
        bool norm_by_monitor = false;
        const WorkspaceHistory::AlgorithmHistories& algohist = workspace->getHistory().getAlgorithmHistories();
        for (WorkspaceHistory::AlgorithmHistories::const_iterator it = algohist.begin(); it != algohist.end(); ++it)
        {
          if (it->name().compare("NormaliseByCurrent") == 0)
            norm_by_current = true;
          if (it->name().compare("NormaliseToMonitor") == 0)
            norm_by_monitor = true;
        }
        os << "#";
        if (norm_by_current)
          os << " Normalised to pCharge";
        if (norm_by_monitor)
          os << " Normalised to monitor";
        os << "\n";
      }

      return;
    }

    inline void writeBankLine(std::ostream& out, const std::string& bintype, const int banknum,
        const size_t datasize)
    {
      out << "BANK " << std::fixed << std::setprecision(0) << banknum // First bank should be 1 for GSAS; this can be changed
          << std::fixed << " " << datasize << std::fixed << " " << datasize << std::fixed << " "
          << bintype;
    }

    inline double fixErrorValue(const double value)
    {
      if (value <= 0. || boost::math::isnan(value) || boost::math::isinf(value)) //Negative errors cannot be read by GSAS
        return 0.;
      else
        return value;
    }

    void SaveGSS::writeRALFdata(const int bank, const bool MultiplyByBinWidth, std::ostream& out,
        const MantidVec& X, const MantidVec& Y, const MantidVec& E) const
    {
      const size_t datasize = Y.size();
      double bc1 = X[0] * 32;
      double bc2 = (X[1] - X[0]) * 32;
      // Logarithmic step
      double bc4 = (X[1] - X[0]) / X[0];
      if (boost::math::isnan(fabs(bc4)) || boost::math::isinf(bc4))
        bc4 = 0; //If X is zero for BANK

      //Write out the data header
      writeBankLine(out, "RALF", bank, datasize);
      out << std::fixed << " " << std::setprecision(0) << std::setw(8) << bc1 << std::fixed << " "
          << std::setprecision(0) << std::setw(8) << bc2 << std::fixed << " " << std::setprecision(0)
          << std::setw(8) << bc1 << std::fixed << " " << std::setprecision(5) << std::setw(7) << bc4
          << " FXYE" << std::endl;

      //Do each Y entry
      for (size_t j = 0; j < datasize; j++)
      {
        //Calculate the error
        double Epos;
        if (MultiplyByBinWidth)
          Epos = E[j] * (X[j + 1] - X[j]); // E[j]*X[j]*bc4;
        else
          Epos = E[j];
        Epos = fixErrorValue(Epos);

        //The center of the X bin.
        out << std::fixed << std::setprecision(5) << std::setw(15) << 0.5 * (X[j] + X[j + 1]);

        // The Y value
        if (MultiplyByBinWidth)
          out << std::fixed << std::setprecision(8) << std::setw(18) << Y[j] * (X[j + 1] - X[j]);
        else
          out << std::fixed << std::setprecision(8) << std::setw(18) << Y[j];

        //The error
        out << std::fixed << std::setprecision(8) << std::setw(18) << Epos << "\n";
      }
    }

    void SaveGSS::writeSLOGdata(const int bank, const bool MultiplyByBinWidth, std::ostream& out,
        const MantidVec& X, const MantidVec& Y, const MantidVec& E) const
    {

      g_log.debug() << "SaveGSS(): MultipyByBinwidth = " << MultiplyByBinWidth << std::endl;

      const size_t datasize = Y.size();
      double bc1 = X.front(); // minimum TOF in microseconds
      if (bc1 <= 0.)
      {
        throw std::runtime_error("Cannot write out logarithmic data starting at zero");
      }
      double bc2 = 0.5 * (*(X.rbegin()) + *(X.rbegin()+ 1)); // maximum TOF (in microseconds?)
      double bc3 = (*(X.begin() + 1) - bc1) / bc1; // deltaT/T

      g_log.debug() << "SaveGSS(): Min TOF = " << bc1 << std::endl;

      writeBankLine(out, "SLOG", bank, datasize);
      out << std::fixed << " " << std::setprecision(0) << std::setw(10) << bc1 << std::fixed << " "
          << std::setprecision(0) << std::setw(10) << bc2 << std::fixed << " " << std::setprecision(7)
          << std::setw(10) << bc3 << std::fixed << " 0 FXYE" << std::endl;

      double delta, y, e;

      for (size_t i = 0; i < datasize; i++)
      {
        y = Y[i];
        e = E[i];
        if (MultiplyByBinWidth)
        {
          // Multiple by bin width as
          delta = X[i + 1] - X[i];
          y *= delta;
          e *= delta;
        }
        e = fixErrorValue(e);

        out << "  " << std::fixed << std::setprecision(9) << std::setw(20) << 0.5 * (X[i] + X[i + 1])
            << "  " << std::fixed << std::setprecision(9) << std::setw(20) << y << "  " << std::fixed
            << std::setprecision(9) << std::setw(20) << e << std::setw(12) << " " << "\n"; // let it flush its own buffer
      }
      out << std::flush;
    }

  } // namespace DataHandling
} // namespace Mantid
