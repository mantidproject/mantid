"""*WIKI* 

This algorithm is to import Fullprof .irf file (peak parameters) and .hkl file (reflections) and 
record the information to TableWorkspaces, which serve as the inputs for algorithm LeBailFit. 

==== Format of Instrument parameter TableWorkspace ====
Instrument parameter TableWorkspace contains all the peak profile parameters imported from Fullprof .irf file.  

Presently these are the peak profiles supported
 * Thermal neutron back to back exponential convoluted with pseudo-voigt (profile No. 10 in Fullprof)

Each row in TableWorkspace corresponds to one profile parameter.  

Columns include Name, Value, FitOrTie, Min, Max and StepSize.


==== Format of reflection TableWorkspace ====
Each row of this workspace corresponds to one diffraction peak.  
The information contains the peak's Miller index and (local) peak profile parameters of this peak.  
For instance of a back-to-back exponential convoluted with Gaussian peak, 
the peak profile parameters include Alpha, Beta, Sigma, centre and height. 

== How to use algorithm with other algorithms ==
This algorithm is designed to work with other algorithms to do Le Bail fit.  The introduction can be found in the wiki page of [[LeBailFit]].

*WIKI*"""
from mantid.api import PythonAlgorithm, registerAlgorithm, ITableWorkspaceProperty, WorkspaceFactory, FileProperty, FileAction
from mantid.kernel import Direction, StringListValidator

_OUTPUTLEVEL = "NOOUTPUT"

class CreateLeBailFitInput(PythonAlgorithm):
    """ Create the input TableWorkspaces for LeBail Fitting
    """
    def category(self):
        """
        """
        return "Diffraction;Utility"

    def name(self):
        """
        """
        return "CreateLeBailFitInput"
 
    def PyInit(self):
        """ Declare properties
        """
        instruments=["POWGEN", "NOMAD", "VULCAN"]
        self.setWikiSummary("""Create various input Workspaces required by algorithm LeBailFit.""")
        
        self.declareProperty("Instrument", "POWGEN", StringListValidator(instruments), "Powder diffractometer's name")

        self.declareProperty(FileProperty("ReflectionsFile","", FileAction.Load, ['.hkl']),
                "Name of [http://www.ill.eu/sites/fullprof/ Fullprof] .hkl file that contains the peaks.")

        self.declareProperty(FileProperty("FullprofParameterFile", "", FileAction.Load, ['.irf']),
                "Fullprof's .irf file containing the peak parameters.")

        self.declareProperty("Bank", 1, "Bank ID for output if there are more than one bank in .irf file.")

        self.declareProperty("LatticeConstant", 10.0, "Lattice constant for cubic crystal.")

        self.declareProperty(ITableWorkspaceProperty("InstrumentParameterWorkspace", "", Direction.Output), 
                "Name of Table Workspace Containing Peak Parameters From .irf File.")
 
        self.declareProperty(ITableWorkspaceProperty("BraggPeakParameterWorkspace", "", Direction.Output), 
                "Name of Table Workspace Containing Peaks' Miller Indices From .prf File.")

        return
 
    def PyExec(self):
        """ Main Execution Body
        """
        # 1. Setup output workspaces
        paramWS = WorkspaceFactory.createTable()
        self.setProperty("InstrumentParameterWorkspace", paramWS)

        hklWS = WorkspaceFactory.createTable()
        self.setProperty("BraggPeakParameterWorkspace", hklWS)

        # 2. Get Other Properties
        instrument = self.getProperty("Instrument")
        reflectionfilename = self.getPropertyValue("ReflectionsFile")
        irffilename = self.getPropertyValue("FullprofParameterFile")

        # 3. Import reflections list
        hkldict = self.importFullProfHKLFile(reflectionfilename)

        hkllist = sorted(hkldict.keys())
        if _OUTPUTLEVEL == "INFORMATION":
            for hkl in hkllist:
                print "Import Peak (%d, %d, %d): FWHM = %f" % (hkl[0], hkl[1], hkl[2], hkldict[hkl]["FWHM"])

        # 4. Import parameter file (.irf)
        peakparamsdict = self.parseFullprofPeakProfileFile(irffilename)

        # 5. Set up the table workspaces 
        self.createPeakParameterWorkspace(peakparamsdict, paramWS)
        self.createReflectionWorkspace(hkldict, hklWS)

        return


    def importFullProfHKLFile(self, hklfilename):
        """ Import Fullprof's .hkl file
        """
        import math

        # 1. Import file
        try:
            hklfile = open(hklfilename, "r")
            lines = hklfile.readlines()
            hklfile.close()
        except IOError:
            print "Error to open/read Fullprof .hkl file %s" % (hklfilename)
            raise IOError()

        # 2. Parse
        hkldict = {}
        for line in lines:
            # a) Clean & split
            line = line.strip()
            if len(line) == 0:
                continue
            terms = line.split()

            # b) parse
            if not terms[0].isdigit():
                # Comment line
                continue

            h = int(terms[0])
            k = int(terms[1])
            l = int(terms[2])
            if len(terms) >= 9:
                dsp = float(terms[3])
                tof = float(terms[4])
                alpha = float(terms[5])
                beta = float(terms[6])
                sigma2 = float(terms[7])
                gamma2 = float(terms[8])
            else:
                dsp = 0.0
                tof = 0.0
                alpha = 0.0
                beta = 0.0
                sigma2 = 0.0
                gamma2 = 0.0
                fwhm = 1.0

            if len(terms) >= 13:
                fwhm = float(terms[12])
            elif len(terms) >= 9:
                fwhm = math.sqrt(sigma2)*2.0

            dkey = (h, k, l)

            if hkldict.has_key(dkey):
                if _OUTPUTLEVEL == "INFORMATION": 
                    print "Warning! Duplicate HKL %d, %d, %d" (h, k, l)
                continue

            if fwhm < 1.0E-5:
                # Peak width is too small/annihilated peak
                if _OUTPUTLEVEL == "INFORMATION": 
                    print "Peak (%d, %d, %d) has an unreasonable small FWHM.  Peak does not exist. " % (h, k, l)
                continue

            hkldict[dkey] = {}
            hkldict[dkey]["dsp"] = dsp
            hkldict[dkey]["tof"] = tof
            hkldict[dkey]["alpha"] = alpha
            hkldict[dkey]["beta"] = beta
            hkldict[dkey]["sigma2"] = sigma2
            hkldict[dkey]["gamma2"] = gamma2
            hkldict[dkey]["FWHM"] = fwhm
        # ENDFOR: line

        print "[CreateLeBailFit] Import Fullprof reflection file %s successfully. " % (hklfilename)

        return hkldict

    def parseFullprofPeakProfileFile(self, irffilename):
        """ Parse Fullprof resolution .irf file
        (It is the same function as what in ConvertInstrumentFile(),
         except the key word in the output dictionary.)

        Note:
        1. Sig-0, Sig-1 and Sig-2 in .irf file are actually the square of sig0, sig1 and sig2
           defined in the manual
    
        Input:
         - irffilename:  Resolution file (.irf)  Can be single bank or multiple bank
    
        Output:
         - dictionary: [bank][parameter name][value]
        """
        import math

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
        for il in xrange(0, len(lines)):
            line = lines[il]
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
            elif line[0] != '!':
                # Not Comment line
                if line.startswith("NPROF"):
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
                    mdict[bank]["Dtt1"] = float(terms[1])
                    if len(terms) == 3:
                        mdict[bank]["Dtt2"] = float(terms[2])
                        mdict[bank]["Zero"] = float(terms[3])
                    else:
                        mdict[bank]["Dtt2"] = 0.0
                        mdict[bank]["Zero"] = 0.0
    
                elif line.startswith("ZD2TOF"):
                    #  Zero   Dtt1  
                    terms = line.split()
                    mdict[bank]["Zero"] = float(terms[1])
                    mdict[bank]["Dtt1"] = float(terms[2])
                    mdict[bank]["Dtt2"] = 0.0
    
                elif line.startswith("D2TOT"):
                    # Dtt1t       Dtt2t    x-cross    Width   Zerot
                    terms = line.split()
                    mdict[bank]["Dtt1t"] = float(terms[1])
                    mdict[bank]["Dtt2t"] = float(terms[2])
                    mdict[bank]["Tcross"] = float(terms[3])
                    mdict[bank]["Width"] = float(terms[4])
                    mdict[bank]["Zerot"] = float(terms[5])
    
                elif line.startswith("ZD2TOT"):
                    # Zerot    Dtt1t       Dtt2t    x-cross    Width
                    terms = line.split()
                    mdict[bank]["Zerot"] = float(terms[1])
                    mdict[bank]["Dtt1t"] = float(terms[2])
                    mdict[bank]["Dtt2t"] = float(terms[3])
                    mdict[bank]["Tcross"] = float(terms[4])
                    mdict[bank]["Width"] = float(terms[5])
    
                elif line.startswith("TWOTH"):
                    # TOF-TWOTH of the bank
                    terms = line.split()
                    mdict[bank]["twotheta"] = float(terms[1])
    
                elif line.startswith("SIGMA"):
                    # Gam-2     Gam-1     Gam-0 
                    terms = line.split()
                    mdict[bank]["Sig2"] = math.sqrt(abs(float(terms[1])))
                    mdict[bank]["Sig1"] = math.sqrt(abs(float(terms[2])))
                    mdict[bank]["Sig0"] = math.sqrt(abs(float(terms[3])))
    
                elif line.startswith("GAMMA"):
                    # Gam-2     Gam-1     Gam-0 
                    terms = line.split()
                    mdict[bank]["Gam2"] = float(terms[1])
                    mdict[bank]["Gam1"] = float(terms[2])
                    mdict[bank]["Gam0"] = float(terms[3])
    
                elif line.startswith("ALFBE"):
                    # alph0       beta0       alph1       beta1 
                    terms = line.split()
                    mdict[bank]["Alph0"] = float(terms[1])
                    mdict[bank]["Beta0"] = float(terms[2])
                    mdict[bank]["Alph1"] = float(terms[3])
                    mdict[bank]["Beta1"] = float(terms[4])
    
                elif line.startswith("ALFBT"):
                    # alph0t       beta0t       alph1t       beta1t 
                    terms = line.split()
                    mdict[bank]["Alph0t"] = float(terms[1])
                    mdict[bank]["Beta0t"] = float(terms[2])
                    mdict[bank]["Alph1t"] = float(terms[3])
                    mdict[bank]["Beta1t"] = float(terms[4])

                else:
                    pass
            
            else:
                # COMMENT Line
                pass
            # ENDIF: Line type
        # ENDFOR

        self.mdict = mdict
        
        print "[CreateLeBailFit] Import Fullprof resolution file %s for bank %d successfully. " % (irffilename, bank)
        #            print "Import .irf File.  Bank = %d" % (bank)

   
        return mdict


    def createPeakParameterWorkspace(self, paramdict, tablews):
        """ Create TableWorkspace containing peak parameters
        """
        # 1. Create an empty workspace and set the column
        tablews.addColumn("str", "Name")
        tablews.addColumn("double", "Value")
        tablews.addColumn("str", "FitOrTie")
        tablews.addColumn("double", "Min")
        tablews.addColumn("double", "Max")
        tablews.addColumn("double", "StepSize")

        # 2. Add value
        bankproperty = self.getProperty("Bank")
        bank = bankproperty.value
        if paramdict.has_key(bank) is False: 
            print "Bank Type: ", type(bank)
            raise NotImplementedError("Bank %s does not exist in input .irf file." % (bank))

        for parname in sorted(paramdict[bank].keys()):
            if _OUTPUTLEVEL == "INFORMATION": 
                print "Insert parameter %s , value = %f " % (parname, paramdict[bank][parname])
            tablews.addRow([parname, paramdict[bank][parname], "f", -1.0E100, 1.0E100, 1.0])
        # ENDFOR

        # 3. Add lattice constant
        latticeconstant = self.getProperty("LatticeConstant").value
        tablews.addRow(["LatticeConstant", latticeconstant, "t", 0.1, 1.0E5, 0.1])

        return tablews

    def createReflectionWorkspace(self, hkldict, tablews):
        """ Create TableWorkspace containing reflections and etc. 
        """
        # 1. Set up columns
        tablews.addColumn("int", "H");
        tablews.addColumn("int", "K");
        tablews.addColumn("int", "L"); 
        tablews.addColumn("double", "PeakHeight"); 
        tablews.addColumn("str", "Include/Exclude")

        # 2. Add rows
        for hkl in sorted(hkldict.keys()):
            tablews.addRow([hkl[0], hkl[1], hkl[2], 1.0, "i"])
        # ENDFOR

        return tablews


# Register algorithm with Mantid
registerAlgorithm(CreateLeBailFitInput)

