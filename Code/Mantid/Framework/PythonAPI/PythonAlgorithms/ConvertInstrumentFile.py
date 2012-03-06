"""*WIKI* 


Convert .irf/.pcr/.pm1 to .iparm/.prm 


*WIKI*"""

from MantidFramework import *
from mantidsimple import *
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

            hz60_f = chopperhertz
            hz30_f = chopperhertz
            hz10_f = chopperhertz

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
        # 1. Import data
        try:
            irffile = open(irffilename, "r")
            rawlines = irffile.readlines()
            irffile.close()
        except IOError:
            print "Fullprof resolution file %s cannot be read" % (irffilename)
            raise NotImplementedError("Input resolution file error!")
    
        lines = []
        for line in rawlines:
            cline = line.strip()
            if len(cline) > 0:
                lines.append(cline)
        # ENDFOR
    
        # 2. Parse
        mdict = {}
        mdict["title"] = lines[0]
        bank = -1
        for il in xrange(1, len(lines)):
            line = lines[il]
            if line[0] == '!':
                # Comment line
                if line.count("Bank") > 0:
                    # Line with bank
                    terms = line.split("Bank")[1].split()
                    bank = int(terms[0])
                    mdict[bank] = {}
                    if len(terms) >= 4 and terms[1] == "CWL":
                        # center wave length
                        cwl = float(terms[3].split("A")[0])
                        mdict[bank]["CWL"] = cwl
                    # ENDIF: CWL
                # ENDIF: Containing Bank
    
            elif line.startswith("NPROF"):
                # Profile Type
                profiletype = int(line.split("NPROF")[1])
                mdict[bank]["Profile"] = profiletype
    
            elif line.startswith("TOFRG"):
                # Tof-min(us)    step      Tof-max(us)
                terms = line.split()
                mdict[bank]["tof-min"] = float(terms[1])
                mdict[bank]["tof-max"] = float(terms[3])
                mdict[bank]["step"]    = float(terms[2])
    
            elif line.startswith("D2TOF"):
                # Dtt1      Dtt2         Zero 
                terms = line.split()
                mdict[bank]["dtt1"] = float(terms[1])
                if len(terms) == 3:
                    mdict[bank]["dtt2"] = float(terms[2])
                    mdict[bank]["zero"] = float(terms[3])
                else:
                    mdict[bank]["dtt2"] = 0.0
                    mdict[bank]["zero"] = 0.0
    
            elif line.startswith("ZD2TOF"):
                #  Zero   Dtt1  
                terms = line.split()
                mdict[bank]["zero"] = float(terms[1])
                mdict[bank]["dtt1"] = float(terms[2])
                mdict[bank]["dtt2"] = 0.0
    
            elif line.startswith("D2TOT"):
                # Dtt1t       Dtt2t    x-cross    Width   Zerot
                terms = line.split()
                mdict[bank]["dtt1t"] = float(terms[1])
                mdict[bank]["dtt2t"] = float(terms[2])
                mdict[bank]["x-cross"] = float(terms[3])
                mdict[bank]["width"] = float(terms[4])
                mdict[bank]["zerot"] = float(terms[5])
    
            elif line.startswith("ZD2TOT"):
                # Zerot    Dtt1t       Dtt2t    x-cross    Width
                terms = line.split()
                mdict[bank]["zerot"] = float(terms[1])
                mdict[bank]["dtt1t"] = float(terms[2])
                mdict[bank]["dtt2t"] = float(terms[3])
                mdict[bank]["x-cross"] = float(terms[4])
                mdict[bank]["width"] = float(terms[5])
    
            elif line.startswith("TWOTH"):
                # TOF-TWOTH of the bank
                terms = line.split()
                mdict[bank]["twotheta"] = float(terms[1])
    
            elif line.startswith("SIGMA"):
                # Gam-2     Gam-1     Gam-0 
                terms = line.split()
                mdict[bank]["sig-2"] = float(terms[1])
                mdict[bank]["sig-1"] = float(terms[2])
                mdict[bank]["sig-0"] = float(terms[3])
    
            elif line.startswith("GAMMA"):
                # Gam-2     Gam-1     Gam-0 
                terms = line.split()
                mdict[bank]["gam-2"] = float(terms[1])
                mdict[bank]["gam-1"] = float(terms[2])
                mdict[bank]["gam-0"] = float(terms[3])
    
            elif line.startswith("ALFBE"):
                # alph0       beta0       alph1       beta1 
                terms = line.split()
                mdict[bank]["alph0"] = float(terms[1])
                mdict[bank]["beta0"] = float(terms[2])
                mdict[bank]["alph1"] = float(terms[3])
                mdict[bank]["beta1"] = float(terms[4])
    
            elif line.startswith("ALFBT"):
                # alph0t       beta0t       alph1t       beta1t 
                terms = line.split()
                mdict[bank]["alph0t"] = float(terms[1])
                mdict[bank]["beta0t"] = float(terms[2])
                mdict[bank]["alph1t"] = float(terms[3])
                mdict[bank]["beta1t"] = float(terms[4])
    
    
            # ENDIF: Line type
        # ENDFOR

        self.mdict = mdict
    
        return mdict


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
            mX = pardict["x-cross"]
            mXb = pardict["width"]
            instC = pardict["dtt1"] - (4*(pardict["alph0"]+pardict["alph1"])) 
        except KeyError:
            print "Cannot Find Key twotheta/x-cross/width/dtt1/alph0/alph1!"
            print "Keys are: "
            print pardict.keys()
       
        if 1: 
            # latest version from Jason
            ddstep = ((1.05*self.mxdsp[bank-1])-(0.9*self.mndsp[bank-1]))/90  
        else: 
            # used in the older prm file
            ddstep = ((1.00*self.mxdsp[bank-1])-(0.9*self.mndsp[bank-1]))/90

        # 2. Calcualte alph, beta table
        for k in xrange(90):
            gdsp[k] = (0.9*self.mndsp[bank-1])+(k*ddstep)
            rd = 1.0/gdsp[k]
            dmX = mX-rd
            gpkX[k] = 0.5*erfc(mXb*dmX)  # this is n in the formula
            gtof[k] = tofh(gpkX[k], pardict["zero"], pardict["dtt1"] ,pardict["dtt2"],
                    pardict["zerot"], pardict["dtt1t"], -pardict["dtt2t"], gdsp[k])
            gdt[k] = gtof[k] - (instC*gdsp[k])
            galpha[k] = aaba(gpkX[k], pardict["alph0"], pardict["alph1"], 
                    pardict["alph0t"], pardict["alph1t"], gdsp[k])
            gbeta[k] = aaba(gpkX[k], pardict["beta0"], pardict["beta1"], 
                    pardict["beta0t"], pardict["beta1t"], gdsp[k])
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

        instC = pardict["dtt1"] - (4*(pardict["alph0"]+pardict["alph1"])) 
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
            self.iL2 = calL2FromDtt1(difc=self.mdict[bank]["dtt1"], L1=self.iL1, twotheta=self.i2theta)

        print "Debug: L2 = %f,  2Theta (irf) = %f,  2Theta (input) = %f" % (self.iL2, pardict["twotheta"], self.i2theta)

        prmfile += ('INS %2i ICONS%10.3f%10.3f%10.3f          %10.3f%5i%10.3f\n' % 
                (bank, instC*1.00009, 0.0, pardict["zero"],0.0,0,0.0))
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
                (bank, 0.0, 0.0, 0.0, pardict["sig-0"]))    # sigma20
        prmfile += ('INS %2iPRCF12%15.6f%15.6f%15.6f%15.6f\n' % 
                (bank, pardict["sig-1"], pardict["sig-2"], pardict["gam-0"], pardict["gam-1"]))
        prmfile += ('INS %2iPRCF13%15.6f%15.6f%15.6f%15.6f\n' % 
                (bank, pardict["gam-2"], 0.0, 0.0, 0.0))
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
        prmfile += ('INS %2iPRCF21%15.6f%15.6f%15.6f%15.6f\n' % (bank, 0.0, 0.0, 0.0, pardict["sig-1"]))
        prmfile += ('INS %2iPRCF22%15.6f%15.6f%15.6f%15.6f\n' % (bank, pardict["sig-2"], pardict["gam-2"], 0.0, 0.0))
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
        prmfile += ('INS %2iPRCF31%15.6f%15.6f%15.6f%15.6f\n' % (bank, 0.0, 0.0, 0.0, pardict["sig-0"]))
        prmfile += ('INS %2iPRCF32%15.6f%15.6f%15.6f%15.6f\n' % (bank, pardict["sig-1"], pardict["sig-2"], 
            pardict["gam-0"], pardict["gam-1"]))
        prmfile += ('INS %2iPRCF33%15.6f%15.6f%15.6f%15.6f\n' % (bank, pardict["gam-2"], 0.0, 0.0, 0.0))
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
        self.declareProperty("L1", 60.0)
        self.declareProperty("L2", 3.0, 
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
        if self.i2theta >= 1000:
            self.iL2 = self.getProperty("L2")
        else:
            self.iL2 = -1;

        self.frequency = int(self.getProperty("Frequency"))
        outputfilename = self.getProperty("OutputFile")

        # 2. Process input
        useirf = False
        if inputfilename.endswith(".irf"):
            useirf = True
            irffilename = inputfilename
        else:
            pcrfilename = inputfilename

        # 3. Run
        self.initConstants(self.frequency)
        if useirf is True: 
            self.parseFullprofResolutionFile(irffilename)
        self.convertToGSAS(banks, outputfilename)

        return


mtd.registerPyAlgorithm(ConvertInstrumentFile())
