//---------------------------------------------------
// Includes
//---------------------------------------------------
#include "MantidDataHandling/LoadGSS.h"
#include "MantidAPI/FileProperty.h"
#include "MantidAPI/WorkspaceValidators.h"
#include "MantidKernel/UnitFactory.h"
#include "MantidAPI/LoadAlgorithmFactory.h"
#include "MantidGeometry/Instrument/Instrument.h"
#include "MantidGeometry/Instrument/Detector.h"
#include "MantidGeometry/Instrument/CompAssembly.h"
#include "MantidGeometry/Instrument/Component.h"

#include <boost/math/special_functions/fpclassify.hpp>
#include <Poco/File.h>
#include <iostream>
#include <fstream>
#include <iomanip>

using namespace Mantid::DataHandling;
using namespace Mantid::API;
using namespace Mantid::Kernel;

namespace Mantid
{
  namespace DataHandling
  {

    // Register the algorithm into the AlgorithmFactory
    DECLARE_ALGORITHM(LoadGSS)

    //register the algorithm into loadalgorithm factory
    DECLARE_LOADALGORITHM(LoadGSS)

    /// Sets documentation strings for this algorithm
    void LoadGSS::initDocs()
    {
      this->setWikiSummary(
          "Loads a GSS file such as that saved by [[SaveGSS]]. This is not a lossless process, as SaveGSS truncates some data. There is no instrument assosciated with the resulting workspace.  '''Please Note''': Due to limitations of the GSS file format, the process of going from Mantid to a GSS file and back is not perfect. ");
      this->setOptionalMessage(
          "Loads a GSS file such as that saved by SaveGSS. This is not a lossless process, as SaveGSS truncates some data. There is no instrument assosciated with the resulting workspace.  'Please Note': Due to limitations of the GSS file format, the process of going from Mantid to a GSS file and back is not perfect.");
    }

    /**
     * Initialise the algorithm
     */
    void LoadGSS::init()
    {
      std::vector<std::string> exts;
      exts.push_back(".gsa");
      exts.push_back(".txt");
      declareProperty(new API::FileProperty("Filename", "", API::FileProperty::Load, exts),
          "The input filename of the stored data");
      declareProperty(new API::WorkspaceProperty<>("OutputWorkspace", "", Kernel::Direction::Output));
    }

    /**
     * Execute the algorithm
     */
    void LoadGSS::exec()
    {
      using namespace Mantid::API;
      std::string filename = getPropertyValue("Filename");

      std::vector<MantidVec*> gsasDataX;
      std::vector<MantidVec*> gsasDataY;
      std::vector<MantidVec*> gsasDataE;

      double primaryflightpath = -1;
      std::vector<double> twothetas;
      std::vector<double> difcs;
      std::vector<double> totalflightpaths;
      std::vector<int> detectorIDs;

      MantidVec* X = new MantidVec();
      MantidVec* Y = new MantidVec();
      MantidVec* E = new MantidVec();

      Progress* prog = NULL;

      char currentLine[256];
      std::string wsTitle;
      std::string slogTitle;
      std::string instrumentname = "Generic";
      bool slogtitleset = false;
      char filetype = 'x';

      int nSpec = 0;
      bool calslogx0 = true;
      double bc4 = 0;
      double bc3 = 0;

      bool db1 = true;

      bool multiplybybinwidth = false;

      std::ifstream input(filename.c_str(), std::ios_base::in);

      // Gather data
      if (input.is_open())
      {

        // 1. First line: Title
        if (!input.eof())
        {
          //    Get workspace title (should be first line or 2nd line for SLOG)
          input.getline(currentLine, 256);
          wsTitle = currentLine;
        } // if

        // 2. Loop all the lines
        while (!input.eof() && input.getline(currentLine, 256))
        {

          if (nSpec != 0 && prog == NULL)
          {
            prog = new Progress(this, 0.0, 1.0, nSpec);
          }
          if (!slogtitleset)
          {
            slogTitle = currentLine;
            slogtitleset = true;
          }
          double bc1 = 0;
          double bc2 = 0;

          if (currentLine[0] == '\n' || currentLine[0] == '#')
          {
            // Comment/Information Line

            std::string key1, key2;
            std::istringstream inputLine(currentLine, std::ios::in);
            inputLine.ignore(256, ' ');
            inputLine >> key1 >> key2;

            if (key2 == "Histograms")
            {
              // X Histograms
              nSpec = atoi(key1.c_str());
              g_log.debug() << "Histogram Line:  " << key1 << "  nSpec = " << nSpec << std::endl;
            }
            else if (key1 == "Instrument:")
            {
              // Instrument: XXXX
              instrumentname = key2;
              g_log.information() << "Instrument    :  " << key2 << std::endl;
            }
            else if (key1 == "with")
            {
              std::string s1;
              inputLine >> s1;
              if (s1 == "multiplied"){
                multiplybybinwidth = true;
                g_log.information() << "Y is multiplied by bin width" << std::endl;
              } else {
                g_log.debug() << "s1 = " << s1 << " is not allowed!\n";
              }
            }
            else if (key1 == "Primary")
            {
              // Primary flight path ...
              std::string s1, s2;
              inputLine >> s1 >> s2;
              primaryflightpath = convertToDouble(s2);
              g_log.information() << "L1 = " << primaryflightpath << std::endl;
            }
            else if (key1 == "Total")
            {
              // Total flight path .... ....
              std::string s1, s2, s3, s4, s5, s6;
              inputLine >> s1 >> s2 >> s3 >> s4 >> s5 >> s6;
              double totalpath = convertToDouble(s2);
              double tth = convertToDouble(s4);
              double difc = convertToDouble(s6);

              totalflightpaths.push_back(totalpath);
              twothetas.push_back(tth);
              difcs.push_back(difc);

              g_log.information() << "Total flight path = " << totalpath << "  2Theta = " << tth << "  DIFC = " << difc << std::endl;
            } // if keys....

          } // Line with #
          else if (currentLine[0] == 'B')
          {
            // Line start with Bank including file format, X0 information and etc.

            // 1. Save the previous to array and initialze new MantiVec for (X, Y, E)
            if (X->size() != 0)
            {
              gsasDataX.push_back(X);
              gsasDataY.push_back(Y);
              gsasDataE.push_back(E);
              X = new MantidVec();
              Y = new MantidVec();
              E = new MantidVec();

              if (prog != NULL)
                prog->report();
            }

            // 2. Parse

            /* BANK <SpectraNo> <NBins> <NBins> RALF <BC1> <BC2> <BC1> <BC4>
             *    OR,
             * BANK <SpectraNo> <NBins> <NBins> SLOG <BC1> <BC2> <BC3> 0>
             *  BC1 = X[0] * 32
             *  BC2 = X[1] * 32 - BC1
             *  BC4 = ( X[1] - X[0] ) / X[0]
             */

            // Parse B-line
            int specno, nbin1, nbin2;
            std::istringstream inputLine(currentLine, std::ios::in);

            /*
             inputLine.ignore(256, 'F');
             inputLine >> bc1 >> bc2 >> bc1 >> bc4;
             */

            inputLine.ignore(256, 'K');
            std::string filetypestring;

            inputLine >> specno >> nbin1 >> nbin2 >> filetypestring;
            g_log.debug() << "Bank: " << specno << "  filetypestring = " << filetypestring << std::endl;

            detectorIDs.push_back(specno);

            if (filetypestring[0] == 'S')
            {
              // SLOG
              filetype = 's';
              inputLine >> bc1 >> bc2 >> bc3 >> bc4;
            }
            else if (filetypestring[0] == 'R')
            {
              // RALF
              filetype = 'r';
              inputLine >> bc1 >> bc2 >> bc1 >> bc4;
            }
            else
            {
              std::cout << "Unsupported File Type: " << filetypestring << std::endl;
              std::cout << "Returned with error!\n";
              return;
            }

            // Determine x0
            double x0 = 0;
            if (filetype == 'r')
            {
              x0 = bc1 / 32;
              g_log.debug() << "RALF: x0 = " << x0 << "  bc4 = " << bc4 << std::endl;
              X->push_back(x0);
            }
            else
            {
              // Cannot calculate x0, turn on the flag
              calslogx0 = true;
            }
          } // Line with B
          else
          {
            double xValue;
            double yValue;
            double eValue;

            double xPrev;

            // * Get previous X value
            if (X->size() != 0)
            {
              xPrev = X->back();
            }
            else if (filetype == 'r')
            {
              // Except if RALF
              throw Mantid::Kernel::Exception::NotImplementedError(
                  "LoadGSS: File was not in expected format.");
            }
            else
            {
              xPrev = -0.0;
            }

            std::istringstream inputLine(currentLine, std::ios::in);
            inputLine >> xValue >> yValue >> eValue;

            // It is different for the definition of X, Y, Z in SLOG and RALF format
            if (filetype == 'r')
            {
              // RALF
              double tempy = yValue;

              xValue = (2 * xValue) - xPrev;
              yValue = yValue / (xPrev * bc4);
              eValue = eValue / (xPrev * bc4);

              if (db1)
              {
                g_log.debug() << "Type: " << filetype << "  xPrev = " << xPrev << " bc4 =" << bc4
                    << std::endl;
                g_log.debug() << "yValue = " << yValue << tempy << std::endl;
                db1 = false;
              }

            } // filetype == r
            else if (filetype == 's')
            {
              // SLOG
              if (calslogx0)
              {
                // calculation of x0 must use the x'[0]
                g_log.debug() << "x'_0 = " << xValue << "  bc3 = " << bc3 << std::endl;

                double x0 = 2 * xValue / (bc3 + 2.0);
                X->push_back(x0);
                xPrev = x0;
                g_log.debug() << "SLOG: x0 = " << x0 << std::endl;
                calslogx0 = false;
              }

              xValue = (2 * xValue) - xPrev;
              if (multiplybybinwidth)
              {
                yValue = yValue / (xValue - xPrev);
                eValue = eValue / (xValue - xPrev);
              }
              else
              {
                yValue = yValue;
                eValue = eValue;
              }
            } // file type == s

            // store read in data (x, y, e) to vector
            X->push_back(xValue);
            Y->push_back(yValue);
            E->push_back(eValue);
          } // Date Line
        } // while

        // Clean up after file is read through
        if (X->size() != 0)
        { // Put final spectra into data
          gsasDataX.push_back(X);
          gsasDataY.push_back(Y);
          gsasDataE.push_back(E);
        }
        input.close();
      } // if input is_open

      int nHist(static_cast<int> (gsasDataX.size()));
      int xWidth(static_cast<int> (X->size()));
      int yWidth(static_cast<int> (Y->size()));

      // 2. Create workspace & GSS Files data is always in TOF
      MatrixWorkspace_sptr outputWorkspace = boost::dynamic_pointer_cast<MatrixWorkspace>(
          WorkspaceFactory::Instance().create("Workspace2D", nHist, xWidth, yWidth));
      outputWorkspace->getAxis(0)->unit() = UnitFactory::Instance().create("TOF");

      // 2.1 Set workspace title
      if (filetype == 'r')
      {
        outputWorkspace->setTitle(wsTitle);
      }
      else
      {
        outputWorkspace->setTitle(slogTitle);
      }

      // 2.2 Put data from MatidVec's into outputWorkspace
      for (int i = 0; i < nHist; ++i)
      {
        // Move data across
        outputWorkspace->dataX(i) = *gsasDataX[i];
        outputWorkspace->dataY(i) = *gsasDataY[i];
        outputWorkspace->dataE(i) = *gsasDataE[i];
        // Clean up after copy
        delete gsasDataX[i];
        delete gsasDataY[i];
        delete gsasDataE[i];
      }

      // 2.3 Build instrument geometry
      createInstrumentGeometry(outputWorkspace, instrumentname, primaryflightpath,
          detectorIDs, totalflightpaths, twothetas);

      // Clean up
      delete prog;

      setProperty("OutputWorkspace", outputWorkspace);
      return;
    }

    // Convert a string containing number and unit to double
    double LoadGSS::convertToDouble(std::string inputstring)
    {

      std::string temps = "";
      int isize = (int)inputstring.size();
      for (int i = 0; i < isize; i++)
      {
        char thechar = inputstring[i];
        if ((thechar <= 'Z' && thechar >= 'A') || (thechar <= 'z' && thechar >= 'a'))
        {
          break;
        }
        else
        {
          temps += thechar;
        }
      }

      double rd = atof(temps.c_str());

      return rd;
    }

    /**This method does a quick file type check by checking the first 100 bytes of the file
     *  @param filePath- path of the file including name.
     *  @param nread :: no.of bytes read
     *  @param header :: The first 100 bytes of the file as a union
     *  @return true if the given file is of type which can be loaded by this algorithm
     */
    bool LoadGSS::quickFileCheck(const std::string& filePath, size_t nread, const file_header& header)
    {
      // check the file extension
      std::string extn = extension(filePath);
      bool bascii;
      if (extn.compare("gsa"))
        bascii = true;
      else if (extn.compare("txt"))
        bascii = true;
      else
        bascii = false;

      // check the bit of header
      bool is_ascii(true);
      for (size_t i = 0; i < nread; i++)
      {
        if (!isascii(header.full_hdr[i]))
          is_ascii = false;
      }
      return (is_ascii || bascii);
    }

    /**checks the file by opening it and reading few lines
     *  @param filePath :: name of the file including its path
     *  @return an integer value how much this algorithm can load the file
     */
    int LoadGSS::fileCheck(const std::string& filePath)
    {
      std::ifstream file(filePath.c_str());
      if (!file)
      {
        g_log.error("Unable to open file: " + filePath);
        throw Exception::FileError("Unable to open file: ", filePath);
      }
      std::string str;
      getline(file, str);//workspace title first line
      while (!file.eof())
      {
        getline(file, str);
        if (str.empty() || str[0] == '#')
        {
          continue;
        }
        if (!str.substr(0, 4).compare("BANK") && (str.find("RALF") != std::string::npos || str.find(
            "SLOG") != std::string::npos) && (str.find("FXYE") != std::string::npos))
        {
          return 80;
        }
        return 0;
      }
      return 0;

    }

    /* Create the instrument geometry with Instrument
     *
     */
    void LoadGSS::createInstrumentGeometry(MatrixWorkspace_sptr workspace, std::string instrumentname, double primaryflightpath,
        std::vector<int> detectorids, std::vector<double> totalflightpaths, std::vector<double> twothetas){

      // 0. Output information
      g_log.information() << "L1 = " << primaryflightpath << std::endl;
      if (detectorids.size() != totalflightpaths.size() || totalflightpaths.size() != twothetas.size()){
        g_log.debug() << "Input error!  cannot create geometry" << std::endl;
        g_log.information() << "Quit!" << std::endl;
        return;
      }
      for (size_t i = 0; i < detectorids.size(); i ++){
        g_log.information() << "Detector " << detectorids[i] << "  L1+L2 = " << totalflightpaths[i] << "  2Theta = " << twothetas[i] << std::endl;
      }

      // 1. Create a new instrument
      workspace->setInstrument(boost::shared_ptr<Geometry::Instrument>(new Geometry::Instrument));

      // 2. Get reference to Instrument and set its name
      boost::shared_ptr<Geometry::Instrument> instrument = workspace->getBaseInstrument();
      if (instrument.get() == 0)
      {
          g_log.error("Trying to use a Parametrized Instrument as an Instrument.");
          throw std::runtime_error("Trying to use a Parametrized Instrument as an Instrument.");
      }

      instrument->setName(instrumentname);

      // 3. Add dummy source and samplepos to instrument
      Geometry::ObjComponent *samplepos = new Geometry::ObjComponent("Sample",instrument.get());
      instrument->add(samplepos);
      instrument->markAsSamplePos(samplepos);
      samplepos->setPos(0.0,0.0,0.0);

      Geometry::ObjComponent *source = new Geometry::ObjComponent("Source",instrument.get());
      instrument->add(source);
      instrument->markAsSource(source);

      double l1 = primaryflightpath;
      source->setPos(0.0,0.0,-1.0*l1);

      // 4. add detectors
      // The L2 and 2-theta values from Raw file assumed to be relative to sample position
      const int numDetector = (int)detectorids.size();    // number of detectors
      std::vector<int> detID = detectorids;    // detector IDs
      std::vector<double> angle = twothetas;  // angle between indicent beam and direction from sample to detector (two-theta)

      for (int i = 0; i < numDetector; ++i)
      {
        // Create a new detector. Instrument will take ownership of pointer so no need to delete.
        Geometry::Detector *detector = new Geometry::Detector("det",detID[i],samplepos);
        Kernel::V3D pos;

        // FIXME : need to confirm the definition
        double r = totalflightpaths[i] - l1;
        pos.spherical(r, angle[i], 0.0 );

        detector->setPos(pos);

        // add copy to instrument and mark it
        instrument->add(detector);
        instrument->markAsDetector(detector);
      }

      /*
      // Now mark the up the monitors
      const int numMonitors = iraw.i_mon;     // The number of monitors
      const int* const monIndex = iraw.mdet;  // Index into the udet array for each monitor

      for (int j = 0; j < numMonitors; ++j)
      {
        const int detectorToMark = detID[monIndex[j]-1];
        boost::shared_ptr<Geometry::IDetector> det = instrument->getDetector(detectorToMark);
      instrument->markAsMonitor(det.get());
        g_log.information() << "Detector with ID " << detectorToMark << " marked as a monitor." << std::endl;
      }
      std::vector<detid_t> monitorList=instrument->getMonitors();
      setProperty("MonitorList",monitorList);
      // Information to the user about what info is extracted from raw file
      g_log.information() << "SamplePos component added with position set to (0,0,0).\n"
        << "Detector components added with position coordinates assumed to be relative to the position of the sample; \n"
        << "L2 and two-theta values were read from raw file and used to set the r and theta spherical coordinates; \n"
        << "the remaining spherical coordinate phi was set to zero.\n"
        << "Source component added with position set to (0,0,-" << l1 << "). In standard configuration, with \n"
        << "the beam along z-axis pointing from source to sample, this implies the source is " << l1 << "m in front \n"
        << "of the sample. This value can be changed via the 'instrument.l1' configuration property.\n";
      */

      return;


      return;
    }

  }//namespace
}//namespace
