/*WIKI*

Fullprof's resolution file contains the peak profile parameters and some powder diffractometer's geometry related parameters in a certain format.  
This algorithm reads a TableWorkspace containing all the information required by Fullprof's resolution file, and
write out a text file conforming to that resolution file's format.  

== Peak Profile Supported ==
Here is the list of peak profile supported by this algorithm:
* Thermal Neutron Back-to-back Exponential Convoluted with Pseudo-voigt peak profile.

== Instrument Profile Parameter TableWorkspace  ==
TableWorkspace as the input of this algorithm can be generated from ''CreateLeBailFitInput'', ''RefinePowderInstrumentParameters'' or ''LeBailFit''.  
To be noticed that the TableWorkspace output from ''RefinePowderInstrumentParameters'' is usually an intermediate product. 

Input TableWorkspace must have two columns, "Name" and "Value", as column 0 and 1.  There is no restriction on other columns. 

== How to use algorithm with other algorithms ==
This algorithm is designed to work with other algorithms to do Le Bail fit.  The introduction can be found in [[Le Bail Fit]].

*WIKI*/

#include "MantidDataHandling/SaveFullprofResolution.h"
#include "MantidAPI/WorkspaceProperty.h"
#include "MantidAPI/FileProperty.h"
#include "MantidAPI/TableRow.h"
#include "MantidKernel/ListValidator.h"
#include "MantidKernel/BoundedValidator.h"

#include <boost/algorithm/string.hpp>
#include <Poco/File.h>

#include <iomanip>
#include <fstream>

using namespace Mantid;
using namespace Mantid::API;
using namespace Mantid::Kernel;
using namespace Mantid::DataObjects;
using namespace std;

namespace Mantid
{
namespace DataHandling
{

  DECLARE_ALGORITHM(SaveFullprofResolution)

  //----------------------------------------------------------------------------------------------
  /** Constructor
   */
  SaveFullprofResolution::SaveFullprofResolution()
  {
  }

    
  //----------------------------------------------------------------------------------------------
  /** Destructor
   */
  SaveFullprofResolution::~SaveFullprofResolution()
  {
  }

  //----------------------------------------------------------------------------------------------
  /** Wiki docs
    */
  void SaveFullprofResolution::initDocs()
  {
    this->setWikiSummary("Save a Table workspace, which contains peak profile parameters' values, to a Fullprof resolution (.irf) file.");
    this->setOptionalMessage("Save a Table workspace, which contains peak profile parameters' values, to a Fullprof resolution (.irf) file.");
  }

  //----------------------------------------------------------------------------------------------
  /** Init to define parameters
    */
  void SaveFullprofResolution::init()
  {
    this->declareProperty(new WorkspaceProperty<TableWorkspace>("InputWorkspace", "Anonymous", Direction::Input),
                          "Input TableWorkspace containing the parameters for .irf file.");

    std::vector<std::string> exts;
    exts.push_back(".irf");
    this->declareProperty(new API::FileProperty("OutputFilename", "fp.irf", API::FileProperty::Save, exts),
                          "Name of the output .irf file.");

    boost::shared_ptr<BoundedValidator<int> > bankboundval = boost::make_shared<BoundedValidator<int> >();
    bankboundval->setLower(0);
    this->declareProperty("Bank", EMPTY_DBL(), "Bank number of the parameters belonged to. ");

    vector<string> supportedfunctions;
    supportedfunctions.push_back("Back-to-back exponential convoluted with pseudo-voigt (profile 9)");
    supportedfunctions.push_back("Jason Hodge's function (profile 10)");
    auto funcvalidator = boost::make_shared<StringListValidator>(supportedfunctions);
    declareProperty("ProfileFunction", "Jason Hodge's function (profile 10)", funcvalidator,
                    "Profile number defined in Fullprof.");

    declareProperty("Append", false, "If true and the output file exists, the bank will be appended to the existing one.");
  }

  //----------------------------------------------------------------------------------------------
  /** Main execution body
    */
  void SaveFullprofResolution::exec()
  {
    // Get input parameters
    processProperties();

    // Parse the input
    parseTableWorkspace(m_bankID);

    // Generate the string for the file to write
    std::string filestr;
    switch (m_fpProfileNumber)
    {
      case 9:
        filestr = toProf9IrfString();
        break;

      case 10:
        filestr = toProf10IrfString();
        break;

      default:
        throw runtime_error("Profile number is not supported yet.");

    }

    // Write to file
    std::ofstream ofile;
    // TODO - Make it an option to write or append a file
#if 0
    // Make it work!
    if (m_append)
    {
      ofile.open(irffilename.c_str());
    }
    else
    {
      ofile.open(irffilename.c_str());
    }
#endif
    ofile << filestr;
    ofile.close();

    return;
  }

  //----------------------------------------------------------------------------------------------
  /** Process properties
    */
  void SaveFullprofResolution::processProperties()
  {
    // Parameter table
    m_profileTableWS = getProperty("InputWorkspace");

    // Output file and operation
    m_outIrfFilename = getPropertyValue("OutputFilename");
    m_append = getProperty("Append");
    if (m_append)
    {
      // Set append flag to false if file does not exist
      bool fileexist = Poco::File(m_outIrfFilename).exists();
      if (!fileexist)
        m_append = false;
    }

    // Bank to write out
    m_bankID = getProperty("Bank");

    // Profile function
    string proffunction = getProperty("ProfileFunction");
    if (proffunction == "Back-to-back exponential convoluted with pseudo-voigt (profile 9)")
      m_fpProfileNumber = 9;
    else if (proffunction == "Jason Hodge's function (profile 10)")
      m_fpProfileNumber = 10;
    else
    {
      stringstream errmsg;
      errmsg << "It is impossible to have profile function " << proffunction << " input. ";
      g_log.error(errmsg.str());
      throw runtime_error(errmsg.str());
    }

    return;
  }

  //----------------------------------------------------------------------------------------------
  /** Parse the table workspace to a map of parameters (name and value)
    * to look up
    */
  void SaveFullprofResolution::parseTableWorkspace(int bankid)
  {
    // Check the table workspace
    std::vector<std::string> colnames = m_profileTableWS->getColumnNames();
    stringstream dbmsgss("Input table's column names: ");
    for (size_t i = 0; i < colnames.size(); ++i)
    {
      dbmsgss << setw(20) << colnames[i];
    }
    g_log.debug(dbmsgss.str());

    // TODO - Read out a list of parameter names
    vector<string> vec_parnames;
    {"......";}

    int rowbankindex = -1;
    // TODO - Locate BANK
    {"......";}


    // Locate the column number to pass parameters
    int colindex = -1;
    if (rowbankindex < 0)
    {
      // TODO - If there is NO 'BANK', Locate first (from left) column starting with 'Value'
      "......";
    }
    else
    {
      // TODO - If there is BANK, Locate first (from left) column starting with 'Value' and BANK matches
      "......";
    }
    if (colindex < 0)
    {
      throw runtime_error("Unable to find column");
    }
    else if (colindex >= static_cast<int>(m_profileTableWS->columnCount()))
    {
      throw runtime_error("Impossible to have this situation.");
    }

#if 0
    // FIXME - The order of the column name can be flexible in future
    if (colnames.size() < 2 || colnames[0].compare("Name") || !boost::starts_with(colnames[colindex], "Value"))
    {
      std::stringstream errmsg;
      errmsg << "Input parameter workspace is not supported or recoganizable.  Possible reason is " << "\n";
      errmsg << "(1) too few columns.  (2) first and second column are not Name and Value.";
      g_log.error() << errmsg.str() << "\n";
      throw std::invalid_argument(errmsg.str());
    }
#endif

    // Clear the parameter
    m_profileParamMap.clear();

    // Parse
    size_t numpars = vec_parnames.size();
    for (size_t ir = 0; ir < numpars; ++ir)
    {
      double parvalue = m_profileTableWS->cell<double>(ir, static_cast<size_t>(colindex));
      m_profileParamMap.insert(std::make_pair(vec_parnames[ir], parvalue));
    }

    // Debug output
    stringstream dbss("Imported Parameter Table: \n");
    map<string, double>::iterator mit;
    for (mit = m_profileParamMap.begin(); mit != m_profileParamMap.end(); ++mit)
      dbss << setw(20) << mit->first << " = " << setprecision(5) << mit->second << endl;
    g_log.debug(dbss.str());

    return;
  }


  //----------------------------------------------------------------------------------------------
  /**  Convert the parameters to Fullprof resolution file string
    */
  std::string SaveFullprofResolution::toProf10IrfString()
  {
    // Get all parameter values
    double tofmin = m_profileParamMap["tof-min"];
    double tofmax = m_profileParamMap["tof-max"];
    double zero = m_profileParamMap["Zero"];
    double zerot = m_profileParamMap["Zerot"];
    double tofstep = m_profileParamMap["step"];
    double dtt1 = m_profileParamMap["Dtt1"];
    double dtt1t = m_profileParamMap["Dtt1t"];
    double dtt2t = m_profileParamMap["Dtt2t"];
    double xcross = m_profileParamMap["Tcross"];
    double width = m_profileParamMap["Width"];
    double sig2 = m_profileParamMap["Sig2"];
    double sig1 = m_profileParamMap["Sig1"];
    double sig0 = m_profileParamMap["Sig0"];
    double gam2 = m_profileParamMap["Gam2"];
    double gam1 = m_profileParamMap["Gam1"];
    double gam0 = m_profileParamMap["Gam0"];
    double alph0 = m_profileParamMap["Alph0"];
    double alph1 = m_profileParamMap["Alph1"];
    double alph0t = m_profileParamMap["Alph0t"];
    double alph1t = m_profileParamMap["Alph1t"];
    double beta0 = m_profileParamMap["Beta0"];
    double beta1 = m_profileParamMap["Beta1"];
    double beta0t = m_profileParamMap["Beta0t"];
    double beta1t = m_profileParamMap["Beta1t"];
    int profindex = static_cast<int>(floor(m_profileParamMap["Profile"] + 0.5));
    double twotheta = m_profileParamMap["twotheta"];

    // Check with profile index
    if (profindex == 0)
    {
      profindex = 10;
    }
    else if (profindex != 10)
    {
      stringstream errmsg;
      errmsg << "This column in table has profile number " << profindex << " other than 10.";
      g_log.error(errmsg.str());
      throw runtime_error(errmsg.str());
    }

    stringstream content;
    content << fixed;

    // Write header
    if (m_append)
    {
      content << "  Instrumental resolution function for POWGEN/SNS  J.P. Hodges  2011-09-02  ireso: 6" << "\n";
      content << "! To be used with function NPROF=" << profindex << " in FullProf  (Res=6)" << "\n";
    }

    // Write bank information
    content << "! ----------------------------------------------  Bank " << m_bankID << "  ";
    if (has_key(m_profileParamMap, "CWL"))
    {
      double cwl = m_profileParamMap["CWL"];
      content << "CWL =   " << setprecision(4) << cwl << "A" << "\n";
    }
    else
    {
      content << "\n";
    }

    // Write profile parameter
    content << "!  Type of profile function: back-to-back exponentials * pseudo-Voigt" << "\n";
    content << "NPROF " << profindex << "\n";

    content << "!       Tof-min(us)    step      Tof-max(us)" << "\n";
    content << "TOFRG   "
            << setprecision(3) << tofmin << " "
            << setw(16) << setprecision(5) << tofstep << " "
            << setw(16) << setprecision(3) << tofmax << "\n";

    content << "!       Zero   Dtt1" << "\n";
    content << "ZD2TOF     "
            << setprecision(5) << zero
            << setw(16) << setprecision(5) << dtt1 << "\n";

    content << "!       Zerot    Dtt1t       Dtt2t    x-cross    Width" << "\n";
    content << "ZD2TOT    "
            << setprecision(5) << zerot
            << setw(16) << setprecision(5) << dtt1t
            << setw(16) << setprecision(5) << dtt2t
            << setw(16) << setprecision(10) << xcross
            << setw(16) << setprecision(5) << width << "\n";

    content << "!     TOF-TWOTH of the bank" << "\n";
    content << "TWOTH    " << setprecision(3) << twotheta << "\n";

    // Note that sig0, sig1 and sig2 used in LeBail/Mantid framework are of the definition in manual.
    // In .irf file, Sig-0, Sig-1 and Sig-2 are the squared values;
    content << "!       Sig-2     Sig-1     Sig-0" << "\n";
    content << "SIGMA  "
            << setprecision(6) << sig2*sig2
            << setw(16) << setprecision(6) << sig1*sig1
            << setw(16) << setprecision(6) << sig0*sig0 << "\n";

    content << "!       Gam-2     Gam-1     Gam-0" << "\n";
    content << "GAMMA  "
            << setw(16) << setprecision(6) << gam2 << " "
            << setw(16) << setprecision(6) << gam1 << " "
            << setw(16) << setprecision(6) << gam0 << "\n";

    content << "!          alph0       beta0       alph1       beta1" << "\n";
    content << "ALFBE        "
            << setprecision(6) << alph0 << " " <<
               setw(16) << setprecision(6) << beta0 << " " <<
               setw(16) << setprecision(6) << alph1 << " " <<
               setw(16) << setprecision(6) << beta1 << "\n";

    content << "!         alph0t      beta0t      alph1t      beta1t" << "\n";
    content << "ALFBT       "
            << setprecision(6) << alph0t << " "
            << setw(16) << setprecision(6) << beta0t << " "
            << setw(16) << setprecision(6) << alph1t << " "
            << setw(16) << setprecision(6) << beta1t << "\n";
    content << "END" << "\n";

    return content.str();

  }

  //----------------------------------------------------------------------------------------------
  /** Write out the string for Fullprof profile 9
    */
  std::string SaveFullprofResolution::toProf9IrfString()
  {
    // Get all parameter values
    double tofmin = m_profileParamMap["tof-min"];
    double tofmax = m_profileParamMap["tof-max"];
    double zero = m_profileParamMap["Zero"];
    double tofstep = m_profileParamMap["step"];
    double dtt1 = m_profileParamMap["Dtt1"];
    double dtt2 = m_profileParamMap["Dtt2"];
    double sig2 = m_profileParamMap["Sig2"];
    double sig1 = m_profileParamMap["Sig1"];
    double sig0 = m_profileParamMap["Sig0"];
    double gam2 = m_profileParamMap["Gam2"];
    double gam1 = m_profileParamMap["Gam1"];
    double gam0 = m_profileParamMap["Gam0"];
    double alph0 = m_profileParamMap["Alph0"];
    double alph1 = m_profileParamMap["Alph1"];
    double beta0 = m_profileParamMap["Beta0"];
    double beta1 = m_profileParamMap["Beta1"];
    int profindex = static_cast<int>(floor(m_profileParamMap["Profile"] + 0.5));
    double twotheta = m_profileParamMap["twotheta"];
    if (twotheta < 0)
      twotheta += 360.;

    // Check with profile index
    if (profindex == 0)
    {
      profindex = 9;
    }
    else if (profindex != 9)
    {
      stringstream errmsg;
      errmsg << "This column in table has profile number " << profindex << " other than 9.";
      g_log.error(errmsg.str());
      throw runtime_error(errmsg.str());
    }

    stringstream content;
    content << fixed;

    // Write header
    if (!m_append)
    {
      content << "Instrumental resolution function for HRPD/ISIS L. Chapon 12/2003  ireso: 5" << "\n";
      content << "! To be used with function NPROF=" << profindex << " in FullProf  (Res=5)" << "\n";
    }

    // Write bank information
    content << "! ----------------------------------------------  Bank " << m_bankID << "  ";
    if (has_key(m_profileParamMap, "CWL"))
    {
      double cwl = m_profileParamMap["CWL"];
      content << "CWL =   " << setprecision(4) << cwl << "A" << "\n";
    }
    else
    {
      content << "\n";
    }

    // Write profile parameters
    content << "!  Type of profile function: back-to-back exponentials * pseudo-Voigt" << "\n";
    content << "NPROF " << profindex << "\n";
    content << "!       Tof-min(us)    step      Tof-max(us)" << "\n";
    content << "TOFRG   "
            << setprecision(3) << tofmin << " "
            << setw(16) << setprecision(5) << tofstep << " "
            << setw(16) << setprecision(3) << tofmax << "\n";

    content << "!        Dtt1           Dtt2       Zero" << "\n";
    content << "D2TOF     "
            << setw(16) << setprecision(5) << dtt1
            << setprecision(5) << dtt2
            << setw(16) << setprecision(5) << zero << "\n";

    content << "!     TOF-TWOTH of the bank" << "\n";
    content << "TWOTH    " << setprecision(3) << twotheta << "\n";

    // Note that sig0, sig1 and sig2 used in LeBail/Mantid framework are of the definition in manual.
    // In .irf file, Sig-0, Sig-1 and Sig-2 are the squared values;
    content << "!       Sig-2     Sig-1     Sig-0" << "\n";
    content << "SIGMA  "
            << setprecision(6) << sig2*sig2
            << setw(16) << setprecision(6) << sig1*sig1
            << setw(16) << setprecision(6) << sig0*sig0 << "\n";

    content << "!       Gam-2     Gam-1     Gam-0" << "\n";
    content << "GAMMA  "
            << setw(16) << setprecision(6) << gam2 << " "
            << setw(16) << setprecision(6) << gam1 << " "
            << setw(16) << setprecision(6) << gam0 << "\n";

    content << "!          alph0       beta0       alph1       beta1" << "\n";
    content << "ALFBE        "
            << setprecision(6) << alph0 << " " <<
               setw(16) << setprecision(6) << beta0 << " " <<
               setw(16) << setprecision(6) << alph1 << " " <<
               setw(16) << setprecision(6) << beta1 << "\n";

    content << "END" << "\n";

    return content.str();
  }

  //
  /** Check wether a profile parameter map has the parameter
    */
  bool SaveFullprofResolution::has_key(std::map<std::string, double> profmap, std::string key)
  {
    // TODO - Implement this function
    {"... ...";}


    throw runtime_error("To Implement ASAP");
    return false;
  }
  


} // namespace DataHandling
} // namespace Mantid
