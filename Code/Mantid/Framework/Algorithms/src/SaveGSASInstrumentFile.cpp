#include "MantidAlgorithms/SaveGSASInstrumentFile.h"
#include "MantidAPI/FrameworkManager.h"
#include "MantidAPI/FileProperty.h"
#include "MantidKernel/ListValidator.h"
#include "MantidKernel/ArrayProperty.h"
#include "MantidKernel/BoundedValidator.h"
#include "MantidAPI/TableRow.h"

#include <stdio.h>
#include <iomanip>

using namespace Mantid;
using namespace Mantid::API;
using namespace Mantid::Kernel;
using namespace Mantid::DataObjects;

using namespace std;

namespace Mantid {
namespace Algorithms {

DECLARE_ALGORITHM(SaveGSASInstrumentFile)

class ChopperConfiguration {
public:
  ChopperConfiguration(vector<int> bankids);
  ChopperConfiguration(const int freq, const std::string &bankidstr,
                       const std::string &cwlstr, const std::string &mndspstr,
                       const std::string &mxdspstr,
                       const std::string &maxtofstr);

  /// Get bank IDs in configuration
  std::vector<unsigned int> getBankIDs() const;
  /// Check wehther a bank is defined
  bool hasBank(unsigned int bankid) const;
  /// Get a parameter from a bank
  double getParameter(unsigned int bankid, const std::string &paramname) const;
  /// Set a parameter to a bank
  void setParameter(unsigned int bankid, const std::string &paramname,
                    double value);

private:
  std::string parseString() const;
  /// Parse string to a double vector
  std::vector<double> parseStringDbl(const std::string &instring) const;
  /// Parse string to an integer vector
  std::vector<unsigned int>
  parseStringUnsignedInt(const std::string &instring) const;

  const double m_frequency;
  std::vector<unsigned int> m_bankIDs;
  std::map<unsigned int, size_t> m_bankIDIndexMap;

  std::vector<double> m_vec2Theta;
  std::vector<double> m_vecL1;
  std::vector<double> m_vecL2;

  std::vector<double> m_vecCWL;
  std::vector<double> m_mindsps;
  std::vector<double> m_maxdsps;
  std::vector<double> m_maxtofs;

  std::vector<double> m_splitds;
  std::vector<int> m_vruns;
};

typedef boost::shared_ptr<ChopperConfiguration> ChopperConfiguration_sptr;

//----------------------------------------------------------------------------------------------
/** Constructor
  */
ChopperConfiguration::ChopperConfiguration(vector<int> bankids)
    : m_frequency(0) {
  size_t numbanks = bankids.size();

  // Initialize vectors
  m_bankIDs.assign(numbanks, 0);
  m_vecCWL.assign(numbanks, EMPTY_DBL());
  m_mindsps.assign(numbanks, EMPTY_DBL());
  m_maxdsps.assign(numbanks, EMPTY_DBL());
  m_maxtofs.assign(numbanks, EMPTY_DBL());

  // Set bank IDs
  m_bankIDs.assign(bankids.begin(), bankids.end());
  m_bankIDIndexMap.clear();
  for (size_t ib = 0; ib < numbanks; ++ib) {
    m_bankIDIndexMap.insert(make_pair(m_bankIDs[ib], ib));
  }
}

//----------------------------------------------------------------------------------------------
/** Constructor of chopper configuration
    * Removed arguments: std::string splitdstr, std::string vrunstr
    */
ChopperConfiguration::ChopperConfiguration(const int freq,
                                           const std::string &bankidstr,
                                           const std::string &cwlstr,
                                           const std::string &mndspstr,
                                           const std::string &mxdspstr,
                                           const std::string &maxtofstr)
    : m_frequency(freq), m_bankIDs(parseStringUnsignedInt(bankidstr)),
      m_vecCWL(parseStringDbl(cwlstr)), m_mindsps(parseStringDbl(mndspstr)),
      m_maxdsps(parseStringDbl(mxdspstr)), m_maxtofs(parseStringDbl(maxtofstr))

{
  size_t numbanks = m_bankIDs.size();

  // Check size
  if (m_vecCWL.size() != numbanks || m_mindsps.size() != numbanks ||
      m_maxdsps.size() != numbanks || m_maxtofs.size() != numbanks) {
    std::string errmsg(
        "Default chopper constants have different number of elements. ");
    throw runtime_error(errmsg);
  }

  // Set up index map
  m_vec2Theta.resize(numbanks, 0.);
  m_vecL1.resize(numbanks, 0.);
  m_vecL2.resize(numbanks, 0.);

  // Set up bank ID / looking up index map
  m_bankIDIndexMap.clear();
  for (size_t ib = 0; ib < numbanks; ++ib) {
    m_bankIDIndexMap.insert(make_pair(m_bankIDs[ib], ib));
  }
}

//----------------------------------------------------------------------------------------------
/** Get bank IDs in the chopper configuration
  */
vector<unsigned int> ChopperConfiguration::getBankIDs() const {
  return m_bankIDs;
}

//----------------------------------------------------------------------------------------------
/**
 */
bool ChopperConfiguration::hasBank(unsigned int bankid) const {
  return std::find(m_bankIDs.begin(), m_bankIDs.end(), bankid) !=
         m_bankIDs.end();
}

//----------------------------------------------------------------------------------------------
/** Get chopper configuration parameters value
  */
double ChopperConfiguration::getParameter(unsigned int bankid,
                                          const string &paramname) const {
  // Obtain index for the bank
  map<unsigned int, size_t>::const_iterator biter =
      m_bankIDIndexMap.find(bankid);
  if (biter == m_bankIDIndexMap.end()) {
    stringstream errss;
    errss << "Bank ID and index map does not have entry for bank " << bankid;
    throw runtime_error(errss.str());
  }
  size_t bindex = biter->second;

  double value(EMPTY_DBL());

  if (paramname == "TwoTheta") {
    value = m_vec2Theta[bindex];
  } else if (paramname == "MinDsp") {
    // cout << "size of min-dsp = " << m_mindsps.size() << ". --> bindex = " <<
    // bindex << ".\n";
    value = m_mindsps[bindex];
  } else if (paramname == "MaxDsp") {
    // cout << "size of max-dsp = " << m_maxdsps.size() << ". --> bindex = " <<
    // bindex << ".\n";
    value = m_maxdsps[bindex];
  } else if (paramname == "MaxTOF") {
    // cout << "size of max-tof = " << m_maxtofs.size() << ". --> bindex = " <<
    // bindex << ".\n";
    value = m_maxtofs[bindex];
  } else if (paramname == "CWL") {
    value = m_vecCWL[bindex];
  } else {
    stringstream errss;
    errss << "ChopperConfiguration unable to locate: Bank ID = " << bankid
          << ", Parameter = " << paramname;
    throw runtime_error(errss.str());
  }

  return value;
}

//----------------------------------------------------------------------------------------------
/** Set a parameter to a bank
  */
void ChopperConfiguration::setParameter(unsigned int bankid,
                                        const string &paramname, double value) {
  map<unsigned, size_t>::iterator biter = m_bankIDIndexMap.find(bankid);

  if (biter == m_bankIDIndexMap.end()) {
    stringstream errss;
    errss << "Chopper configuration does not have bank " << bankid;
    throw runtime_error(errss.str());
  } else {
    size_t ibank = biter->second;

    if (paramname == "2Theta")
      m_vec2Theta[ibank] = value;
    else if (paramname == "CWL")
      m_vecCWL[ibank] = value;
    else if (paramname == "L1")
      m_vecL1[ibank] = value;
    else if (paramname == "L2")
      m_vecL2[ibank] = value;
    else if (paramname == "MinTOF") {
      // m_mintofs[ibank] = value;
    } else if (paramname == "MaxTOF")
      m_maxtofs[ibank] = value * 1.0E-3;
    else if (paramname == "MinDsp")
      m_mindsps[ibank] = value;
    else if (paramname == "MaxDsp")
      m_maxdsps[ibank] = value;
    else {
      stringstream errss;
      errss << "In Chopper configuration's bank " << bankid
            << ", there is no parameter named " << paramname;
      throw runtime_error(errss.str());
    }
  }

  return;
}

//----------------------------------------------------------------------------------------------
/** Parse string to double vector
  */
vector<double>
ChopperConfiguration::parseStringDbl(const string &instring) const {
  vector<string> strs;
  boost::split(strs, instring, boost::is_any_of(", "));

  vector<double> vecdouble;
  for (size_t i = 0; i < strs.size(); i++) {
    if (strs[i].size() > 0) {
      double item = atof(strs[i].c_str());
      vecdouble.push_back(item);
      // cout << "[C] |" << strs[i] << "|" << item << "\n";
    }
  }

  // cout << "[C]* Input: " << instring << ": size of double vector: " <<
  // vecdouble.size() << endl;

  return vecdouble;
}

//----------------------------------------------------------------------------------------------
/** Parse string to double vector
  */
vector<unsigned int>
ChopperConfiguration::parseStringUnsignedInt(const string &instring) const {
  vector<string> strs;
  boost::split(strs, instring, boost::is_any_of(", "));

  vector<unsigned int> vecinteger;
  for (size_t i = 0; i < strs.size(); i++) {
    if (strs[i].size() > 0) {
      int item = atoi(strs[i].c_str());
      if (item < 0) {
        throw runtime_error(
            "Found negative number in a string for unsigned integers.");
      }
      vecinteger.push_back(static_cast<unsigned int>(item));
    }
  }

  // cout << "[C]* Input : " << instring << ": size of string vector: " <<
  // vecinteger.size() << endl;

  return vecinteger;
}

//----------------------------------------------------------------------------------------------
/** Constructor
 */
SaveGSASInstrumentFile::SaveGSASInstrumentFile() {}

//----------------------------------------------------------------------------------------------
/** Destructor
 */
SaveGSASInstrumentFile::~SaveGSASInstrumentFile() {}

//----------------------------------------------------------------------------------------------

//----------------------------------------------------------------------------------------------
/** Declare properties
  */
void SaveGSASInstrumentFile::init() {
  declareProperty(
      new WorkspaceProperty<ITableWorkspace>(
          "InputWorkspace", "", Direction::Input, PropertyMode::Optional),
      "Name of the table workspace containing the parameters.");

  vector<string> infileexts;
  infileexts.push_back(".irf");
  auto infileprop = new FileProperty("InputFileName", "",
                                     FileProperty::OptionalLoad, infileexts);
  declareProperty(infileprop,
                  "Name of the input Fullprof resolution file (.irf).");

  vector<string> exts;
  exts.push_back(".iparam");
  exts.push_back(".prm");
  auto fileprop =
      new FileProperty("OutputFileName", "", FileProperty::Save, exts);
  declareProperty(fileprop, "Name of the output GSAS instrument file.");

  declareProperty(
      new ArrayProperty<unsigned int>("BankIDs"),
      "Bank IDs of the banks to be written to GSAS instrument file.");

  vector<string> instruments;
  instruments.push_back("powgen");
  instruments.push_back("nomad");
  declareProperty("Instrument", "powgen",
                  boost::make_shared<StringListValidator>(instruments),
                  "Name of the instrument that parameters are belonged to. "
                  "So far, only PG3 and NOM are supported.");

  vector<string> vecfreq;
  vecfreq.push_back("10");
  vecfreq.push_back("30");
  vecfreq.push_back("60");
  declareProperty("ChopperFrequency", "60",
                  boost::make_shared<StringListValidator>(vecfreq),
                  "Frequency of the chopper. ");

  declareProperty("IDLine", "",
                  "ID line to be written in GSAS instrument file");
  declareProperty("Sample", "",
                  "Sample information written to header (title) ");

  boost::shared_ptr<BoundedValidator<double>> mustBePositive(
      new BoundedValidator<double>());
  mustBePositive->setLower(0.0);

  declareProperty("L1", EMPTY_DBL(), mustBePositive,
                  "L1 (primary flight path) of the instrument. ");
  declareProperty("L2", EMPTY_DBL(),
                  "L2 (secondary flight path) of the instrument. "
                  "It must be given if 2Theta is not given. ");
  declareProperty("TwoTheta", EMPTY_DBL(), mustBePositive,
                  "Angle of the detector bank. "
                  "It must be given if L2 is not given. ");

  return;
}

//----------------------------------------------------------------------------------------------
/** Main execution body
  */
void SaveGSASInstrumentFile::exec() {
  // Process user specified properties
  processProperties();

  // Parse profile table workspace
  map<unsigned int, map<string, double>> bankprofileparammap;
  parseProfileTableWorkspace(m_inpWS, bankprofileparammap);

  // Initialize some conversion constants related to the chopper
  initConstants(bankprofileparammap);

  // Deal with a default
  if (m_vecBankID2File.empty()) {
    // Default is to export all banks
    for (map<unsigned int, map<string, double>>::iterator miter =
             bankprofileparammap.begin();
         miter != bankprofileparammap.end(); ++miter) {
      unsigned int bankid = miter->first;
      m_vecBankID2File.push_back(bankid);
    }
    sort(m_vecBankID2File.begin(), m_vecBankID2File.end());
  }
  g_log.debug() << "Number of banks to output = " << m_vecBankID2File.size()
                << ".\n";

  // Convert to GSAS
  convertToGSAS(m_vecBankID2File, m_gsasFileName, bankprofileparammap);

  // Fix?
  Mantid::API::FrameworkManager::Instance();
  IAlgorithm_sptr fit;
  try {
    // Fitting the candidate peaks to a Gaussian
    fit = createChildAlgorithm("FixGSASInstrumentFile", -1, -1, true);
    fit->initialize();
    fit->setProperty("InputFilename", m_gsasFileName);
    fit->setProperty("OutputFilename", m_gsasFileName);
    fit->execute();
  } catch (Exception::NotFoundError &) {
    std::string errorstr(
        "FindPeaks algorithm requires the CurveFitting library");
    g_log.error(errorstr);
    throw std::runtime_error(errorstr);
  }

  return;
}

//----------------------------------------------------------------------------------------------
/** Process input properties
 */
void SaveGSASInstrumentFile::processProperties() {
  // Input workspace
  bool loadirf = false;
  m_inpWS = getProperty("InputWorkspace");
  if (!m_inpWS)
    loadirf = true;

  if (loadirf) {
    // Load .irf file to m_inpWS
    string irffilename = getProperty("InputFileName");
    loadFullprofResolutionFile(irffilename);

    if (!m_inpWS) {
      stringstream errss;
      errss << "Neither input table workspace ("
            << getPropertyValue("InputWorkspace") << ") nor "
            << "input .irf file " << getPropertyValue("InputFileName")
            << " is valid. ";
      g_log.error(errss.str());
      throw runtime_error(errss.str());
    }
  }

  // Instrument information
  m_instrument = getPropertyValue("Instrument");
  m_id_line = getPropertyValue(
      "IDLine"); // Standard Run LB4844 Vanadium: 4866 J.P. Hodges 2011-09-01
  m_sample = getPropertyValue(
      "Sample"); // titleline = "LaB6 NIST RT 4844[V=4866] 60Hz CW=.533"

  m_gsasFileName = getPropertyValue("OutputFileName");
  m_vecBankID2File = getProperty("BankIDs");

  m_L1 = getProperty("L1");
  if (isEmpty(m_L1))
    throw runtime_error("L1 must be given!");
  m_2theta = getProperty("TwoTheta");
  m_L2 = getProperty("L2");
  string freqtempstr = getProperty("ChopperFrequency");
  m_frequency = atoi(freqtempstr.c_str());

  /* Set default value for L1
  if (m_L1 == EMPTY_DBL())
  {
    if (m_instrument == "PG3")
    {
      m_L1 = 60.0;
    }
    else if (m_instrument == "NOM")
    {
      m_L1 = 19.5;
    }
    else
    {
      stringstream errss;
      errss << "L1 is not given. There is no default value for instrument " <<
  m_instrument
            << "Only NOMAD and POWGEN are supported now.\n";
      g_log.error(errss.str());
      throw runtime_error(errss.str());
    }
  }
  else if (m_L1 <= 0.)
  {
    throw runtime_error("Input L1 cannot be less or equal to 0.");
  }
  */

  // Set default value for L2
  if (isEmpty(m_2theta)) {
    if (isEmpty(m_L2)) {
      string errmsg(
          "User must specify either 2theta or L2.  Neither of them is given.");
      g_log.error(errmsg);
      throw runtime_error(errmsg);
    }
  } else {
    // Override L2 by 2theta
    m_L2 = EMPTY_DBL();
  }
}

//----------------------------------------------------------------------------------------------
/** Set up some constant by default
  * Output--> m_configuration
  * @param profmap :: map of parameters
  */
void SaveGSASInstrumentFile::initConstants(
    const map<unsigned int, map<string, double>> &profmap) {
  m_configuration = setupInstrumentConstants(profmap);

  /*
  if (m_instrument.compare("PG3") == 0)
  {
    m_configuration = setupPG3Constants(chopperfrequency);
  }
  else if (m_instrument.compare("NOM") == 0)
  {
    m_configuration = setupNOMConstants(chopperfrequency);
  }
  else
  {
    stringstream errss;
    errss << "Instrument " << m_instrument << " is not supported. ";
    throw runtime_error(errss.str());
  }
  */

  return;
}

//----------------------------------------------------------------------------------------------
/** Parse profile table workspace to a map (the new ...
  */
void SaveGSASInstrumentFile::parseProfileTableWorkspace(
    ITableWorkspace_sptr ws,
    map<unsigned int, map<string, double>> &profilemap) {
  size_t numbanks = ws->columnCount() - 1;
  size_t numparams = ws->rowCount();
  vector<map<string, double>> vec_maptemp(numbanks);
  vector<unsigned int> vecbankindex(numbanks);

  // Check
  vector<string> colnames = ws->getColumnNames();
  if (colnames[0].compare("Name"))
    throw runtime_error("The first column must be Name");

  // Parse
  for (size_t irow = 0; irow < numparams; ++irow) {
    TableRow tmprow = ws->getRow(irow);
    string parname;
    tmprow >> parname;
    if (parname.compare("BANK")) {
      for (size_t icol = 0; icol < numbanks; ++icol) {
        double tmpdbl;
        tmprow >> tmpdbl;
        vec_maptemp[icol].insert(make_pair(parname, tmpdbl));
      }
    } else {
      for (size_t icol = 0; icol < numbanks; ++icol) {
        double tmpint;
        tmprow >> tmpint;
        vecbankindex[icol] = static_cast<unsigned int>(tmpint);
      }
    }
  }

  // debug output
  stringstream db1ss;
  db1ss << "[DBx912] Number of banks in profile table = " << vecbankindex.size()
        << " containing bank ";
  for (size_t i = 0; i < vecbankindex.size(); ++i)
    db1ss << vecbankindex[i] << ", ";
  g_log.information(db1ss.str());

  // Construct output
  profilemap.clear();

  for (size_t i = 0; i < vecbankindex.size(); ++i) {
    unsigned int bankid = vecbankindex[i];
    profilemap.insert(make_pair(bankid, vec_maptemp[i]));
  }

  return;
}

//----------------------------------------------------------------------------------------------
/** Set up chopper/instrument constant parameters from profile map
  */
ChopperConfiguration_sptr SaveGSASInstrumentFile::setupInstrumentConstants(
    const map<unsigned int, map<string, double>> &profmap) {
  // Collect bank ids
  vector<int> bankids;
  map<unsigned int, map<string, double>>::const_iterator bmiter;
  for (bmiter = profmap.begin(); bmiter != profmap.end(); ++bmiter) {
    int bankid = bmiter->first;
    bankids.push_back(bankid);
  }

  // Create a configuration object
  ChopperConfiguration_sptr chconfig =
      boost::make_shared<ChopperConfiguration>(bankids);

  // Add chopper/instrument constants by banks
  for (bmiter = profmap.begin(); bmiter != profmap.end(); ++bmiter) {
    int bankid = bmiter->first;

    double cwl = getProfileParameterValue(bmiter->second, "CWL");
    double mintof = getProfileParameterValue(bmiter->second, "tof-min");
    double maxtof = getProfileParameterValue(bmiter->second, "tof-max");
    double dtt1 = getProfileParameterValue(bmiter->second, "Dtt1");
    double zero = getProfileParameterValue(bmiter->second, "Zero");

    double dmin = calDspRange(dtt1, zero, mintof);
    double dmax = calDspRange(dtt1, zero, maxtof);

    chconfig->setParameter(bankid, "CWL", cwl);
    chconfig->setParameter(bankid, "MaxTOF", maxtof);
    chconfig->setParameter(bankid, "MinDsp", dmin);
    chconfig->setParameter(bankid, "MaxDsp", dmax);

    g_log.information() << "Import bank " << bankid
                        << ".  TOF range: " << mintof << ", " << maxtof
                        << "; D-space range: " << dmin << ", " << dmax << ".\n";
  }

  return chconfig;
}

//----------------------------------------------------------------------------------------------
/** Set up the chopper/instrument constant parameters for PG3
  */
ChopperConfiguration_sptr
SaveGSASInstrumentFile::setupPG3Constants(int intfrequency) {
  string bankidstr, cwlstr, mndspstr, mxdspstr, maxtofstr;

  // Create string
  switch (intfrequency) {
  case 60:
    bankidstr = "1,2,3,4,5,6,7";
    cwlstr = "0.533, 1.066, 1.333, 1.599, 2.665, 3.731, 4.797";
    mndspstr = "0.10, 0.276, 0.414, 0.552, 1.104, 1.656, 2.208";
    mxdspstr = "2.06, 3.090, 3.605, 4.120, 6.180, 8.240, 10.30";
    maxtofstr = "46.76, 70.14, 81.83, 93.52, 140.3, 187.0, 233.8";

    break;
  case 30:
    bankidstr = "1,2,3";
    cwlstr = "1.066, 3.198, 5.33";
    mndspstr = "0.10, 1.104, 2.208";
    mxdspstr = "4.12, 8.24, 12.36";
    maxtofstr = "93.5, 187.0, 280.5";

    break;

  case 10:
    // Frequency = 10
    bankidstr = "1";
    cwlstr = "3.198";
    mndspstr = "0.10";
    mxdspstr = "12.36";
    maxtofstr = "280.5";

    break;

  default:
    throw runtime_error("Not supported");
    break;
  }

  // Return
  return boost::make_shared<ChopperConfiguration>(
      intfrequency, bankidstr, cwlstr, mndspstr, mxdspstr, maxtofstr);
}

//----------------------------------------------------------------------------------------------
/** Set up the converting constants for NOMAD
  * @param intfrequency :: frequency in integer
  */
ChopperConfiguration_sptr
SaveGSASInstrumentFile::setupNOMConstants(int intfrequency) {
  // Set up string
  string bankidstr, cwlstr, mndspstr, mxdspstr, maxtofstr;

  // FIXME : Requiring more banks
  switch (intfrequency) {
  case 60:
    bankidstr = "4,5";
    cwlstr = "1.500, 1.5000";
    mndspstr = "0.052, 0.0450";
    mxdspstr = "2.630, 2.6000";
    maxtofstr = "93.52, 156.00";
    break;

  default:
    stringstream errss;
    errss << "NOMAD Frequency = " << intfrequency << " is not supported. ";
    throw runtime_error(errss.str());
    break;
  }

  // Create configuration
  return boost::make_shared<ChopperConfiguration>(
      intfrequency, bankidstr, cwlstr, mndspstr, mxdspstr, maxtofstr);
}

//----------------------------------------------------------------------------------------------
/** Convert to GSAS instrument file
  * @param outputbankids : list of banks (sorted) to .iparm or prm file
  * @param gsasinstrfilename: string
  * @param bankprofilemap :: map containing each bank's profile parameter stored
 * in map.
  */
void SaveGSASInstrumentFile::convertToGSAS(
    const std::vector<unsigned int> &outputbankids,
    const std::string &gsasinstrfilename,
    const std::map<unsigned int, std::map<std::string, double>> &
        bankprofilemap) {
  // Check
  if (!m_configuration)
    throw runtime_error("Not set up yet!");

  // Set up min-dsp, max-tof
  for (size_t i = 0; i < outputbankids.size(); ++i) {
    unsigned int bankid = outputbankids[i];
    if (!m_configuration->hasBank(bankid))
      throw runtime_error(
          "Chopper configuration does not have some certain bank.");

    double mndsp = m_configuration->getParameter(bankid, "MinDsp");
    m_bank_mndsp.insert(make_pair(bankid, mndsp));
    double mxtof = m_configuration->getParameter(bankid, "MaxTOF");
    m_bank_mxtof.insert(make_pair(bankid, mxtof));
  }

  // Write bank header
  g_log.information() << "Export header of GSAS instrument file "
                      << gsasinstrfilename << ".\n";
  writePRMHeader(outputbankids, gsasinstrfilename);

  //  Convert and write
  vector<unsigned int> banks = outputbankids;
  sort(banks.begin(), banks.end());
  for (size_t ib = 0; ib < banks.size(); ++ib) {
    unsigned int bankid = banks[ib];
    if (m_configuration->hasBank(bankid)) {
      buildGSASTabulatedProfile(bankprofilemap, bankid);
      writePRMSingleBank(bankprofilemap, bankid, gsasinstrfilename);
    } else {
      vector<unsigned int> bankids = m_configuration->getBankIDs();
      stringstream errss;
      errss << "Bank " << bankid
            << " does not exist in source resolution file. "
            << "There are " << bankids.size() << " banks given, including "
            << ".\n";
      for (size_t i = 0; i < bankids.size(); ++i) {
        errss << bankids[i];
        if (i < bankids.size() - 1)
          errss << ", ";
      }
      g_log.error(errss.str());
      throw runtime_error(errss.str());
    }
  }

  return;
}

//----------------------------------------------------------------------------------------------
/** Build a data structure for GSAS"s tabulated peak profile
  * from Fullprof"s TOF peak profile
  * @param bankprofilemap :: map of all banks' peak profile that is stored in
 * map;
  * @param bankid :: the ID of the bank to be converted
  */
void SaveGSASInstrumentFile::buildGSASTabulatedProfile(
    const std::map<unsigned int, std::map<std::string, double>> &bankprofilemap,
    unsigned int bankid) {
  // Locate the profile map
  map<unsigned int, map<string, double>>::const_iterator biter =
      bankprofilemap.find(bankid);
  if (biter == bankprofilemap.end())
    throw runtime_error("Bank ID cannot be found in bank-profile-map-map. 001");
  const map<string, double> &profilemap = biter->second;

  // Init data structure
  // m_gdsp = gdsp;
  // m_gdt = gdt;
  // m_galpha = galpha;
  // m_gbeta = gbeta;

  m_gdsp.assign(90, 0.);   // TOF_thermal(d_k)
  m_galpha.assign(90, 0.); // delta(alpha)
  m_gbeta.assign(90, 0.);  // delta(beta)
  m_gdt.assign(90, 0.);

  vector<double> gtof(90, 0.); // TOF_thermal(d_k) - TOF(d_k)
  vector<double> gpkX(90, 0.); // ratio (n) b/w thermal and epithermal neutron

  // double twotheta = m_configuration->getParameter(bankid, "TwoTheta");
  double mx = getProfileParameterValue(profilemap, "Tcross");
  double mxb = getProfileParameterValue(profilemap, "Width");

  double zero = getProfileParameterValue(profilemap, "Zero");
  double zerot = getProfileParameterValue(profilemap, "Zerot");
  double dtt1 = getProfileParameterValue(profilemap, "Dtt1");
  double dtt1t = getProfileParameterValue(profilemap, "Dtt1t");
  double dtt2 = getProfileParameterValue(profilemap, "Dtt2");
  double dtt2t = getProfileParameterValue(profilemap, "Dtt2t");

  double alph0 = getProfileParameterValue(profilemap, "Alph0");
  double alph1 = getProfileParameterValue(profilemap, "Alph1");
  double alph0t = getProfileParameterValue(profilemap, "Alph0t");
  double alph1t = getProfileParameterValue(profilemap, "Alph1t");

  double beta0 = getProfileParameterValue(profilemap, "Beta0");
  double beta1 = getProfileParameterValue(profilemap, "Beta1");
  double beta0t = getProfileParameterValue(profilemap, "Beta0t");
  double beta1t = getProfileParameterValue(profilemap, "Beta1t");

  double instC = dtt1 - 4 * (alph0 + alph1);

  double mxdsp = m_configuration->getParameter(bankid, "MaxDsp");
  double mndsp = m_configuration->getParameter(bankid, "MinDsp");

  double ddstep = ((1.05 * mxdsp) - (0.9 * mndsp)) / 90.;

  for (size_t k = 0; k < 90; ++k) {
    m_gdsp[k] = (0.9 * mndsp) + (static_cast<double>(k) * ddstep);
    double rd = 1.0 / m_gdsp[k];
    double dmX = mx - rd;
    gpkX[k] = 0.5 * erfc(mxb * dmX); //  # this is n in the formula
    gtof[k] =
        calTOF(gpkX[k], zero, dtt1, dtt2, zerot, dtt1t, -dtt2t, m_gdsp[k]);
    m_gdt[k] = gtof[k] - (instC * m_gdsp[k]);
    m_galpha[k] = aaba(gpkX[k], alph0, alph1, alph0t, alph1t, m_gdsp[k]);
    m_gbeta[k] = aaba(gpkX[k], beta0, beta1, beta0t, beta1t, m_gdsp[k]);

    g_log.debug() << k << "\t" << setw(20) << setprecision(10) << gtof[k]
                  << "\t  " << setw(20) << setprecision(10) << m_gdsp[k]
                  << "\t  " << setw(20) << setprecision(10) << instC << "\t "
                  << setw(20) << setprecision(10) << m_gdt[k] << ".\n";
  }

  return;
}

//----------------------------------------------------------------------------------------------
/** Write the header of the file
  */
void SaveGSASInstrumentFile::writePRMHeader(const vector<unsigned int> &banks,
                                            const string &prmfilename) {
  int numbanks = static_cast<int>(banks.size());

  FILE *pFile;
  pFile = fopen(prmfilename.c_str(), "w");
  if (!pFile) {
    stringstream errss;
    errss << "Unable to open file " << prmfilename << " in write-mode";
    throw runtime_error(errss.str());
  }
  fprintf(pFile, "            "
                 "1234567890123456789012345678901234567890123456789012345678901"
                 "2345678\n");
  fprintf(pFile, "ID    %s\n", m_id_line.c_str());
  fprintf(pFile, "INS   BANK  %5d\n", numbanks);
  fprintf(pFile, "INS   FPATH1     %f \n", m_L1);
  fprintf(pFile, "INS   HTYPE   PNTR \n");
  fclose(pFile);

  return;
}

//----------------------------------------------------------------------------------------------
/** Write out .prm/.iparm file
  * @param bankid : integer as the ID of the bank to be written to file
  * @param bankprofilemap: map of all banks' profile parameters stored in map
  * @param prmfilename: output file name
  */
void SaveGSASInstrumentFile::writePRMSingleBank(
    const std::map<unsigned int, std::map<std::string, double>> &bankprofilemap,
    unsigned int bankid, const std::string &prmfilename) {
  // Get access to the profile map
  map<unsigned int, map<string, double>>::const_iterator biter =
      bankprofilemap.find(bankid);
  if (biter == bankprofilemap.end())
    throw runtime_error("Bank does not exist in bank-profile-map. 002");

  const map<string, double> &profilemap = biter->second;

  // Collect parameters used for output
  double zero = getProfileParameterValue(profilemap, "Zero");
  double dtt1 = getProfileParameterValue(profilemap, "Dtt1");
  double alph0 = getProfileParameterValue(profilemap, "Alph0");
  double alph1 = getProfileParameterValue(profilemap, "Alph1");
  double twotheta = getProfileParameterValue(profilemap, "twotheta");

  double sig0 = getProfileParameterValue(profilemap, "Sig0");
  sig0 = sig0 * sig0;
  double sig1 = getProfileParameterValue(profilemap, "Sig1");
  sig1 = sig1 * sig1;
  double sig2 = getProfileParameterValue(profilemap, "Sig2");
  sig2 = sig2 * sig2;
  double gam0 = getProfileParameterValue(profilemap, "Gam0");
  double gam1 = getProfileParameterValue(profilemap, "Gam1");
  double gam2 = getProfileParameterValue(profilemap, "Gam2");

  int randint = 10001 + (rand() % (int)(99999 - 10001 + 1));

  double mindsp = m_configuration->getParameter(bankid, "MinDsp");
  double maxtof = m_configuration->getParameter(bankid, "MaxTOF");
  double cwl = m_configuration->getParameter(bankid, "CWL");

  // Calculate L2
  double instC = dtt1 - (4 * (alph0 + alph1));
  g_log.debug() << "Bank " << bankid << ": MaxTOF = " << maxtof
                << "; Dtt1 = " << dtt1 << ", Alph0 = " << alph0
                << ", Alph1 = " << alph1 << ", MinDsp = " << mindsp << ".\n";

  if (m_L2 <= 0. || m_L2 == EMPTY_DBL()) {
    m_L2 = calL2FromDtt1(dtt1, m_L1, m_2theta);
  }

  // Title line
  stringstream titless;
  titless << m_sample << " " << static_cast<int>(m_frequency)
          << "Hz CW=" << cwl;
  string titleline(titless.str());

  // Write to file
  FILE *pFile;
  pFile = fopen(prmfilename.c_str(), "a");
  if (!pFile) {
    stringstream errss;
    errss << "Unable to open file " << prmfilename << " in append-mode";
    throw runtime_error(errss.str());
  }

  fprintf(pFile, "INS %2d ICONS%10.3f%10.3f%10.3f          %10.3f%5d%10.3f\n",
          bankid, instC * 1.00009, 0.0, zero, 0.0, 0, 0.0);
  fprintf(pFile, "INS %2dBNKPAR%10.3f%10.3f%10.3f%10.3f%10.3f%5d%5d\n", bankid,
          m_L2, twotheta, 0., 0., 0.2, 1, 1);

  fprintf(pFile, "INS %2dBAKGD     1    4    Y    0    Y\n", bankid);
  fprintf(pFile, "INS %2dI HEAD %s\n", bankid, titleline.c_str());
  fprintf(pFile, "INS %2dI ITYP%5d%10.4f%10.4f%10i\n", bankid, 0,
          mindsp * 0.001 * instC, maxtof, randint);
  fprintf(pFile, "INS %2dINAME   %s \n", bankid, m_instrument.c_str());
  fprintf(pFile, "INS %2dPRCF1 %5d%5d%10.5f\n", bankid, -3, 21, 0.002);
  fprintf(pFile, "INS %2dPRCF11%15.6f%15.6f%15.6f%15.6f\n", bankid, 0.0, 0.0,
          0.0, sig0);
  fprintf(pFile, "INS %2dPRCF12%15.6f%15.6f%15.6f%15.6f\n", bankid, sig1, sig2,
          gam0, gam1);
  fprintf(pFile, "INS %2dPRCF13%15.6f%15.6f%15.6f%15.6f\n", bankid, gam2, 0.0,
          0.0, 0.0);
  fprintf(pFile, "INS %2dPRCF14%15.6f%15.6f%15.6f%15.6f\n", bankid, 0.0, 0.0,
          0.0, 0.0);
  fprintf(pFile, "INS %2dPRCF15%15.6f%15.6f%15.6f%15.6f\n", bankid, 0.0, 0.0,
          0.0, 0.0);
  fprintf(pFile, "INS %2dPRCF16%15.6f\n", bankid, 0.0);
  fprintf(pFile, "INS %2dPAB3    %3d\n", bankid, 90);

  for (size_t k = 0; k < 90; ++k) {
    fprintf(pFile, "INS %2dPAB3%2d%10.5f%10.5f%10.5f%10.5f\n", bankid,
            static_cast<int>(k) + 1, m_gdsp[k], m_gdt[k], m_galpha[k],
            m_gbeta[k]);
  }
  fprintf(pFile, "INS %2dPRCF2 %5i%5i%10.5f\n", bankid, -4, 27, 0.002);
  fprintf(pFile, "INS %2dPRCF21%15.6f%15.6f%15.6f%15.6f\n", bankid, 0.0, 0.0,
          0.0, sig1);
  fprintf(pFile, "INS %2dPRCF22%15.6f%15.6f%15.6f%15.6f\n", bankid, sig2, gam2,
          0.0, 0.0);
  fprintf(pFile, "INS %2dPRCF23%15.6f%15.6f%15.6f%15.6f\n", bankid, 0.0, 0.0,
          0.0, 0.0);
  fprintf(pFile, "INS %2dPRCF24%15.6f%15.6f%15.6f%15.6f\n", bankid, 0.0, 0.0,
          0.0, 0.0);
  fprintf(pFile, "INS %2dPRCF25%15.6f%15.6f%15.6f%15.6f\n", bankid, 0.0, 0.0,
          0.0, 0.0);
  fprintf(pFile, "INS %2dPRCF26%15.6f%15.6f%15.6f%15.6f\n", bankid, 0.0, 0.0,
          0.0, 0.0);
  fprintf(pFile, "INS %2dPRCF27%15.6f%15.6f%15.6f \n", bankid, 0.0, 0.0, 0.0);

  fprintf(pFile, "INS %2dPAB4    %3i\n", bankid, 90);
  for (size_t k = 0; k < 90; ++k) {
    fprintf(pFile, "INS %2dPAB4%2d%10.5f%10.5f%10.5f%10.5f\n", bankid,
            static_cast<int>(k) + 1, m_gdsp[k], m_gdt[k], m_galpha[k],
            m_gbeta[k]);
  }

  fprintf(pFile, "INS %2dPRCF3 %5i%5i%10.5f\n", bankid, -5, 21, 0.002);
  fprintf(pFile, "INS %2dPRCF31%15.6f%15.6f%15.6f%15.6f\n", bankid, 0.0, 0.0,
          0.0, sig0);
  fprintf(pFile, "INS %2dPRCF32%15.6f%15.6f%15.6f%15.6f\n", bankid, sig1, sig2,
          gam0, gam1);
  fprintf(pFile, "INS %2dPRCF33%15.6f%15.6f%15.6f%15.6f\n", bankid, gam2, 0.0,
          0.0, 0.0);
  fprintf(pFile, "INS %2dPRCF34%15.6f%15.6f%15.6f%15.6f\n", bankid, 0.0, 0.0,
          0.0, 0.0);
  fprintf(pFile, "INS %2dPRCF35%15.6f%15.6f%15.6f%15.6f\n", bankid, 0.0, 0.0,
          0.0, 0.0);
  fprintf(pFile, "INS %2dPRCF36%15.6f\n", bankid, 0.0);

  fprintf(pFile, "INS %2dPAB5    %3i\n", bankid,
          90); // 90 means there will be 90 lines of table
  for (size_t k = 0; k < 90; k++) {
    fprintf(pFile, "INS %2dPAB5%2d%10.5f%10.5f%10.5f%10.5f\n", bankid,
            static_cast<int>(k) + 1, m_gdsp[k], m_gdt[k], m_galpha[k],
            m_gbeta[k]);
  }

  fclose(pFile);

  return;
}

//----------------------------------------------------------------------------------------------
/** Calculate L2 from DIFFC and L1
  * DIFC = 252.816*2sin(theta)sqrt(L1+L2)
  */
double SaveGSASInstrumentFile::calL2FromDtt1(double difc, double L1,
                                             double twotheta) {
  double l2 = difc / (252.816 * 2.0 * sin(0.5 * twotheta * M_PI / 180.0)) - L1;
  g_log.debug() << "DIFC = " << difc << ", L1 = " << L1
                << ", 2Theta = " << twotheta << " ==> L2 = " << l2 << ".\n";

  return l2;
}

//----------------------------------------------------------------------------------------------
/** Calculate TOF from d-space value of thermal neutron back-to-back exponential
 * conv. pseudo-voigt
  * Epithermal: te = zero  + d*dtt1  + 0.5*dtt2*erfc( (1/d-1.05)*10 );
  * Thermal:    tt = zerot + d*dtt1t + dtt2t/d;
  * Total TOF:  t  = n*te + (1-n) tt
  * @param n :: ratio between thermal neutron and epithermal neutron
  * @param ep :: zero
  * @param eq :: dtt1
  * @param er :: dtt2
  * @param tp :: zerot
  * @param tq :: dtt1t
  * @param tr :: dtt2t
  * @param dsp :: d-space value
*/
double SaveGSASInstrumentFile::calTOF(double n, double ep, double eq, double er,
                                      double tp, double tq, double tr,
                                      double dsp) {
  double te = ep + (eq * dsp) + er * 0.5 * erfc(((1.0 / dsp) - 1.05) * 10.0);
  double tt = tp + (tq * dsp) + (tr / dsp);
  double t = (n * te) + tt - (n * tt);

  return t;
}

//----------------------------------------------------------------------------------------------
/** Calculate a value related to alph0, alph1, alph0t, alph1t or
  * beta0, beta1, beta0t, beta1t
  */
double SaveGSASInstrumentFile::aaba(double n, double ea1, double ea2,
                                    double ta1, double ta2, double dsp) {
  double ea = ea1 + (ea2 * dsp);
  double ta = ta1 - (ta2 / dsp);
  double am1 = (n * ea) + ta - (n * ta);
  double a = 1.0 / am1;

  return a;
}

//----------------------------------------------------------------------------------------------
/** Get parameter value from a map
  */
double
SaveGSASInstrumentFile::getValueFromMap(const map<string, double> &profilemap,
                                        const string &parname) {
  std::map<std::string, double>::const_iterator piter;
  piter = profilemap.find(parname);
  if (piter == profilemap.end()) {
    stringstream errss;
    errss << "Profile parameter map does not contain parameter" << parname
          << ". ";
    g_log.error(errss.str());
    throw runtime_error(errss.str());
  }

  double value = piter->second;

  return value;
}

//----------------------------------------------------------------------------------------------
/** Get parameter value from m_configuration/m_profile
  */
double SaveGSASInstrumentFile::getProfileParameterValue(
    const map<string, double> &profilemap, const string &paramname) {
  map<string, double>::const_iterator piter = profilemap.find(paramname);
  if (piter == profilemap.end()) {
    stringstream errss;
    errss << "Profile map does not contain parameter " << paramname
          << ". Available parameters are ";
    for (map<string, double>::const_iterator piter = profilemap.begin();
         piter != profilemap.end(); ++piter) {
      errss << piter->first << ", ";
    }
    g_log.error(errss.str());
    throw runtime_error(errss.str());
  }

  double value = piter->second;

  return value;
}

//----------------------------------------------------------------------------------------------
/** Calcualte range of data on d-spacing from TOF.
  * Algorithm: use the approximation from Dtt1.  The part with thermal neutron
 * has complex operation
  * on d ralated.
  * @return :: d-space value
 */
double SaveGSASInstrumentFile::calDspRange(double dtt1, double zero,
                                           double tof) {
  double dsp = (tof - zero) / dtt1;
  return dsp;
}

//----------------------------------------------------------------------------------------------
/** Load fullprof resolution file.
  * - call LoadFullprofResolution
  * - set output table workspace to m_inpWS
  * @param irffilename
  */
void
SaveGSASInstrumentFile::loadFullprofResolutionFile(std::string irffilename) {
  IAlgorithm_sptr loadfpirf;
  try {
    loadfpirf = createChildAlgorithm("LoadFullprofResolution");
  } catch (Exception::NotFoundError &) {
    g_log.error("SaveGSASInstrumentFile requires DataHandling library for "
                "LoadFullprofResolution.");
    throw runtime_error("SaveGSASInstrumentFile requires DataHandling library "
                        "for LoadFullprofResolution.");
  }

  loadfpirf->setProperty("Filename", irffilename);

  loadfpirf->execute();
  if (!loadfpirf->isExecuted())
    throw runtime_error("LoadFullprof cannot be executed. ");

  m_inpWS = loadfpirf->getProperty("OutputTableWorkspace");
  if (!m_inpWS)
    throw runtime_error("Failed to obtain a table workspace from "
                        "LoadFullprofResolution's output.");

  return;
}

//----------------------------------------------------------------------------------------------
/** Complementary error function
*/
double SaveGSASInstrumentFile::erfc(double xx) {
  double x = fabs(xx);
  double t = 1.0 / (1.0 + (0.5 * x));
  double ty = (0.27886807 +
               t * (-1.13520398 +
                    t * (1.48851587 + t * (-0.82215223 + t * 0.17087277))));
  double tx =
      (1.00002368 +
       t * (0.37409196 + t * (0.09678418 + t * (-0.18628806 + t * ty))));
  double y = t * exp(-x * x - 1.26551223 + t * tx);
  if (xx < 0)
    y = 2.0 - y;

  return y;
}

} // namespace Algorithms
} // namespace Mantid
