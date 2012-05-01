/*WIKI* 

 This algorithm outputs the data in ASCII as a 3 column X, Y ,E format for use in subsequent analysis by other programs.  The output files can be read for example into FullProf with format instrument=10.

 For data where the focusing routine has generated several spectra (for example, multi-bank instruments),
 the option is provided for saving all spectra into a single file, separated by headers, or into
 several files that will be named "workspaceName-"+spectra_number

 == Current Issues ==
 Fullprof expects the data to be in TOF, however at present the [[DiffractionFocussing]] algorithm in Mantid leaves the data in d-spacing.

 If the written file is to be loaded into TOPAS, then headers should be omitted (set the IncludeHeader property to false);


 *WIKI*/
//---------------------------------------------------
// Includes
//---------------------------------------------------
#include "MantidDataHandling/SaveFocusedXYE.h"
#include "MantidAPI/FileProperty.h"
#include "MantidKernel/ListValidator.h"
#include <Poco/File.h>
#include <fstream>
#include <iomanip>

using namespace Mantid::DataHandling;

// Register the algorithm into the AlgorithmFactory
DECLARE_ALGORITHM(SaveFocusedXYE)

/// Sets documentation strings for this algorithm
void SaveFocusedXYE::initDocs()
{
  this->setWikiSummary(
      "Saves a focused data set (usually the output of a diffraction focusing routine but not exclusively) into a three column format containing X_i, Y_i, and E_i. ");
  this->setOptionalMessage(
      "Saves a focused data set (usually the output of a diffraction focusing routine but not exclusively) into a three column format containing X_i, Y_i, and E_i.");
}

//---------------------------------------------------
// Private member functions
//---------------------------------------------------
/**
 * Initialise the algorithm
 */
void SaveFocusedXYE::init()
{
  declareProperty(new API::WorkspaceProperty<>("InputWorkspace", "", Kernel::Direction::Input),
      "The name of the workspace containing the data you wish to save");
  declareProperty(new API::FileProperty("Filename", "", API::FileProperty::Save),
      "The filename to use when saving data");
  std::vector<std::string> Split(2);
  Split[0] = "True";
  Split[1] = "False";
  declareProperty("SplitFiles", "True", boost::make_shared<Kernel::StringListValidator>(Split),
      "Save each spectrum in a different file (default true)");
  declareProperty("StartAtBankNumber", 0, "Start bank (spectrum) numbers at this number in the file.  "
    "The bank number in the file will be the workspace index + StartAtBankNumber.");
  declareProperty("Append", false, "If true and Filename already exists, append, else overwrite");
  declareProperty("IncludeHeader", true, "Whether to include the header lines (default: true)");
}

/**
 * Execute the algorithm
 */
void SaveFocusedXYE::exec()
{
  using namespace Mantid::API;
  //Retrieve the input workspace
  MatrixWorkspace_const_sptr inputWS = getProperty("InputWorkspace");
  const size_t nHist = inputWS->getNumberHistograms();
  const bool isHistogram = inputWS->isHistogramData();
  std::string filename = getProperty("Filename");

  std::size_t pos = filename.find_first_of(".");
  std::string ext;
  if (pos != std::string::npos) //Remove the extension
  {
    ext = filename.substr(pos + 1, filename.npos);
    filename = filename.substr(0, pos);
  }
  const bool append = getProperty("Append");
  const bool headers = getProperty("IncludeHeader");

  int startingbank = getProperty("StartAtBankNumber");
  if (startingbank < 0)
  {
    g_log.error() << "Starting bank number cannot be less than 0. " << std::endl;
    throw std::invalid_argument("Incorrect starting bank number");
  }

  std::string split = getProperty("SplitFiles");
  std::ostringstream number;
  std::fstream out;
  using std::ios_base;
  ios_base::openmode mode = (append ? (ios_base::out | ios_base::app) : ios_base::out);

  Progress progress(this, 0.0, 1.0, nHist);
  for (size_t i = 0; i < nHist; i++)
  {
    const MantidVec& X = inputWS->readX(i);
    const MantidVec& Y = inputWS->readY(i);
    const MantidVec& E = inputWS->readE(i);
    if (split == "False" && i == 0) // Assign only one file
    {
      const std::string file(filename + '.' + ext);
      Poco::File fileObj(file);
      const bool exists = fileObj.exists();
      out.open(file.c_str(), mode);
      if (headers && (!exists || !append))
        writeHeaders(out, inputWS);
    }
    else if (split == "True")//Several files will be created with names: filename-i.ext
    {
      number << "-" << i + startingbank;
      const std::string file(filename + number.str() + "." + ext);
      Poco::File fileObj(file);
      const bool exists = fileObj.exists();
      out.open(file.c_str(), mode);
      number.str("");
      if (headers && (!exists || !append))
        writeHeaders(out, inputWS);
    }

    { // New scope
      if (!out.is_open())
      {
        g_log.information("Could not open filename: " + filename);
        throw std::runtime_error("Could not open filename: " + filename);
      }
      if (headers)
      {
        out << "# Data for spectra :" << i + startingbank << std::endl;
        out << "# " << inputWS->getAxis(0)->unit()->caption() << "              Y                 E"
            << std::endl;
      }
      const size_t datasize = Y.size();
      for (size_t j = 0; j < datasize; j++)
      {
        double xvalue(0.0);
        if (isHistogram)
        {
          xvalue = (X[j] + X[j + 1]) / 2.0;
        }
        else
        {
          xvalue = X[j];
        }
        out << std::fixed << std::setprecision(5) << std::setw(15) << xvalue << std::fixed
            << std::setprecision(8) << std::setw(18) << Y[j] << std::fixed << std::setprecision(8)
            << std::setw(18) << E[j] << "\n";
      }
    } // End separate scope
    //Close at each iteration
    if (split == "True")
    {
      out.close();
    }
    progress.report();
  }
  // Close if single file
  if (split == "False")
  {
    out.close();
  }
  return;
}

/** virtual method to set the non workspace properties for this algorithm
 *  @param alg :: pointer to the algorithm
 *  @param propertyName :: name of the property
 *  @param propertyValue :: value  of the property
 *  @param perioidNum :: period number
 */
void SaveFocusedXYE::setOtherProperties(IAlgorithm* alg, const std::string& propertyName,
    const std::string& propertyValue, int perioidNum)
{
  if (!propertyName.compare("Append"))
  {
    if (perioidNum != 1)
    {
      alg->setPropertyValue(propertyName, "1");
    }
    else
      alg->setPropertyValue(propertyName, propertyValue);
  }
  else
    Algorithm::setOtherProperties(alg, propertyName, propertyValue, perioidNum);
}

/**
 * Write the header information for the given workspace
 * @param os :: The stream to use to write the information
 * @param workspace :: A shared pointer to MatrixWorkspace
 */
void SaveFocusedXYE::writeHeaders(std::ostream& os, Mantid::API::MatrixWorkspace_const_sptr& workspace) const
{
  os << "# File generated by Mantid:" << std::endl;
  os << "# Instrument: " << workspace->getInstrument()->getName() << std::endl;
  os << "# The X-axis unit is: " << workspace->getAxis(0)->unit()->caption() << std::endl;
  os << "# The Y-axis unit is: " << workspace->YUnitLabel() << std::endl;

  return;
}

