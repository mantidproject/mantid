/** *WIKI*

Convert Fullprof's instrument resolution file (.irf) to  GSAS's instrument file (.iparm/.prm).

==== Supported peak profiles ====
* Time-of-flight back-to-back exponential convoluted with pseudo-voigt (planned)
** Fullprof: Profile 9;
** GSAS: Type 3 TOF profile.

* Thermal neutron time-of-flight back-to-back exponential convoluted with pseudo-voigt (implemented)
** Fullprof: Profile 10;
** GSAS: tabulated peak profile.

==== Supported input Fullprof file ====
There can be several types of Fullprof files as the input file
* resolution file .irf (implemented)
* configuration file .pcr (planned)

==== Calculation of L2 ====
* If 2Theta (<math>2\theta</math>) is given, L2 will be calculated from given 2Theta and L1 by <math>DIFC = 252.816\cdot2sin(\theta)\sqrt{L1+L2}</math>. Notice that <math>2\theta</math> given in input .irf file may have subtle difference to "2Theta", which is input by user in order to calculate L2.

* If "2Theta" (<math>2\theta</math>) is not given, L2 will be read from user input.


*WIKI*
*/

#include "MantidAlgorithms/SaveGSASInstrumentFile.h"
#include "MantidAPI/FileProperty.h"
#include "MantidKernel/ListValidator.h"
#include "MantidKernel/ArrayProperty.h"

using namespace Mantid;
using namespace Mantid::API;
using namespace Mantid::Kernel;
using namespace Mantid::DataObjects;

using namespace std;

const double PI = 3.14159265;

namespace Mantid
{
namespace Algorithms
{

  DECLARE_ALGORITHM(SaveGSASInstrumentFile)

  //----------------------------------------------------------------------------------------------
  /** Constructor of chopper configuration
    * Removed arguments: std::string splitdstr, std::string vrunstr
    */
  ChopperConfiguration::ChopperConfiguration(double freq, string bankidstr, string cwlstr, string mndspstr, string mxdspstr,
                                             string maxtofstr)
  {
    // Import and set up from default constants
    m_frequency = freq;

    m_bankIDs = parseStringUnsignedInt(bankidstr);
    size_t numbanks = m_bankIDs.size();

    m_vecCWL = parseStringDbl(cwlstr);
    m_mindsps = parseStringDbl(mndspstr);
    m_maxdsps = parseStringDbl(mxdspstr);
    m_maxtofs = parseStringDbl(maxtofstr);

#if 0
    m_splitds = parseStringDbl(splitdstr);
    m_vruns = parseStringInt(vrunstr);
#endif

    // Check size
    if (m_vecCWL.size() != numbanks || m_vecCWL.size() != numbanks || m_vecCWL.size() != numbanks)
    {
      stringstream errss;
      errss << "Default chopper constants have different number of elements. ";
      throw runtime_error(errss.str());
    }

    // Set up index map
    m_vec2Theta.resize(numbanks, 0.);
    m_vecL1.resize(numbanks, 0.);
    m_vecL2.resize(numbanks, 0.);


    // Set up bank ID / looking up index map
    m_bankIDIndexMap.clear();
    for (size_t ib = 0; ib < numbanks; ++ib)
    {
      m_bankIDIndexMap.insert(make_pair(m_bankIDs[ib], ib));
    }

    return;
  }

  //----------------------------------------------------------------------------------------------
  /** Get bank IDs in the chopper configuration
    */
  vector<unsigned int> ChopperConfiguration::getBankIDs()
  {
    vector<unsigned int> bids;
    // FIXME : use sorphisticated vector methods to replace the below!
    for (size_t i = 0; i < m_bankIDs.size(); ++i)
    {
      bids.push_back(m_bankIDs[i]);
    }

    return bids;
  }

  //----------------------------------------------------------------------------------------------
  bool ChopperConfiguration::hasBank(unsigned int bankid)
  {
    bool hasbank = false;
    for (size_t i = 0; i < m_bankIDs.size(); ++i)
      if (bankid == m_bankIDs[i])
      {
        hasbank = true;
        break;
      }

    return hasbank;
  }

  //----------------------------------------------------------------------------------------------
  double ChopperConfiguration::getParameter(unsigned int bankid, string paramname)
  {
    // Check bank IDs
    if (!hasBank(bankid))
    {
      stringstream errss;
      errss << "ChopperConfiguration does not have bank " << bankid;
      throw runtime_error(errss.str());
    }

    // Obtain index for the bank
    map<unsigned int, size_t>::iterator biter = m_bankIDIndexMap.find(bankid);
    if (biter == m_bankIDIndexMap.end())
    {
      stringstream errss;
      errss << "Bank ID and index map does not have entry for bank " << bankid;
      throw runtime_error(errss.str());
    }
    size_t bindex = biter->second;

    double value(EMPTY_DBL());
    if (paramname.compare("TwoTheta") == 0)
      value = m_vec2Theta[bindex];
    else
    {
      // FIXME : Remove Fake Return ASAP!
      stringstream errss;
      errss << "Bank ID : " << bankid << ", Parameter : " << paramname;
      cout << "Error: " << errss.str() << "\n[Fake Return] " << ".\n";
      return 100.;
      // throw runtime_error(errss.str());
    }

    return value;
  }

  //----------------------------------------------------------------------------------------------
  /** Set a parameter to a bank
    */
  void ChopperConfiguration::setParameter(unsigned int bankid, string paramname, double value)
  {
    map<unsigned, size_t>::iterator biter = m_bankIDIndexMap.find(bankid);

    if (biter == m_bankIDIndexMap.end())
    {
      stringstream errss;
      errss << "Chopper configuration does not have bank " << bankid;
      throw runtime_error(errss.str());
    }
    else
    {
      size_t ibank = biter->second;
      if (paramname.compare("2Theta") == 0)
        m_vec2Theta[ibank] = value;
      else if (paramname.compare("L1") == 0)
        m_vecL1[ibank] = value;
      else if (paramname.compare("L2") == 0)
        m_vecL2[ibank] = value;
      else
      {
        stringstream errss;
        errss << "In Chopper configuration's bank " << bankid << ", there is no parameter named "
              << paramname;
        throw runtime_error(errss.str());
      }
    }

    return;
  }

  //----------------------------------------------------------------------------------------------
  /** Parse string to double vector
    */
  vector<double> ChopperConfiguration::parseStringDbl(string instring)
  {
    vector<string> strs;
    boost::split(strs, instring, boost::is_any_of(", "));

    vector<double> vecdouble;
    for (size_t i = 0; i < strs.size(); i++)
    {
      if (strs[i].size() > 0)
      {
        double item = atof(strs[i].c_str());
        vecdouble.push_back(item);
        // cout << "[C] |" << strs[i] << "|" << item << "\n";
      }
    }

    cout << "[C]* Input: " << instring << ": size of double vector: " << vecdouble.size() << endl;

    return vecdouble;
  }

  //----------------------------------------------------------------------------------------------
  /** Parse string to double vector
    */
  vector<unsigned int> ChopperConfiguration::parseStringUnsignedInt(string instring)
  {
    vector<string> strs;
    boost::split(strs, instring, boost::is_any_of(", "));

    vector<unsigned int> vecinteger;
    for (size_t i = 0; i < strs.size(); i++)
    {
      if (strs[i].size() > 0)
      {
        int item = atoi(strs[i].c_str());
        if (item < 0)
        {
          throw runtime_error("Found negative number in a string for unsigned integers.");
        }
        vecinteger.push_back(static_cast<unsigned int>(item));
      }
    }

    cout << "[C]* Input : " << instring << ": size of string vector: " << vecinteger.size() << endl;

    return vecinteger;
  }


  //----------------------------------------------------------------------------------------------
  /** Constructor
   */
  SaveGSASInstrumentFile::SaveGSASInstrumentFile()
  {
  }
    
  //----------------------------------------------------------------------------------------------
  /** Destructor
   */
  SaveGSASInstrumentFile::~SaveGSASInstrumentFile()
  {
  }

  //----------------------------------------------------------------------------------------------
  void SaveGSASInstrumentFile::initDocs()
  {
    setWikiSummary("Save a instrument parameter table workspace to GSAS instrument file.");
    setOptionalMessage("");
  }

  //----------------------------------------------------------------------------------------------
  /** Declare properties
    */
  void SaveGSASInstrumentFile::init()
  {
    declareProperty(new WorkspaceProperty<TableWorkspace>("InputWorkspace", "", Direction::Input),
                    "Name of the table workspace containing the parameters.  Usually it is generated by "
                    "LoadFullprofResolution.");

    vector<string> exts;
    exts.push_back(".iparam");
    exts.push_back(".prm");
    auto fileprop = new FileProperty("OutputFileName", "", FileProperty::Save, exts);
    declareProperty(fileprop, "Name of the output GSAS instrument file.");

    declareProperty(new ArrayProperty<unsigned int>("BankIDs"), "Bank IDs of the banks to be written to GSAS instrument file.");


    vector<string> instruments;
    instruments.push_back("PG3");
    instruments.push_back("NOM");
    instruments.push_back("VULCAN");
    // auto instrumentval = new ListValidator<string>(instruments);
    declareProperty("Instrument", "PG3", boost::make_shared<StringListValidator>(instruments), "Name of the instrument that parameters are belonged to. ");

    vector<string> vecfreq;
    vecfreq.push_back("10");
    vecfreq.push_back("30");
    vecfreq.push_back("60");
    declareProperty("ChopperFrequency", "60", boost::make_shared<StringListValidator>(vecfreq), "Frequency of the chopper. ");

    declareProperty("IDLine", "", "ID line to be written in GSAS instrumetn file.");
    declareProperty("Sample", "", "Name of the sample used to calibrate the instrument parameters. ");

    declareProperty("L1", EMPTY_DBL(), "L1 (primary flight path) of the instrument. ");
    declareProperty("L2", EMPTY_DBL(), "L2 (secondary flight path) of the insturment. ");
    declareProperty("TwoTheta", EMPTY_DBL(), "Angle of the detector bank. ");

    return;
  }

  //----------------------------------------------------------------------------------------------
  /** Process input properties
   */
  void SaveGSASInstrumentFile::processProperties()
  {
    m_inpWS = getProperty("InputWorkspace");

    m_instrument = getPropertyValue("Instrument");
    m_id_line = getPropertyValue("IDLine"); // Standard Run LB4844 Vanadium: 4866 J.P. Hodges 2011-09-01
    m_sample = getPropertyValue("Sample"); // titleline = "LaB6 NIST RT 4844[V=4866] 60Hz CW=.533"

    m_gsasFileName = getPropertyValue("OutputFileName");
    m_bankIDsOutput = getProperty("BankIDs");

    m_L1 = getProperty("L1");
    m_2theta = getProperty("TwoTheta");
    m_L2 = getProperty("L2");
    string freqtempstr = getProperty("ChopperFrequency");
    m_frequency = atoi(freqtempstr.c_str());

#if 0
    // Process input
    bool useirf = false;
    string irffilename, pcrfilename;
    if (endswith(inputfilename, ".irf"))
    {
      useirf = true;
      irffilename = inputfilename;
    }
    else
    {
      pcrfilename = inputfilename;
    }
#endif

    // Set default value for L1
    if (m_L1 == EMPTY_DBL())
    {
      if (m_instrument.compare("PG3") == 0)
      {
        m_L1 = 60.0;
      }
      else if (m_instrument.compare("NOM") == 0)
      {
        m_L1 = 19.5;
      }
      else
      {
        stringstream errss;
        errss << "L1 is not given. There is no default value for instrument " << m_instrument << ".\n";
        g_log.error(errss.str());
        throw runtime_error(errss.str());
      }
    }
    else if (m_L1 <= 0.)
    {
      throw runtime_error("Input L1 cannot be less or equal to 0.");
    }

    // Set default value for L2
    if (m_2theta == EMPTY_DBL())
    {
      if (m_L2 == EMPTY_DBL())
      {
        string errmsg("User must specify either 2theta or L2.  Neither of them is given.");
        g_log.error(errmsg);
        throw runtime_error(errmsg);
      }
    }
    else
    {
      // Override L2 by 2theta
      m_L2 = EMPTY_DBL();
    }

  }

  //----------------------------------------------------------------------------------------------
  /** Main execution body
    */
  void SaveGSASInstrumentFile::exec()
  {
    // Process user specified properties
    processProperties();

    // Execute
    initConstants(m_frequency);

#if 0
    This might be a legacy
    if (useirf)
    {
      parseFullprofResolutionFile(irffilename);
    }
    else
    {
      throw runtime_error("Parsing .pcr file has not been implemented yet.");
    }
#endif

    makeParameterConsistent();

    convertToGSAS(m_bankIDsOutput, m_gsasFileName);

    return;
  }

  //----------------------------------------------------------------------------------------------
  /** Set up some constant by default
    * Output--> m_configuration
    * @param chopperfrequency :: chopper frequency of the profile for.
    */
  void SaveGSASInstrumentFile::initConstants(double chopperfrequency)
  {
    if (m_instrument.compare("PG3"))
    {
      m_configuration = setupPG3Constants(static_cast<int>(chopperfrequency));
    }
    else if (m_instrument.compare("NOM"))
    {
      m_configuration = setupNOMConstants(static_cast<int>(chopperfrequency));
    }
    else
    {
      throw runtime_error("Instrument is not supported");
    }

    return;
  }

  //----------------------------------------------------------------------------------------------
  /** Some parameter can come from 3 sources (1) default in code; (2) input table workspace and
    * (3) user-specified property.  Set up this kind of parameter from all possible source
    * with proper prioirty
    */
  void SaveGSASInstrumentFile::makeParameterConsistent()
  {
    // Parse input profile table workspace
    map<string, double> profileparammap = parseProfileTableWorkspace(m_inpWS);

    // 2-theta
    // FIXME - Figure out what (bank ID) to set up!
    unsigned int fake4(4);
    if (m_2theta != EMPTY_DBL())
    {
      m_configuration->setParameter(fake4, "2Theta", m_2theta);
    }
    else
    {
      double tth = getParameterValue(profileparammap, "twotheta");
      if (tth != EMPTY_DBL())
        m_configuration->setParameter(fake4,"2Theta", tth);
    }

    // L1
    if (m_L1 != EMPTY_DBL())
    {
      m_configuration->setParameter(fake4, "L1", m_L1);
    }

    // L2
    if (m_L2 != EMPTY_DBL())
    {
      m_configuration->setParameter(fake4, "L2", m_L2);
    }

    // TODO :: Implement ASAP
    // throw runtime_error("ASAP");
    g_log.error("How to make parameter consistent? Which parameters should be consistent? ");

    return;
  }

  //----------------------------------------------------------------------------------------------
  /** Parse profile table workspace to a map
    */
  map<string, double> SaveGSASInstrumentFile::parseProfileTableWorkspace(TableWorkspace_sptr ws)
  {
    // TODO - Implement ASAP
    // throw runtime_error("Haven't implemented yet.");
    g_log.error("Faking now [1]!");

    map<string, double> profilemap;


    return profilemap;
  }

  //----------------------------------------------------------------------------------------------
  /** Set up the chopper/instrument constant parameters for PG3
    */
  ChopperConfiguration_sptr SaveGSASInstrumentFile::setupPG3Constants(int intfrequency)
  {
    string bankidstr, cwlstr, mndspstr, mxdspstr, maxtofstr;

    // Create string
    switch (intfrequency)
    {
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

    // Create configuration
    ChopperConfiguration conf(static_cast<double>(intfrequency), bankidstr, cwlstr, mndspstr, mxdspstr, maxtofstr);

    ChopperConfiguration_sptr confsptr = boost::make_shared<ChopperConfiguration>(conf);

    return confsptr;
  }

  //----------------------------------------------------------------------------------------------
  /** Set up the converting constants for NOMAD
    * @param intfrequency :: frequency in integer
    */
  ChopperConfiguration_sptr SaveGSASInstrumentFile::setupNOMConstants(int intfrequency)
  {
    // Set up string
    string bankidstr, cwlstr, mndspstr, mxdspstr, maxtofstr;

    // FIXME : Requiring more banks
    switch (intfrequency)
    {
      case 60:
        bankidstr = "4,5";
        cwlstr = "1.500, 1.5000";
        mndspstr = "0.052, 0.0450";
        mxdspstr = "2.630, 2.6000";
        maxtofstr = "93.52, 156.00";
        break;

      default:
        throw runtime_error("Not supported");
        break;
    }

    // Create configuration
    ChopperConfiguration conf(static_cast<float>(intfrequency), bankidstr, cwlstr, mndspstr, mxdspstr, maxtofstr);
    ChopperConfiguration_sptr confsptr = boost::make_shared<ChopperConfiguration>(conf);

    return confsptr;
  }

  //----------------------------------------------------------------------------------------------
  /** Convert to GSAS instrument file
    * @param banks : list of banks (sorted) to .iparm or prm file
    * @param gsasinstrfilename: string
    */
  void SaveGSASInstrumentFile::convertToGSAS(vector<unsigned int> banks, string gsasinstrfilename)
  {
    if (!m_configuration)
      throw runtime_error("Not set up yet!");


    //  Convert and write
    sort(banks.begin(), banks.end());
    bool isfirstbank = true;
    for (size_t ib = 0; ib < banks.size(); ++ib)
    {
      int bankid = banks[ib];
      if (m_configuration->hasBank(bankid))
      {
        buildGSASTabulatedProfile(bankid);
        writePRM(bankid, banks.size(), gsasinstrfilename, isfirstbank);
        isfirstbank = false;
      }
      else
      {
        vector<unsigned int> bankids = m_configuration->getBankIDs();
        stringstream errss;
        errss << "Bank " << bankid << " does not exist in source resolution file. "
              << "There are " << bankids.size() << " banks given, including " << ".\n";
        for (size_t i = 0; i < bankids.size(); ++i)
        {
          errss << bankids[i];
          if (i < bankids.size()-1)
            errss << ", ";
        }
        g_log.error(errss.str());
        throw runtime_error(errss.str());
      }
    }

    return;
  }

  //----------------------------------------------------------------------------------------------
  /** Build a data structure for GSAS's tabulated peak profile
     * from Fullprof's TOF peak profile

  Note:
  - gdsp[k] : d_k as the tabulated d-spacing value
  -

    @param pardict ::
  */
  void SaveGSASInstrumentFile::buildGSASTabulatedProfile(unsigned int bankid)
  {
    // Init data structure
    vector<double> gdsp(90, 0.);   // TOF_thermal(d_k)
    vector<double> gtof(90, 0.);   // TOF_thermal(d_k) - TOF(d_k)
    vector<double> galpha(90, 0.); // delta(alpha)
    vector<double> gbeta(90, 0.);  // delta(beta)
    vector<double> gdt(90);
    vector<double> gpkX(90, 0.);   // ratio (n) b/w thermal and epithermal neutron

    double twotheta = m_configuration->getParameter(bankid, "TwoTheta");
    double mx = m_configuration->getParameter(bankid, "Tcross");
    double mxb = m_configuration->getParameter(bankid, "Width");

    double zero = m_configuration->getParameter(bankid, "Zero");
    double zerot = m_configuration->getParameter(bankid, "Zerot");
    double dtt1 = m_configuration->getParameter(bankid, "Dtt1");
    double dtt1t = m_configuration->getParameter(bankid, "Dtt1t");
    double dtt2 = m_configuration->getParameter(bankid, "Dtt2");

    double alph0 = m_configuration->getParameter(bankid, "Alph0");
    double alph1 = m_configuration->getParameter(bankid, "Alph1");
    double alph0t = m_configuration->getParameter(bankid, "Alph0t");
    double alph1t = m_configuration->getParameter(bankid, "Alph1t");

    double beta0 = m_configuration->getParameter(bankid, "Beta0");
    double beta1 = m_configuration->getParameter(bankid, "Beat1");
    double beta0t = m_configuration->getParameter(bankid, "Beta0t");
    double beta1t = m_configuration->getParameter(bankid, "Beta1t");

    double instC = dtt1 - 4*(alph0+alph1);

    double mxdsp = m_configuration->getParameter(bankid, "MaxDsp");
    double mndsp = m_configuration->getParameter(bankid, "MinDsp");


    double ddstep = ((1.05*mxdsp)-(0.9*mndsp))/90.;

    for (size_t k = 0; k < 90; ++k)
    {
      gdsp[k] = (0.9*mndsp)+(static_cast<double>(k)*ddstep);
      double rd = 1.0/gdsp[k];
      double dmX = mx-rd;
      gpkX[k] = 0.5*erfc(mxb*dmX); //  # this is n in the formula
      gtof[k] = calTOF(gpkX[k], zero, dtt1 ,dtt2, zerot, dtt1t, -dtt2, gdsp[k]);
      gdt[k] = gtof[k] - (instC*gdsp[k]);
      galpha[k] = aaba(gpkX[k], alph0, alph1, alph0t, alph1t, gdsp[k]);
      gbeta[k] = aaba(gpkX[k], beta0, beta1, beta0t, beta1t, gdsp[k]);
    }

    // 3. Set to class variables 
    m_gdsp = gdsp;
    m_gdt = gdt;
    m_galpha = galpha;
    m_gbeta = gbeta;

    /** To translate
    gdsp = np.zeros(90) # d_k
          gtof = np.zeros(90) # TOF_thermal(d_k)
          gdt = np.zeros(90) # TOF_thermal(d_k) - TOF(d_k)
          galpha = np.zeros(90) # delta(alpha)
          gbeta = np.zeros(90) # delta(beta)
          gpkX = np.zeros(90) # n ratio b/w thermal and epithermal neutron
          try:
              twosintheta = pardict["twotheta"]
              mX = pardict["Tcross"]
              mXb = pardict["Width"]
              instC = pardict["Dtt1"] - (4*(pardict["Alph0"]+pardict["Alph1"]))
          except KeyError:
              print "Cannot Find Key twotheta/x-cross/width/dtt1/alph0/alph1!"
              print "Keys are: "
              print pardict.keys()
              raise NotImplementedError("Key works cannot be found!")

          if 1:
              # latest version from Jason
              ddstep = ((1.05*self.mxdsp[bank-1])-(0.9*self.mndsp[bank-1]))/90
          else:
              # used in the older prm file
              ddstep = ((1.00*self.mxdsp[bank-1])-(0.9*self.mndsp[bank-1]))/90

          # 2. Calcualte alph, beta table
          for k in xrange(90):
              #try:
              gdsp[k] = (0.9*self.mndsp[bank-1])+(k*ddstep)
              rd = 1.0/gdsp[k]
              dmX = mX-rd
              gpkX[k] = 0.5*erfc(mXb*dmX) # this is n in the formula
              gtof[k] = tofh(gpkX[k], pardict["Zero"], pardict["Dtt1"] ,pardict["Dtt2"],
                      pardict["Zerot"], pardict["Dtt1t"], -pardict["Dtt2t"], gdsp[k])
              gdt[k] = gtof[k] - (instC*gdsp[k])
              galpha[k] = aaba(gpkX[k], pardict["Alph0"], pardict["Alph1"],
                      pardict["Alph0t"], pardict["Alph1t"], gdsp[k])
              gbeta[k] = aaba(gpkX[k], pardict["Beta0"], pardict["Beta1"],
                      pardict["Beta0t"], pardict["Beta1t"], gdsp[k])
              #except KeyError err:
              # print err
              # raise NotImplementedError("Unable to find some parameter name as key")
          # ENDFOR: k

          # 3. Set to class variables
          self.gdsp = gdsp
          self.gdt = gdt
          self.galpha = galpha
          self.gbeta = gbeta
        */

    return;
  }


  //----------------------------------------------------------------------------------------------
  /** Write out .prm/.iparm file
    * @bankid : integer, bank ID
    * @numbanks: integer, total number of banks written in file
    * @prmfilename: output file name
    * @isfirstbank: bool
    */
  void SaveGSASInstrumentFile::writePRM(unsigned int bankid, size_t numbanks, std::string prmfilename, bool isfirstbank)
  {
    double zero = m_configuration->getParameter(bankid, "Zero");
    double dtt1 = m_configuration->getParameter(bankid, "Dtt1");
    double alph0 = m_configuration->getParameter(bankid, "Alph0");
    double alph1 = m_configuration->getParameter(bankid, "Alph1");
    double twotheta = m_configuration->getParameter(bankid, "TwoTheta");
    double cwl = m_configuration->getParameter(bankid, "CWL");

    // Calculate
    double instC = dtt1 - (4*(alph0+alph1));
    if (m_L2 <= 0. || m_L2 == EMPTY_DBL())
    {
      m_L2 = calL2FromDtt1(dtt1, m_L1, m_2theta);
    }

    stringstream titless;
    titless << m_sample << " " << static_cast<int>(m_frequency) << "Hz CW=" << cwl;
    string titleline(titless.str());

    // Build the output
    stringstream bufss;

    // Header
    bufss << " 12345678901234567890123456789012345678901234567890123456789012345678\n";
    if (isfirstbank)
    {
      bufss << "ID " << m_id_line << "\n"
             <<  "INS BANK " << numbanks << "\n"
              << "INS FPATH1 " << m_L1 << "\n"
              << "INS HTYPE PNTR \n";
    }

    printf("INS %2d ICONS%10.3f%10.3f%10.3f%10.3f%5d%10.3f\n", bankid, instC*1.00009, 0.0, zero,0.0, 0, 0.0);
    printf("INS %2dBNKPAR%10.3f%10.3f%10.3f%10.3f%10.3f%5d%5d\n", bankid, m_L2, twotheta, 0., 0., 0.2, 1, 1);


    // FIXME ::
    throw runtime_error("Test (compile and run) to use fprintf() in Mantid");

    return;

    // printf('INS %2dBAKGD 1 4 Y 0 Y\n', bankid);
    // printf('INS %2dI HEAD %s\n', bankid, titleline);



    /** To translate


  # print "Debug: L2 = %f, 2Theta (irf) = %f, 2Theta (input) = %f" % (self.iL2, pardict["twotheta"], self.i2theta)

  prmfile += ('INS %2i ICONS%10.3f%10.3f%10.3f %10.3f%5i%10.3f\n' %
  (bank, instC*1.00009, 0.0, pardict["Zero"],0.0,0,0.0))
  prmfile += ('INS %2iBNKPAR%10.3f%10.3f%10.3f%10.3f%10.3f%5i%5i\n' %
  (bank, self.iL2, pardict["twotheta"], 0, 0, 0.2, 1, 1))
  prmfile += ('INS %2iBAKGD 1 4 Y 0 Y\n' % (bank))
  prmfile += ('INS %2iI HEAD %s\n' %
  (bank, titleline))

  prmfile += ('INS %2iI ITYP%5i%10.4f%10.4f%10i\n' %
  (bank, 0, self.mndsp[bank-1]*0.001*instC, self.mxtofs[bank-1], randint(10001,99999)))
  prmfile += ('INS %2iINAME powgen \n' %(bank))
  prmfile += ('INS %2iPRCF1 %5i%5i%10.5f\n' % (bank, -3, 21, 0.002))
  prmfile += ('INS %2iPRCF11%15.6f%15.6f%15.6f%15.6f\n' %
  (bank, 0.0, 0.0, 0.0, pardict["Sig0"])) # sigma20
  prmfile += ('INS %2iPRCF12%15.6f%15.6f%15.6f%15.6f\n' %
  (bank, pardict["Sig1"], pardict["Sig2"], pardict["Gam0"], pardict["Gam1"]))
  prmfile += ('INS %2iPRCF13%15.6f%15.6f%15.6f%15.6f\n' %
  (bank, pardict["Gam2"], 0.0, 0.0, 0.0))
  prmfile += ('INS %2iPRCF14%15.6f%15.6f%15.6f%15.6f\n' %
  (bank, 0.0, 0.0, 0.0, 0.0))
  prmfile += ('INS %2iPRCF15%15.6f%15.6f%15.6f%15.6f\n' %
  (bank, 0.0, 0.0, 0.0, 0.0))
  prmfile += ('INS %2iPRCF16%15.6f\n' % (bank, 0.0))
  prmfile += ('INS %2iPAB3 %3i\n' % (bank, 90))
  for k in xrange(90):
  prmfile += ('INS %2iPAB3%2i%10.5f%10.5f%10.5f%10.5f\n' %(bank, k+1,
  self.gdsp[k], self.gdt[k], self.galpha[k], self.gbeta[k]))
  prmfile += ('INS %2iPRCF2 %5i%5i%10.5f\n' % (bank, -4, 27, 0.002))
  prmfile += ('INS %2iPRCF21%15.6f%15.6f%15.6f%15.6f\n' % (bank, 0.0, 0.0, 0.0, pardict["Sig1"]))
  prmfile += ('INS %2iPRCF22%15.6f%15.6f%15.6f%15.6f\n' % (bank, pardict["Sig2"], pardict["Gam2"], 0.0, 0.0))
  prmfile += ('INS %2iPRCF23%15.6f%15.6f%15.6f%15.6f\n' % (bank, 0.0, 0.0, 0.0, 0.0))
  prmfile += ('INS %2iPRCF24%15.6f%15.6f%15.6f%15.6f\n' % (bank, 0.0, 0.0, 0.0, 0.0))
  prmfile += ('INS %2iPRCF25%15.6f%15.6f%15.6f%15.6f\n' % (bank, 0.0, 0.0, 0.0, 0.0))
  prmfile += ('INS %2iPRCF26%15.6f%15.6f%15.6f%15.6f\n' % (bank, 0.0, 0.0, 0.0, 0.0))
  prmfile += ('INS %2iPRCF27%15.6f%15.6f%15.6f \n' % (bank, 0.0, 0.0, 0.0 ))
  prmfile += ('INS %2iPAB4 %3i\n' % (bank, 90))
  for k in xrange(90):
  prmfile += ('INS %2iPAB4%2i%10.5f%10.5f%10.5f%10.5f\n' %(bank, k+1,
  self.gdsp[k], self.gdt[k], self.galpha[k], self.gbeta[k]))
  prmfile += ('INS %2iPRCF3 %5i%5i%10.5f\n' % (bank, -5, 21, 0.002))
  prmfile += ('INS %2iPRCF31%15.6f%15.6f%15.6f%15.6f\n' % (bank, 0.0, 0.0, 0.0, pardict["Sig0"]))
  prmfile += ('INS %2iPRCF32%15.6f%15.6f%15.6f%15.6f\n' % (bank, pardict["Sig1"], pardict["Sig2"],
  pardict["Gam0"], pardict["Gam1"]))
  prmfile += ('INS %2iPRCF33%15.6f%15.6f%15.6f%15.6f\n' % (bank, pardict["Gam2"], 0.0, 0.0, 0.0))
  prmfile += ('INS %2iPRCF34%15.6f%15.6f%15.6f%15.6f\n' % (bank, 0.0, 0.0, 0.0, 0.0))
  prmfile += ('INS %2iPRCF35%15.6f%15.6f%15.6f%15.6f\n' % (bank, 0.0, 0.0, 0.0, 0.0))
  prmfile += ('INS %2iPRCF36%15.6f\n' % (bank, 0.0))
  prmfile += ('INS %2iPAB5 %3i\n' % (bank, 90))
  for k in xrange(90):
  prmfile += ('INS %2iPAB5%2i%10.5f%10.5f%10.5f%10.5f\n' %(bank, k+1, self.gdsp[k], self.gdt[k], self.galpha[k], self.gbeta[k]))
  prmfile += ('')

  # 3. Write to file
  if isfirstbank:
  wprmfile = open(prmfilename, 'w')
  else:
  wprmfile = open(prmfilename, 'a')
  wprmfile.write(prmfile)
  wprmfile.close()
  */

    return;
  }

  /** Caclualte L2 from DIFFC and L1
    * DIFC = 252.816*2sin(theta)sqrt(L1+L2)
    */
  double SaveGSASInstrumentFile::calL2FromDtt1(double difc, double L1, double twotheta)
  {
    double l2 = difc/(252.816*2.0*sin(0.5*twotheta*PI/180.0)) - L1;
    // cout <<  "DIFC = %f,  L1 = %f,  2theta = %f, L2 = %f" % (difc, L1, twotheta, l2)

    return l2;
  }


  /** Calculate TOF difference
   * @param ep :: zero
   * @param eq :: dtt1
   * @param er :: dtt2
   * @param tp :: zerot
   * @param tq :: dtt1t
   * @param er :: dtt2t
  */
  double SaveGSASInstrumentFile::calTOF(double n, double ep, double eq, double er, double tp, double tq, double tr, double dsp)
  {
      // Epithermal
      // double te = zero  + d*dtt1  + 0.5*dtt2*erfc( (1/d-1.05)*10 );
      // Thermal
      // double tt = zerot + d*dtt1t + dtt2t/d;
      // Total TOF
      // double t  = n*te + (1-n) tt

      // FIXME Is this equation for TOF^e correct?
      double te = ep + (eq*dsp) + er*0.5*erfc(((1.0/dsp)-1.05)*10.0);
      double tt = tp + (tq*dsp) + (tr/dsp);
      double t = (n*te) + tt - (n*tt);

      return t;
  }

  //----------------------------------------------------------------------------------------------
  /** Calculate a value related to alph0, alph1, alph0t, alph1t or
    * beta0, beta1, beta0t, beta1t
    */
  double SaveGSASInstrumentFile::aaba(double n, double ea1, double ea2, double ta1, double ta2, double dsp)
  {
    double ea = ea1 + (ea2*dsp);
    double ta = ta1 - (ta2/dsp);
    double am1 = (n*ea) + ta - (n*ta);
    double a = 1.0/am1;

    return a;
  }

  //----------------------------------------------------------------------------------------------
  /** Get parameter value from a map
    */
  double SaveGSASInstrumentFile::getParameterValue(std::map<std::string, double> profilemap, string parname)
  {
    // FIXME - Implement ASAP
    throw runtime_error("Implement ASAP.");
  }

} // namespace Algorithms
} // namespace Mantid



// from MantidFramework import *
// from mantidsimple import *
// import mantid.simpleapi as api
// import os
// from random import *
// import numpy as np




/** Complementary error function
*/
double erfc(double xx)
{
  double x = fabs(xx);
  double t = 1.0 / (1.0 + (0.5 * x));
  double ty = (0.27886807 + t * (-1.13520398 + t * (1.48851587 + t * (-0.82215223 + t * 0.17087277))));
  double tx = (1.00002368 + t * (0.37409196 + t * (0.09678418 + t * (-0.18628806 + t * ty))));
  double y = t * exp(-x * x - 1.26551223 + t * tx);
  if (xx < 0)
    y = 2.0 - y;

  return y;
}




/** Initialize constant library
  */
void initConstantLib()
{

}


