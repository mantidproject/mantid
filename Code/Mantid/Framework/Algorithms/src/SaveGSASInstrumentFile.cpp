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

namespace Mantid
{
namespace Algorithms
{

  DECLARE_ALGORITHM(SaveGSASInstrumentFile)

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
    setWikiSummary("");
    setOptionalMessage("");
  }

  //----------------------------------------------------------------------------------------------
  void SaveGSASInstrumentFile::init()
  {
    /**
      """ Set Property
      """
      instruments = ["PG3", "NOM", "VULCAN", "SNAP"]
      frequencies = ["10", "30", "60"]

      self.declareProperty("Instrument", "PG3", Validator=ListValidator(instruments),
              Description="SNS Instrument Name")
      self.declareFileProperty("InputFile", "", FileAction.Load, [".irf", ".pcr"],
              Description="Resolution (Fullprof, .irf) file")
      self.declareProperty("IDLine", "", Description="ID Line in output GSAS instrument file")
      self.declareProperty("Sample", "", Description="Sample information written to header (title)")
      self.declareListProperty("Banks", [1], Validator=ArrayBoundedValidator(Lower=0),
              Description="Banks to be written into output file")
      self.declareProperty("Frequency", "60", Validator=ListValidator(frequencies),
              Description="Frequency of the instrument file corresponds to")
      self.declareProperty("L1", -1.0)
      self.declareProperty("L2", -1.0,
              Description="Distance from sample to detector.  If 2Theta is given, this won't work. ")
      self.declareProperty("2Theta", 1001.0,
              Description="Angle of the detector bank.  It is to calculate L2 with given Dtt1")
      self.declareFileProperty("OutputFile", "", FileAction.Save, [".iparm", ".prm"],
              Description="Output .iparm or .prm file")
    */
    return;
  }

  //----------------------------------------------------------------------------------------------
  void SaveGSASInstrumentFile::exec()
  {
    // Get properties
    m_instrument = getProperty("Instrument");
    string id_line = getProperty("IDLine"); // Standard Run LB4844 Vanadium: 4866 J.P. Hodges 2011-09-01
    string sample = getProperty("Sample"); // titleline = "LaB6 NIST RT 4844[V=4866] 60Hz CW=.533"
    string inputfilename = getProperty("InputFile");
    string outputfilename = getProperty("OutputFile");
    int banks = getProperty("Banks");
    m_L1 = getProperty("L1");
    m_2theta = getProperty("2Theta");
    m_L2 = getProperty("L2");
    m_frequency = getProperty("Frequency");

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
      if (templ2 == EMPTY_DBL())
      {
        string errmsg("User must specify either 2theta or L2.  Neither of them is given.");
        g_log.error(errmsg);
        throw runtime_error(errmsg);
      }

      m_L2 = templ2;
    }

    // Execute
    initConstants(m_frequency);

    if (useirf)
    {
      parseFullprofResolutionFile(irffilename);
    }
    else
    {
      throw runtime_error("Parsing .pcr file has not been implemented yet.");
    }

    makeParameterConsistent();

    convertToGSAS(banks, outputfilename);

    return;
  }

  //----------------------------------------------------------------------------------------------
  /** Set up some constant by default
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
  /** Constructor of chopper configuration
    */
  ChopperConfiguration::ChopperConfiguration(double freq, string bankidstr, string cwlstr, string mndspstr, string mxdspstr, string maxtofstr, string splitdstr, string vrunstr)
  {
    m_frequency = freq;
    m_bankIDs = parseStringInt(vankidstr);
    m_CWL = parseString(cwlstr);
    m_mindsps = parseStringDbl(mndspstr);
    m_maxdsps = parseStringDbl(mxdspstr);
    m_maxtofs = parseStringDbl(maxtofstr);
    m_splitds = parseStringDbl(splitdstr);
    m_vruns = parseStringInt(vrunstr);

    // Check size
    if (m_bankIDs.size() != m_CWL.size() || m_CWL.size() != m_mindsps.size() || m_CWL.size() != m_maxdsps.size())
      throw "Input string has different length.  ";

    // Build

  }

  //----------------------------------------------------------------------------------------------
  void SaveGSASInstrumentFile::setupPG3Constants(int intfrequency)
  {
    // Create string
    switch (intfrequency)
    {
      string bankidstr, cwlstr, mndspstr, mxdspstr;
      case 60:
        bankidstr = "1,2,3,4,5,6,7";
        cwlstr = "0.533, 1.066, 1.333, 1.599, 2.665, 3.731, 4.797";
        mndspstr = "0.10, 0.276, 0.414, 0.552, 1.104, 1.656, 2.208";
        mxdspstr = "2.06, 3.090, 3.605, 4.120, 6.180, 8.240, 10.30";
        mxtofstr = "46.76, 70.14, 81.83, 93.52, 140.3, 187.0, 233.8";

        break;
      case 30:
        bankidstr = "1,2,3";
        cwlstr = "1.066, 3.198, 5.33";
        mndspstr = "0.10, 1.104, 2.208";
        mxdspstr = "4.12, 8.24, 12.36";
        mxtofstr = "93.5, 187.0, 280.5";

        break;

      case 10:
        // Frequency = 10
        bankidstr = "1";
        cwlstr = "3.198";
        mndspstr = "0.10";
        mxdspstr = "12.36";
        mxtofstr = "280.5";

        break;

      default:
        throw runtime_error("Not supported");
        break;
    }

    // Create configuration
    ChopperConfiguration conf(static_cast<float>(intfrequecy), bankidstr, cwlstr, mndspstr, mxdspstr, maxtofstr);

    return conf;
  }

  //----------------------------------------------------------------------------------------------
  /** Set up the converting constants for NOMAD
    * @param intfrequency :: frequency in integer
    */
  void SaveGSASInstrumentFile::setupNOMConstants(int intfrequency)
  {
    // Set up string
    switch (intfrequency)
    {
      string bankidstr, cwlstr, mndspstr, mxdspstr;
      case 60:
        bankidstr = "4,5";
        cwlstr = "1.500, 1.5000";
        mndspstr = "0.052, 0.0450";
        mxdspstr = "2.630, 2.6000";
        mxtofstr = "93.52, 156.00";
        break;

      default:
        throw runtime_error("Not supported");
        break;
    }

    // Create configuration
    ChopperConfiguration conf(static_cast<float>(intfrequecy), bankidstr, cwlstr, mndspstr, mxdspstr, maxtofstr);

    return conf;
  }


  //----------------------------------------------------------------------------------------------
  /** Parse string to double vector
    */
  vector<double> ChopperConfiguration::parseStringDbl(string instring)
  {
    vector<string> strs;
    boost::split(strs,line,boost::is_any_of(", "));
    cout << "* size of the vector: " << strs.size() << endl;

    vector<double> vecdouble;
    for (size_t i = 0; i < strs.size(); i++)
    {
      double item = atof(strs[i].c_str());
      vecdouble.push_back(item);
    }

    return vecdouble;
  }

  //----------------------------------------------------------------------------------------------
  /** Convert to GSAS instrument file
  - banks : list of banks (sorted) to .iparm or prm file
  - gsasinstrfilename: string
    */
  void convertToGSAS(vector<int> banks, string gsasinstrfilename)
  {
    if (!m_config)
      throw runtime_error("Not set up yet!");


      //  Convert and write
    banks = sort(banks.begin(), banks.end());
    bool isfirstbank = True;
    for (size_t ib = 0; ib < banks.size(); ++ib)
    {
      bankid = banks[ib];
      if (m_config.hasBank(bankid))
      {
        buildGSASTabulatedProfile(bankid);
        writePRM(bank, banks.size(), gsasinstrfilename, isfirstbank);
        isfirstbank = false;
      }
      else
      {
        g_log.warning() << "Bank " << bankid << " does not exist in source resolution file";
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
  void SaveGSASInstrumentFile::buildGSASTabulatedProfile(int bank)
  {
    // Init data structure
    vector<double> gdsp(90, 0.);

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


  /**  Write out .prm/.iparm file

  Arguments:
- bank : integer, bank ID
- numbanks: integer, total number of banks written in file
- prmfilename: output file name
- isfirstbank: bool
  */
  void SaveGSASInstrumentFile::writePRM(int bank, size_t numbanks, std::string prmfilename, bool isfirstbank)
  {

    /** To translate
      # 1. Set essential values
      pardict = self.mdict[bank]

      instC = pardict["Dtt1"] - (4*(pardict["Alph0"]+pardict["Alph1"]))
      titleline = "%s %dHz CW=%f" % (self.sample, self.frequency, self.CWL[bank-1])

      # 2. Write section of prm file to string
      prmfile = ""
      if isfirstbank is True:
          # First bank in the file, Write header
          prmfile += (' 12345678901234567890123456789012345678901234567890123456789012345678\n')
          prmfile += ('ID %s\n' %(self.id_line))
          prmfile += ('INS BANK %5i\n' % (numbanks))
          prmfile += ('INS FPATH1 %f \n' % (self.iL1))
          prmfile += ('INS HTYPE PNTR \n')
       # ENDIF

      if self.iL2 < 0:
          self.iL2 = calL2FromDtt1(difc=self.mdict[bank]["Dtt1"], L1=self.iL1, twotheta=self.i2theta)

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


} // namespace Algorithms
} // namespace Mantid



// from MantidFramework import *
// from mantidsimple import *
// import mantid.simpleapi as api
// import os
// from random import *
// import numpy as np

/** Calculate TOF difference
 * @param ep :: zero
 * @param eq :: dtt1
 * @param er :: dtt2
 * @param tp :: zerot
 * @param tq :: dtt1t
 * @param er :: dtt2t
*/
double tofh(double n, double ep, double eq, double er, double tp, double tq, double tr, double dsp)
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


/** Complementary error function
*/
double erfc(double xx)
{
    double x = fabs(xx);
    double t = 1.0 / (1.0 + (0.5 * x));
    double ty = (0.27886807 + t * (-1.13520398 + t * (1.48851587 + t * (-0.82215223 + t * 0.17087277))));
    double tx = (1.00002368 + t * (0.37409196 + t * (0.09678418 + t * (-0.18628806 + t * ty))));
    double y = t * np.exp(-x * x - 1.26551223 + t * tx);
    if (xx < 0)
        y = 2.0 - y;

    return y;
}

/** Calculate a value related to alph0, alph1, alph0t, alph1t or
    beta0, beta1, beta0t, beta1t
*/
double aaba(double n, double ea1, double ea2, double ta1, double ta2, double dsp)
{
    /*
    double g  = g0  + g1*d;
    double gt = gt1 + g1t*d;
    a = 1/(n*g + (1-n)gt)
    */
    double ea = ea1 + (ea2*dsp);
    double ta = ta1 - (ta2/dsp);
    double am1 = (n*ea) + ta - (n*ta);
    double a = 1.0/am1;

    return a;
}

/** Caclualte L2 from DIFFC and L1

    DIFC = 252.816*2sin(theta)sqrt(L1+L2)
*/
double calL2FromDtt1(double difc, double L1, double twotheta)
{
    double l2 = difc/(252.816*2.0*sin(0.5*twotheta*math.pi/180.0)) - L1;
    // cout <<  "DIFC = %f,  L1 = %f,  2theta = %f, L2 = %f" % (difc, L1, twotheta, l2)

    return l2;
}

/** Initialize constant library
  */
void initConstantLib()
{

}


