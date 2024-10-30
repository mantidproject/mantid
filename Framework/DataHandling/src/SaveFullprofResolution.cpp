// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#include "MantidDataHandling/SaveFullprofResolution.h"
#include "MantidAPI/FileProperty.h"
#include "MantidAPI/TableRow.h"
#include "MantidAPI/WorkspaceProperty.h"
#include "MantidKernel/BoundedValidator.h"
#include "MantidKernel/ListValidator.h"

#include "Poco/File.h"
#include "boost/math/special_functions/round.hpp"

#include <fstream>
#include <iomanip>

using namespace Mantid;
using namespace Mantid::API;
using namespace Mantid::Kernel;
using namespace Mantid::DataObjects;
using namespace std;

namespace Mantid::DataHandling {

DECLARE_ALGORITHM(SaveFullprofResolution)

//----------------------------------------------------------------------------------------------
/** Constructor
 */
SaveFullprofResolution::SaveFullprofResolution()
    : API::Algorithm(), m_profileParamMap(), m_profileTableWS(), m_outIrfFilename(), m_bankID(-1),
      m_fpProfileNumber(-1), m_append(false) {}

//----------------------------------------------------------------------------------------------
/** Init to define parameters
 */
void SaveFullprofResolution::init() {
  declareProperty(std::make_unique<WorkspaceProperty<TableWorkspace>>("InputWorkspace", "", Direction::Input),
                  "Input TableWorkspace containing the parameters for .irf file.");

  std::vector<std::string> exts{".irf"};
  declareProperty(std::make_unique<API::FileProperty>("OutputFilename", "", API::FileProperty::Save, exts),
                  "Name of the output .irf file.");

  std::shared_ptr<BoundedValidator<int>> bankboundval = std::make_shared<BoundedValidator<int>>();
  bankboundval->setLower(0);
  declareProperty("Bank", EMPTY_INT(), "Bank number of the parameters belonged to. ");

  vector<string> supportedfunctions;
  supportedfunctions.emplace_back("Back-to-back exponential convoluted with pseudo-voigt (profile 9)");
  supportedfunctions.emplace_back("Jason Hodge's function (profile 10)");
  auto funcvalidator = std::make_shared<StringListValidator>(supportedfunctions);
  declareProperty("ProfileFunction", "Jason Hodge's function (profile 10)", funcvalidator,
                  "Profile number defined in Fullprof.");

  declareProperty("Append", false,
                  "If true and the output file exists, the "
                  "bank will be appended to the existing "
                  "one.");
}

//----------------------------------------------------------------------------------------------
/** Main execution body
 */
void SaveFullprofResolution::exec() {
  // Get input parameters
  processProperties();

  // Parse the input
  parseTableWorkspace();

  // Generate the string for the file to write
  std::string filestr;
  switch (m_fpProfileNumber) {
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
  // Make it work!
  if (m_append) {
    ofile.open(m_outIrfFilename.c_str(), std::ofstream::out | std::ofstream::app);
    g_log.information() << "Opened output file " << m_outIrfFilename << " in append mode. "
                        << "\n";
  } else {
    ofile.open(m_outIrfFilename.c_str(), std::ofstream::out | std::ofstream::trunc);
    g_log.information() << "Opened output file " << m_outIrfFilename << " in new/overwrite mode. "
                        << "\n";
  }
  ofile << filestr;
  ofile.close();
}

//----------------------------------------------------------------------------------------------
/** Process properties
 */
void SaveFullprofResolution::processProperties() {
  // Parameter table
  m_profileTableWS = getProperty("InputWorkspace");

  // Output file and operation
  m_outIrfFilename = getPropertyValue("OutputFilename");
  if (m_outIrfFilename.empty())
    throw runtime_error("Input file name invalid. ");
  m_append = getProperty("Append");
  if (m_append) {
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
  else {
    stringstream errmsg;
    errmsg << "It is impossible to have profile function " << proffunction << " input. ";
    g_log.error(errmsg.str());
    throw runtime_error(errmsg.str());
  }
}

//----------------------------------------------------------------------------------------------
/** Parse the table workspace to a map of parameters (name and value)
 * to look up
 */
void SaveFullprofResolution::parseTableWorkspace() {
  // Check the table workspace
  std::vector<std::string> colnames = m_profileTableWS->getColumnNames();
  size_t numcols = colnames.size();

  stringstream dbmsgss("Input table's column names: ");
  for (const auto &colname : colnames) {
    dbmsgss << setw(20) << colname;
  }
  g_log.debug(dbmsgss.str());

  if (colnames[0] != "Name")
    throw runtime_error("First colunm must be 'Name'");

  // Read out a list of parameter names
  size_t numpars = m_profileTableWS->rowCount();
  vector<string> vec_parnames(numpars);
  int rowbankindex = -1;
  for (size_t i = 0; i < numpars; ++i) {
    string parname = m_profileTableWS->cell<string>(i, 0);
    vec_parnames[i] = parname;
    if (parname == "BANK")
      rowbankindex = static_cast<int>(i);
  }

  // Locate the column number to pass parameters
  int colindex = -1;
  if (rowbankindex < 0) {
    // If there is NO 'BANK', locate first (from left) column starting with
    // 'Value'
    for (size_t i = 1; i < numcols; ++i) {
      if (colnames[i].starts_with("Value")) {
        colindex = static_cast<int>(i);
        break;
      }
    }
  } else {
    // If there is BANK, Locate first (from left) column starting with 'Value'
    // and BANK matches
    for (size_t i = 1; i < numcols; ++i) {
      if (colnames[i].starts_with("Value")) {
        int bankid = boost::math::iround(m_profileTableWS->cell<double>(rowbankindex, i));
        if (bankid == m_bankID) {
          colindex = static_cast<int>(i);
          break;
        }
      }
    }
  }

  if (colindex < 0) {
    throw runtime_error("Unable to find column");
  } else if (colindex >= static_cast<int>(m_profileTableWS->columnCount())) {
    throw runtime_error("Impossible to have this situation.");
  }

  // Clear the parameter
  m_profileParamMap.clear();

  // Parse
  for (size_t ir = 0; ir < numpars; ++ir) {
    double parvalue = m_profileTableWS->cell<double>(ir, static_cast<size_t>(colindex));
    m_profileParamMap.emplace(vec_parnames[ir], parvalue);
  }

  // Debug output
  stringstream dbss("Imported Parameter Table: \n");
  map<string, double>::iterator mit;
  for (mit = m_profileParamMap.begin(); mit != m_profileParamMap.end(); ++mit)
    dbss << setw(20) << mit->first << " = " << setprecision(5) << mit->second << '\n';
  g_log.debug(dbss.str());
}

//----------------------------------------------------------------------------------------------
/**  Convert the parameters to Fullprof resolution file string
 */
std::string SaveFullprofResolution::toProf10IrfString() {
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
  if (profindex == 0) {
    profindex = 10;
  } else if (profindex != 10) {
    stringstream errmsg;
    errmsg << "This column in table has profile number " << profindex << " other than 10.";
    g_log.error(errmsg.str());
    throw runtime_error(errmsg.str());
  }

  stringstream content;
  content << fixed;

  // Write header
  if (!m_append) {
    content << "  Instrumental resolution function for POWGEN/SNS  ireso: 6"
            << "\n";
    content << "! To be used with function NPROF=" << profindex << " in FullProf  (Res=6)"
            << "\n";
  }

  // Write bank information
  content << "! ----------------------------------------------  Bank " << m_bankID << "  ";
  if (has_key(m_profileParamMap, "CWL")) {
    double cwl = m_profileParamMap["CWL"];
    if (cwl > 0)
      content << "CWL =   " << setprecision(4) << cwl << "A"
              << "\n";
    else
      content << "\n";
  } else {
    content << "\n";
  }

  // Write profile parameter
  content << "!  Type of profile function: back-to-back exponentials * pseudo-Voigt"
          << "\n";
  content << "NPROF " << profindex << "\n";

  content << "!       Tof-min(us)    step      Tof-max(us)"
          << "\n";
  content << "TOFRG   " << setprecision(3) << tofmin << " " << setw(16) << setprecision(5) << tofstep << " " << setw(16)
          << setprecision(3) << tofmax << "\n";

  content << "!       Zero   Dtt1"
          << "\n";
  content << "ZD2TOF     " << setw(16) << setprecision(5) << zero << setw(16) << setprecision(5) << dtt1 << "\n";

  content << "!       Zerot    Dtt1t       Dtt2t    x-cross    Width"
          << "\n";
  content << "ZD2TOT    " << setprecision(5) << zerot << setw(16) << setprecision(5) << dtt1t << setw(16)
          << setprecision(5) << dtt2t << setw(16) << setprecision(10) << xcross << setw(16) << setprecision(5) << width
          << "\n";

  content << "!     TOF-TWOTH of the bank"
          << "\n";
  content << "TWOTH    " << setprecision(3) << twotheta << "\n";

  // Note that sig0, sig1 and sig2 used in LeBail/Mantid framework are of the
  // definition in manual.
  // In .irf file, Sig-0, Sig-1 and Sig-2 are the squared values;
  content << "!       Sig-2     Sig-1     Sig-0"
          << "\n";
  content << "SIGMA  " << setprecision(6) << sig2 * sig2 << setw(16) << setprecision(6) << sig1 * sig1 << setw(16)
          << setprecision(6) << sig0 * sig0 << "\n";

  content << "!       Gam-2     Gam-1     Gam-0"
          << "\n";
  content << "GAMMA  " << setw(16) << setprecision(6) << gam2 << " " << setw(16) << setprecision(6) << gam1 << " "
          << setw(16) << setprecision(6) << gam0 << "\n";

  content << "!          alph0       beta0       alph1       beta1"
          << "\n";
  content << "ALFBE        " << setprecision(6) << alph0 << " " << setw(16) << setprecision(6) << beta0 << " "
          << setw(16) << setprecision(6) << alph1 << " " << setw(16) << setprecision(6) << beta1 << "\n";

  content << "!         alph0t      beta0t      alph1t      beta1t"
          << "\n";
  content << "ALFBT       " << setprecision(6) << alph0t << " " << setw(16) << setprecision(6) << beta0t << " "
          << setw(16) << setprecision(6) << alph1t << " " << setw(16) << setprecision(6) << beta1t << "\n";
  content << "END"
          << "\n";

  return content.str();
}

//----------------------------------------------------------------------------------------------
/** Write out the string for Fullprof profile 9
 */
std::string SaveFullprofResolution::toProf9IrfString() {
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
  if (profindex == 0) {
    profindex = 9;
  } else if (profindex != 9) {
    stringstream errmsg;
    errmsg << "This column in table has profile number " << profindex << " other than 9.";
    g_log.error(errmsg.str());
    throw runtime_error(errmsg.str());
  }

  stringstream content;
  content << fixed;

  // Write header
  if (!m_append) {
    content << "Instrumental resolution function for HRPD/ISIS L. Chapon "
               "12/2003  ireso: 5"
            << "\n";
    content << "! To be used with function NPROF=" << profindex << " in FullProf  (Res=5)"
            << "\n";
  }

  // Write bank information
  content << "! ----------------------------------------------  Bank " << m_bankID << "  ";
  if (has_key(m_profileParamMap, "CWL")) {
    double cwl = m_profileParamMap["CWL"];
    if (cwl > 0.)
      content << "CWL =   " << setprecision(4) << cwl << "A"
              << "\n";
    else
      content << "\n";
  } else {
    content << "\n";
  }

  // Write profile parameters
  content << "!  Type of profile function: back-to-back exponentials * pseudo-Voigt"
          << "\n";
  content << "NPROF " << profindex << "\n";
  content << "!       Tof-min(us)    step      Tof-max(us)"
          << "\n";
  content << "TOFRG   " << setprecision(3) << tofmin << " " << setw(16) << setprecision(5) << tofstep << " " << setw(16)
          << setprecision(3) << tofmax << "\n";

  content << "!        Dtt1           Dtt2       Zero"
          << "\n";
  content << "D2TOF     " << setw(16) << setprecision(5) << dtt1 << setw(16) << setprecision(5) << dtt2 << setw(16)
          << setprecision(5) << zero << "\n";

  content << "!     TOF-TWOTH of the bank"
          << "\n";
  content << "TWOTH    " << setprecision(3) << twotheta << "\n";

  // Note that sig0, sig1 and sig2 used in LeBail/Mantid framework are of the
  // definition in manual.
  // In .irf file, Sig-0, Sig-1 and Sig-2 are the squared values;
  content << "!       Sig-2     Sig-1     Sig-0"
          << "\n";
  content << "SIGMA  " << setprecision(6) << sig2 * sig2 << setw(16) << setprecision(6) << sig1 * sig1 << setw(16)
          << setprecision(6) << sig0 * sig0 << "\n";

  content << "!       Gam-2     Gam-1     Gam-0"
          << "\n";
  content << "GAMMA  " << setw(16) << setprecision(6) << gam2 << " " << setw(16) << setprecision(6) << gam1 << " "
          << setw(16) << setprecision(6) << gam0 << "\n";

  content << "!          alph0       beta0       alph1       beta1"
          << "\n";
  content << "ALFBE        " << setprecision(6) << alph0 << " " << setw(16) << setprecision(6) << beta0 << " "
          << setw(16) << setprecision(6) << alph1 << " " << setw(16) << setprecision(6) << beta1 << "\n";

  content << "END"
          << "\n";

  return content.str();
}

//
/** Check whether a profile parameter map has the parameter
 */
bool SaveFullprofResolution::has_key(std::map<std::string, double> profmap, const std::string &key) {
  map<string, double>::iterator fiter;
  fiter = profmap.find(key);
  bool exist = true;
  if (fiter == profmap.end())
    exist = false;

  return exist;
}

} // namespace Mantid::DataHandling
