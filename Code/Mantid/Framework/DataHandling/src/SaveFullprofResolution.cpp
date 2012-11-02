#include "MantidDataHandling/SaveFullprofResolution.h"
#include "MantidAPI/WorkspaceProperty.h"
#include "MantidAPI/FileProperty.h"
#include "MantidAPI/TableRow.h"
#include <iomanip>
#include <fstream>

using namespace Mantid;
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

  /** Wiki docs
    */
  void SaveFullprofResolution::initDocs()
  {
    this->setWikiSummary("Generate Fullprof's resolution file (.irf) from table workspace.");
    this->setOptionalMessage("Genearte Fullprof's .irf file from TableWorkspace. ");
  }

  /** Init to define parameters
    */
  void SaveFullprofResolution::init()
  {
    this->declareProperty(new API::WorkspaceProperty<DataObjects::TableWorkspace>("InputWorkspace", "Anonymous", Direction::Input),
                          "Input TableWorkspace containing the parameters for .irf file.");

    std::vector<std::string> exts;
    exts.push_back(".irf");
    this->declareProperty(new API::FileProperty("OutputFile", "fp.irf", API::FileProperty::Save, exts),
                          "Name of the output .irf file.");

    this->declareProperty("Bank", 1, "Bank number of the parameters belonged to. ");
  }

  /** Main execution body
    */
  void SaveFullprofResolution::exec()
  {
    // 1. Get input parameters
    inpWS = getProperty("InputWorkspace");
    std::string irffilename = getProperty("OutputFile");
    int bankid = getProperty("Bank");

    // 2. Parse the input
    parseTableWorkspace();

    // 3. Generate the string for the file to write
    std::string filestr = toIRFString(bankid);

    // 4. Write to file
    std::ofstream ofile;
    ofile.open(irffilename.c_str());
    ofile << filestr;
    ofile.close();

    return;
  }

  /** Parse the table workspace to a map of parameters (name and value)
    * to look up
    */
  void SaveFullprofResolution::parseTableWorkspace()
  {
    // 1. Clear the parameter
    mParameters.clear();

    // 2. Check the table workspace
    std::vector<std::string> colnames = inpWS->getColumnNames();
    stringstream dbmsgss("Input table's column names: ");
    for (size_t i = 0; i < colnames.size(); ++i)
    {
      dbmsgss << setw(20) << colnames[i];
    }
    cout << dbmsgss.str() << endl;

    // FIXME - The order of the column name can be flexible in future
    if (colnames.size() < 2 || colnames[0].compare("Name") || colnames[1].compare("Value"))
    {
      std::stringstream errmsg;
      errmsg << "Input parameter workspace is not supported or recoganizable.  Possible reason is " << std::endl;
      errmsg << "(1) too few columns.  (2) first and second column are not Name and Value.";
      g_log.error() << errmsg.str() << std::endl;
      throw std::invalid_argument(errmsg.str());
    }

    // 3. Parse
    size_t numrows = inpWS->rowCount();
    for (size_t ir = 0; ir < numrows; ++ir)
    {
      const API::TableRow& row = inpWS->getRow(ir);
      std::string parname;
      double parvalue;
      row >> parname >> parvalue;
      mParameters.insert(std::make_pair(parname, parvalue));
    }

    // 4. Debug output
    stringstream dbss("Imported Parameter Table: \n");
    map<string, double>::iterator mit;
    for (mit = mParameters.begin(); mit != mParameters.end(); ++mit)
      dbss << setw(20) << mit->first << " = " << setprecision(5) << mit->second << endl;
    cout << dbss.str();

    return;
  }


  /**  Convert the parameters to Fullprof resolution file string
    */
  std::string SaveFullprofResolution::toIRFString(int bankid)
  {
    double tofmin = mParameters["tof-min"];
    double tofmax = mParameters["tof-max"];
    double zero = mParameters["Zero"];
    double zerot = mParameters["Zerot"];
    double tofstep = mParameters["step"];
    double dtt1 = mParameters["Dtt1"];
    double dtt1t = mParameters["Dtt1t"];
    double dtt2t = mParameters["Dtt2t"];
    double xcross = mParameters["Tcross"];
    double width = mParameters["Width"];
    double sig2 = mParameters["Sig2"];
    double sig1 = mParameters["Sig1"];
    double sig0 = mParameters["Sig0"];
    double gam2 = mParameters["Gam2"];
    double gam1 = mParameters["Gam1"];
    double gam0 = mParameters["Gam0"];
    double alph0 = mParameters["Alph0"];
    double alph1 = mParameters["Alph1"];
    double alph0t = mParameters["Alph0t"];
    double alph1t = mParameters["Alph1t"];
    double beta0 = mParameters["Beta0"];
    double beta1 = mParameters["Beta1"];
    double beta0t = mParameters["Beta0t"];
    double beta1t = mParameters["Beta0"];
    double profindex = mParameters["Profile"];
    double twotheta = mParameters["twotheta"];

    std::stringstream content;

    content << "  Instrumental resolution function for POWGEN/SNS  J.P. Hodges  2011-09-02  ireso: 6" << std::endl;
    content << "! To be used with function NPROF=" << profindex << " in FullProf  (Res=6)" << std::endl;
    content << "! ----------------------------------------------  Bank " << bankid << "  CWL =   0.5330A" << std::endl;
    content << "!  Type of profile function: back-to-back exponentials * pseudo-Voigt" << std::endl;
    content << "NPROF " << profindex << std::endl;
    content << "!       Tof-min(us)    step      Tof-max(us)" << std::endl;
    content << "TOFRG   " << setw(10) << tofmin << setw(10) << tofstep << setw(10) << tofmax << std::endl;
    content << "!       Zero   Dtt1" << std::endl;
    content << "ZD2TOF     " << setw(10) << zero << setw(10) << dtt1 << std::endl;
    content << "!       Zerot    Dtt1t       Dtt2t    x-cross    Width" << std::endl;
    content << "ZD2TOT    " << setw(10) << zerot << setw(10) << dtt1t << setw(10) << dtt2t << setw(10) << xcross
            << setw(10) << width << std::endl;
    content << "!     TOF-TWOTH of the bank" << std::endl;
    content << "TWOTH    " << twotheta << std::endl;
    content << "!       Sig-2     Sig-1     Sig-0" << std::endl;
    content << "SIGMA  " << setw(10) << sig2 << setw(10) << sig1 << setw(10) << sig0 << std::endl;
    content << "!       Gam-2     Gam-1     Gam-0" << std::endl;
    content << "GAMMA  " << setw(10) << gam2 << setw(10) << gam1 << setw(10) << gam0 << std::endl;
    content << "!          alph0       beta0       alph1       beta1" << std::endl;
    content << "ALFBE        " << setw(10) << alph0 << setw(10) << beta0 << setw(10) << alph1 << setw(10) << beta1 << std::endl;
    content << "!         alph0t      beta0t      alph1t      beta1t" << std::endl;
    content << "ALFBT       " << setw(10) << alph0t << setw(10) << beta0t << setw(10) << alph1t << setw(10) << beta1t << std::endl;
    content << "END" << std::endl;

    return content.str();

  }
  


} // namespace DataHandling
} // namespace Mantid
