"""*WIKI* 

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


*WIKI*"""

from MantidFramework import *
from mantidsimple import *
import mantid.simpleapi as api
import os
from random import *
import numpy as np

def tofh(n, ep, eq, er, tp, tq, tr, dsp):
    """ Calculate TOF difference

    te = zero  + d*dtt1  + 0.5*dtt2*erfc( (1/d-1.05)*10 )
    tt = zerot + d*dtt1t + dtt2t/d
    t  = n*te + (1-n) tt
    """
    te = ep + (eq*dsp) + er*0.5*erfc(((1.0/dsp)-1.05)*10.0)
    tt = tp + (tq*dsp) + (tr/dsp)
    t = (n*te) + tt - (n*tt)

    return t


def erfc(xx):
    """ Complementary error function
    """
    x = abs(xx)
    t = 1.0 / (1.0 + (0.5 * x))
    ty = (0.27886807 + t * (-1.13520398 + t * (1.48851587 + t * (-0.82215223 + t * 0.17087277))))
    tx = (1.00002368 + t * (0.37409196 + t * (0.09678418 + t * (-0.18628806 + t * ty))))
    y = t * np.exp(-x * x - 1.26551223 + t * tx)
    if (xx < 0):
        y = 2.0 - y

    return y

def aaba(n,ea1, ea2, ta1, ta2, dsp):
    """ Calculate a value related to alph0, alph1, alph0t, alph1t or
    beta0, beta1, beta0t, beta1t

    g  = g0  + g1*d
    gt = gt1 + g1t*d
    a = 1/(n*g + (1-n)gt)
    """
    ea = ea1 + (ea2*dsp)
    ta = ta1 - (ta2/dsp)
    am1 = (n*ea) + ta - (n*ta)
    a = 1.0/am1

    return a

def calL2FromDtt1(difc, L1, twotheta):
    """ Caclualte L2 from DIFFC and L1

    DIFC = 252.816*2sin(theta)sqrt(L1+L2)
    """
    import math
    # print "DIFC = %f,  L1 = %f,  2theta = %f" % (difc, L1, twotheta)
    l2 = difc/(252.816*2.0*math.sin(0.5*twotheta*math.pi/180.0)) - L1
    # print "L2 = %f" % (l2)

    return l2
    
#
#    IRF            =   Dtt1,Dtt2,ZERO ,     
#    Python_code    =   epi_q, 0.0, epi_p,     
#
#                       Dtt1t,Dtt2t,                   
#                       therm_q, -therm_r,     
#
#                       x-cross,Width,Zerot,   
#                       mX, mXb, therm_p,       
#
#                       Sig-2,Sig-1,Sig-0,                        
#                       sigma22, sigma21, sigma20,    
#
#                       Gam-2,Gam-1,Gam-0,     
#                       gam2, gam1, gam0,         
#
#                       alph0,beta0,alph1,beta1,                 
#                       epi_a1, epi_b1, epi_a2, epi_b2,      
#
#                       alph0t,beta0t,alph1t,beta1t
#                       therm_a1, therm_b1, therm_a2, therm_b2

class ConvertInstrumentFile(PythonAlgorithm):
    """ Class to convert between instrument files
    """
    def __init__(self):
        """ Initialization
        """
        PythonAlgorithm.__init__(self)

        self.mdict = {}

        return

    def initConstants(self, chopperhertz):
        """ Initialize constants

        Arguments:
         - hertz:  integer, chopper's frequency
        """

        if self.instrument == "PG3":

            hz60_f = chopperhertz==60
            hz30_f = chopperhertz==30
            hz10_f = chopperhertz==10

            if (hz60_f):
                # 60 Hz
                self.rep='60'
                self.CWL    = [0.533, 1.066, 1.333, 1.599, 2.665, 3.731, 4.797]
                self.mndsp  = [0.10,  0.276, 0.414, 0.552, 1.104, 1.656, 2.208]   #for gsas parameter file extrapolation
                self.mxdsp  = [2.06,  3.090, 3.605, 4.120, 6.180, 8.240, 10.30]
                self.mxtofs = [46.76, 70.14, 81.83, 93.52, 140.3, 187.0, 233.8]
                self.splitd = [0.53, 0.87, 0.87, 0.87, 0.87, 0.87]  #!@#$ don't need this
                self.vrun =   [4866,4867,4868,4869,4870,4871,4872]
            
            elif (hz30_f):
                # 30 Hz
                self.rep='30'  #none for 11-A cycle
                self.CWL    = [1.066, 3.198, 5.33]
                self.mndsp  = [0.10,  1.104, 2.208]
                self.mxdsp  = [4.12,   8.24, 12.36]
                self.mxtofs = [93.5,  187.0, 280.5]
                self.splitd = [0.85, 0.87, 0.87]
                self.vrun = [4873,4874,4891]
            
            elif (hz10_f):
                # 10 Hz
                self.rep='10'  #none for 11-A cycle
                self.CWL    = [3.198]
                self.mndsp  = [0.10]
                self.mxdsp  = [12.36]
                self.mxtofs = [280.5]
                self.splitd = [0.87]
                self.vrun = [4920]
            # ENDIFELSE

        elif self.instrument == "NOM":
            hz60_f = chopperhertz==60
            if (hz60_f):
                # 60 Hz
                self.rep='60'
                self.CWL    = [0.00,  1.11,  2.22,  3.33,  1.5000]
                self.mndsp  = [0,     1,     2,     3,     0.0450]   #for gsas parameter file extrapolation
                self.mxdsp  = [0,     1,     2,     3,     2.6000]
                #self.mndsp  = [0,     1,     2,     3,     0.2335]   #for gsas parameter file extrapolation
                #self.mxdsp  = [0,     1,     2,     3,     1.3270]
                self.mxtofs = [46.76, 70.14, 81.83, 93.52, 156.00]
                self.vrun =   [0,     1,     2,     3,     4   ]
            else:
                raise NotImplementedError("For instrument %s running in %d mode is not supported." 
                        % (self.instrument, chopperhertz))

        else:
            # Cases are not supported
            print "Instrument %s Is Not Setup For CWL, Min/Max d-spacing, and etc"
            raise NotImplementedError("Chopper frequency %d Hz is not supported" % (chopperhertz))

        # ENDIFELSE

        return

    def parseFullprofResolutionFile(self, irffilename):
        """ Parse Fullprof resolution .irf file
    
        Input:
         - irffilename:  Resolution file (.irf)  Can be single bank or multiple bank
    
        Output:
         - dictionary
        """
        mdict = {}

        profilews = api.LoadFullprofResolution(Filename=irffilename, OutputWorkspace="BankInfoTable")

        numbanks = profilews.columnCount() - 1
        self.log().notice("Total %d banks in %s. " % (numbanks, irffilename))

        for i in xrange(numbanks):
            bankid = int(profilews.cell(0, i+1))
            self.log().information("Found bank %d" %(bankid))
            
            colindex = 1 + i
            mdict = self.parseTableWorkspaceToDict(profilews, mdict, bankid, colindex)

            # .irf file may not have term Dtt2
            if mdict[bankid].has_key("Dtt2") is False:
                mdict[bankid]["Dtt2"] = 0.0

            # Convert the unit of tof-min and tof-max 
            mdict[bankid]["tof-max"] = 1.0E-3*mdict[bankid]["tof-max"]

            # Definition of sigma might be different
            mdict[bankid]["Sig0"] = mdict[bankid]["Sig0"]**2
            mdict[bankid]["Sig1"] = mdict[bankid]["Sig1"]**2
            mdict[bankid]["Sig2"] = mdict[bankid]["Sig2"]**2
        # ENDFOR i (each bank)

        self.mdict = mdict
    
        return mdict


    def parseTableWorkspaceToDict(self, tablews, pardict, bankid, colindex):
        """ Parse parameter table workspace to a dictionary
        """
        pardict[bankid] = {}

        numrows = tablews.rowCount()
        
        for irow in xrange(numrows):
            parname = tablews.cell(irow, 0)
            parvalue = tablews.cell(irow, colindex)
            pardict[bankid][parname] = parvalue

        return pardict


    def makeParameterConsistent(self):
        """ Make some parameters consistent between preset values and input values
        """
        for ib in self.mdict.keys():
            try:
                # If it is a dictionary
                if self.mdict[ib].has_key("tof-min"):
                    self.mxtofs[ib-1] = self.mdict[ib]["tof-max"]
            except AttributeError:
                # Not a dictionary
                continue
        # ENDFOR

        return

    def convertToGSAS(self, banks, gsasinstrfilename):
        """ Convert to GSAS instrument file 

        Arguments
         - banks    :   list of banks (sorted) to .iparm or prm file
         - gsasinstrfilename:   string

        Return  
         - None
        """
        # 1. Check
        if self.mdict is None:
            print "No instrument resolution file imported yet!"
            raise NotImplementedError("No instrument resolution file imported!")

        # 2. Convert and write
        banks = sorted(banks)
        isfirstbank = True
        for bank in banks:
            if self.mdict.has_key(bank):
                # Bank exist
                self.buildGSASTabulatedProfile(bank)
                self.writePRM(bank, len(banks), gsasinstrfilename, isfirstbank)
                isfirstbank = False
            else:
                # Bank does not exist
                print "Bank %d does not exist in source resolution file" % (bank)
            # ENDIFELSE
        # ENDFOR

        return


    def buildGSASTabulatedProfile(self, bank):
        """ Build a data structure for GSAS's tabulated peak profile
        from Fullprof's TOF peak profile

        Note:
         - gdsp[k]  :   d_k as the tabulated d-spacing value
         - 

        Argument:
         - pardict

        Return:
         - None
        """
        pardict = self.mdict[bank]

        # 1. Init data structure
        gdsp   = np.zeros(90)       # d_k
        gtof   = np.zeros(90)       # TOF_thermal(d_k) 
        gdt    = np.zeros(90)       # TOF_thermal(d_k) - TOF(d_k)
        galpha = np.zeros(90)       # delta(alpha)
        gbeta  = np.zeros(90)       # delta(beta)
        gpkX   = np.zeros(90)       # n ratio b/w thermal and epithermal neutron
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
            gpkX[k] = 0.5*erfc(mXb*dmX)  # this is n in the formula
            gtof[k] = tofh(gpkX[k], pardict["Zero"], pardict["Dtt1"] ,pardict["Dtt2"],
                    pardict["Zerot"], pardict["Dtt1t"], -pardict["Dtt2t"], gdsp[k])
            gdt[k] = gtof[k] - (instC*gdsp[k])
            galpha[k] = aaba(gpkX[k], pardict["Alph0"], pardict["Alph1"], 
                    pardict["Alph0t"], pardict["Alph1t"], gdsp[k])
            gbeta[k] = aaba(gpkX[k], pardict["Beta0"], pardict["Beta1"], 
                    pardict["Beta0t"], pardict["Beta1t"], gdsp[k])
            #except KeyError err:
            #    print err
            #    raise NotImplementedError("Unable to find some parameter name as key")
        # ENDFOR: k 

        # 3. Set to class variables
        self.gdsp   = gdsp
        self.gdt    = gdt
        self.galpha = galpha
        self.gbeta  = gbeta

        return


    def writePRM(self, bank, numbanks, prmfilename, isfirstbank):
        """ Write out .prm/.iparm file

        Arguments:
         - bank :       integer, bank ID
         - numbanks:    integer, total number of banks written in file
         - prmfilename: output file name
         - isfirstbank: bool
        """
        # 1. Set essential values
        pardict = self.mdict[bank]

        instC = pardict["Dtt1"] - (4*(pardict["Alph0"]+pardict["Alph1"])) 
        titleline = "%s %dHz CW=%f" % (self.sample, self.frequency, self.CWL[bank-1])

        # 2. Write section of prm file to string
        prmfile = ""
        if isfirstbank is True:
            # First bank in the file, Write header
            prmfile += ('            12345678901234567890123456789012345678901234567890123456789012345678\n')
            prmfile += ('ID    %s\n' %(self.id_line))
            prmfile += ('INS   BANK  %5i\n' % (numbanks))
            prmfile += ('INS   FPATH1     %f \n' % (self.iL1))
            prmfile += ('INS   HTYPE   PNTR \n')
         # ENDIF
  
        if self.iL2 < 0:
            self.iL2 = calL2FromDtt1(difc=self.mdict[bank]["Dtt1"], L1=self.iL1, twotheta=self.i2theta)

        # print "Debug: L2 = %f,  2Theta (irf) = %f,  2Theta (input) = %f" % (self.iL2, pardict["twotheta"], self.i2theta)

        prmfile += ('INS %2i ICONS%10.3f%10.3f%10.3f          %10.3f%5i%10.3f\n' % 
                (bank, instC*1.00009, 0.0, pardict["Zero"],0.0,0,0.0))
        prmfile += ('INS %2iBNKPAR%10.3f%10.3f%10.3f%10.3f%10.3f%5i%5i\n' % 
                (bank, self.iL2, pardict["twotheta"], 0, 0, 0.2, 1, 1))
        prmfile += ('INS %2iBAKGD     1    4    Y    0    Y\n'   % (bank))
        prmfile += ('INS %2iI HEAD %s\n' % 
                (bank, titleline))
        prmfile += ('INS %2iI ITYP%5i%10.4f%10.4f%10i\n' %
                (bank, 0, self.mndsp[bank-1]*0.001*instC, self.mxtofs[bank-1], randint(10001,99999)))
        prmfile += ('INS %2iINAME   powgen \n' %(bank))
        prmfile += ('INS %2iPRCF1 %5i%5i%10.5f\n' % (bank, -3, 21, 0.002))
        prmfile += ('INS %2iPRCF11%15.6f%15.6f%15.6f%15.6f\n' % 
                (bank, 0.0, 0.0, 0.0, pardict["Sig0"]))    # sigma20
        prmfile += ('INS %2iPRCF12%15.6f%15.6f%15.6f%15.6f\n' % 
                (bank, pardict["Sig1"], pardict["Sig2"], pardict["Gam0"], pardict["Gam1"]))
        prmfile += ('INS %2iPRCF13%15.6f%15.6f%15.6f%15.6f\n' % 
                (bank, pardict["Gam2"], 0.0, 0.0, 0.0))
        prmfile += ('INS %2iPRCF14%15.6f%15.6f%15.6f%15.6f\n' % 
                (bank, 0.0, 0.0, 0.0, 0.0))
        prmfile += ('INS %2iPRCF15%15.6f%15.6f%15.6f%15.6f\n' % 
                (bank, 0.0, 0.0, 0.0, 0.0))
        prmfile += ('INS %2iPRCF16%15.6f\n' % (bank, 0.0))
        prmfile += ('INS %2iPAB3    %3i\n' % (bank, 90))
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
        prmfile += ('INS %2iPRCF27%15.6f%15.6f%15.6f      \n' % (bank, 0.0, 0.0, 0.0     ))
        prmfile += ('INS %2iPAB4    %3i\n' % (bank, 90))
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
        prmfile += ('INS %2iPAB5    %3i\n' % (bank, 90))
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

        return


    def PyInit(self):
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

        return


    def PyExec(self):
        """ Get Property and Execute
        """
        # 1. Get input
        self.instrument = self.getProperty("Instrument")
        self.id_line = self.getProperty("IDLine") # Standard Run LB4844 Vanadium: 4866   J.P. Hodges  2011-09-01 
        self.sample = self.getProperty("Sample")  # titleline = "LaB6 NIST RT 4844[V=4866] 60Hz CW=.533"
        inputfilename = self.getProperty("InputFile")
        banks = self.getProperty("Banks")
        self.iL1 = self.getProperty("L1")
        self.i2theta = self.getProperty("2Theta")
        templ2 = self.getProperty("L2")
        self.frequency = int(self.getProperty("Frequency"))
        outputfilename = self.getProperty("OutputFile")

        # 2. Process input
        useirf = False
        if inputfilename.endswith(".irf"):
            useirf = True
            irffilename = inputfilename
        else:
            pcrfilename = inputfilename

        # 3. Some "default" values for L1 and L2
        if self.iL1 < 0:
            if self.instrument == "PG3":
                self.iL1 = 60.0
            elif self.instrument == "NOM":
                self.iL1 = 19.5 
            else:
                errmsg = "L1 is not given (unphysical).  There is no default value for instrument %s." % (self.instrument)
                raise NotImplementedError(errmsg)
        # ENDIF

        if self.i2theta >= 1000:
            self.iL2 = templ2            
            if templ2 < 0:
                if self.instrument == "PG3":
                    self.iL2 = 3.0
                elif self.instrument == "NOM":
                    self.iL2 = 2.0
                else:
                    errmsg = "L2 is not given (unphysical).  There is no default value for instrument %s." % (self.instrument)
                    raise NotImplementedError(errmsg)
                # ENDIF
        else:
            self.iL2 = -1;

        # 3. Run
        self.initConstants(self.frequency)
        if useirf is True: 
            self.parseFullprofResolutionFile(irffilename)

        self.makeParameterConsistent()

        self.convertToGSAS(banks, outputfilename)

        return


mtd.registerPyAlgorithm(ConvertInstrumentFile())
