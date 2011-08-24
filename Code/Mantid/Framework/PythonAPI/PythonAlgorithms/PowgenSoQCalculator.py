#!/usr/bin/python
################################################################################
#
# Python algorithm to calculate S(Q) for POWGEN
# The algorithm and data processing procedure are referred to PDFgetN
#
# V10:  POWGEN data only!
#
################################################################################
from MantidFramework import *
from mantidsimple import *
import math

class PowgenSoQCalculator(PythonAlgorithm):
    """ Calculate S(Q) for POWGEN
    The output S(Q) is raw S(Q) before user input the best effective density of 
    sample

    Note: 
    1. The user-defined range is applied in self.reBin().  
       reBin() will set dataE(0)[i] to -1.0 if the dta is out of range
       reBin() will fill the array for values can be interpolated
    2. After the range is extended, code shoudl check again to put the E[i] in the extended region to -1.0
    3. For data not defined, interploate or exterploate in low Q or set to 1 in hgih Q
    """
    def __init__(self):
        """ Initialization
        """
        PythonAlgorithm.__init__(self)

        self.smoothparams = "20,3"
        self.infodict = {}
        self.infodict["vanadium"] = { 
            "Density":              6.0,
            "AttenuationXSection":  2.8, 
            "ScatteringXSection":   5.1, 
            "SampleNumberDensity":  0.0721, 
            }

        return

    def category(self):
        """ Mantid required
        """
        return "Algorithm"

    def name(self):
        """ Mantid required
        """
        return "PowgenSoQCalculator"

    def PyInit(self):
        """ Python init
        (Mantid required)
        """
        # 1. Instrument
        instruments = ["PG3", "NOM", "VULCAN", "SNAP"]
        self.declareProperty("Instrument", "PG3",
                             Validator=ListValidator(instruments))

        # 2. Runs/Workspaces
        self.declareListProperty("SampleRunNumber", [0], 
                Validator=ArrayBoundedValidator(Lower=0))
        self.declareListProperty("CanRunNumber", [0], 
                Validator=ArrayBoundedValidator(Lower=0))
        self.declareListProperty("BackgroundRunNumber", [0], 
                Validator=ArrayBoundedValidator(Lower=0))
        self.declareListProperty("VanadiumRunNumber", [0], 
                Validator=ArrayBoundedValidator(Lower=0))
        self.declareFileProperty("CharacterizationRunsFile", "", FileAction.OptionalLoad, 
                ['.txt'], Description="File with characterization runs denoted")

        # 3. About S(Q)
        self.declareProperty("DeltaQ", 0.02,
                Description = "Delta Q for constant binning for S(Q)")
        self.declareListProperty("DetectorRangeQ", [0.0], 
                Validator=ArrayBoundedValidator(Lower=0))
        #        Description="Detector's range.  Input = Detector ID_1, Qmin, Qmax, Detector ID_2, Qmin, Qmax, ...")

        # 4. Sample information
        self.declareProperty("SampleDensity", 1.00, Description="Sample density")
        self.declareProperty("SampleScatteringCrossSection", 1.00, 
                Description="Sample's total scattering cross-section")
        self.declareProperty("SampleRadius", 1.0,
                Description="Sample's radius")
        self.declareProperty("SampleHeight", 1.0,
                Description="Sample's height")
        self.declareProperty("VanadiumRadius", 1.0,
                Description="Sample's radius")
        self.declareProperty("VanadiumHeight", 1.0,
                Description="Vanadium rod's height")

        return

    def PyExec(self):
        """ generic stuff for running
        (Mantid required)
        """
        # 1. Get user input
        samRuns = self.getProperty("SampleRunNumber")
        canRuns = self.getProperty("CanRunNumber")
        bakRuns = self.getProperty("BackgroundRunNumber")
        vanRuns = self.getProperty("VanadiumRunNumber")
        characterfilename = self.getProperty("CharacterizationRunsFile")
        instrument = self.getProperty("Instrument")
        self.SofQdQ = self.getProperty("DeltaQ")
        detectorrangeinq = self.getProperty("DetectorRangeQ")
        self.infodict["sample"] = {
            "Density":              self.getProperty("SampleDensity"),
            "ScatteringXSection":   self.getProperty("SampleScatteringCrossSection"),
            "CylinderRadius":       self.getProperty("SampleRadius"),
            "Height":               self.getProperty("SampleHeight"),
            }
        self.infodict["vanadium"]["CylinderRadius"] = self.getProperty("VanadiumRadius")
        self.infodict["vanadium"]["Height"] = self.getProperty("VanadiumHeight")

        # 2. Process input
        self.setCharacterInformation(characterfilename)
        # a) Get workspace, generate dictionary and sort all workspaces by banks
        if instrument == "PG3":
            samruns = self.constructWorkspaceName(samRuns, instrument)
            bkgruns = self.constructWorkspaceName(bakRuns, instrument)
            canruns = self.constructWorkspaceName(canRuns, instrument)
            vanruns = self.constructWorkspaceName(vanRuns, instrument)
        # ENDIF
        # result: samruns = ["PG3_2581", "PG3_2582"] 
        # result: bkgruns = ["PG3_2585", "PG3_2586"] 
        # result: canruns = ["PG3_2583", "PG3_2584"] 
        # result: vanruns = ["PG3_2548", "PG3_2577"] 
        self.samwsdict = self.sortRuns(samruns)
        self.bkgwsdict = self.sortRuns(bkgruns)
        self.canwsdict = self.sortRuns(canruns)
        self.vanwsdict = self.sortRuns(vanruns)

        # b) Detector range
        self.SofQUserRange = self.parseDetectorRange(detectorrangeinq)
        # Result: 1: (2.80, 62.97),
        #         5: (0.74,  3.70)
        self.checkBankConsistency([self.samwsdict, self.bkgwsdict, self.canwsdict, self.vanwsdict])

        # 3. Rebin such that for each bank, sample, background, can and vanadium are
        #    in the same range and bin size
        for bankid in self.samwsdict.keys():
            self.unifySameBankWorkspaces(bankid)

        # 4. Process vanadium
        for bid in self.vanwsdict.keys():
            self.vanwsdict[bid] = self.processVanadium(self.vanwsdict[bid])

        # 4. Calculate S(Q) of each bank
        soqwsdict = {}
        factor = self.calTotalScatteringFactor()
        for bankid in self.samwsdict.keys():
            soqws = self.calSofQ(bankid, factor)
            soqwsdict[bankid] = soqws
        # ENDFOR

        # 5. Blend
        resultsoqwsname = "BlendedSofQ"
        rawsoqws = self.blendMultBankSofQ(soqwsdict, resultsoqwsname)

        SaveNexus(Filename="temp.nxs", InputWorkspace=rawsoqws)

        return


    def constructWorkspaceName(self, runs, instrument):
        """ Construct Workspace names from sample run number

        Exception:
        1. If there is no such workspace existing in Mantid
        """
        workspacenames = []
        for run in runs:
            wsname = instrument+"_"+str(run)
            ws = mtd[wsname]
            if ws is None:
                raise NotImplementedError("Workspace %s does not exist for run %s" % (wsname, str(run)))
            workspacenames.append(wsname)
        # ENDFOR

        return workspacenames

    def parseDetectorRange(self, detectorrangeinq):
        """ Parse input list to dictionary such that
        - key: bank ID
        - value: 2-tuple, Qmin, Qmax

        Return: dict
        """
        if len(detectorrangeinq)%3 != 0:
            raise NotImplementedError("Input of detector's ranges must be in format: detector, qmin, qmax, ... format")

        detrangedict = {}

        numdet = len(detectorrangeinq)/3
        for idet in xrange(numdet):
            bankid = int(detectorrangeinq[3*idet])
            qmin = detectorrangeinq[3*idet+1]
            qmax = detectorrangeinq[3*idet+2]
            if bankid < 0 or qmin < 0 or qmax <= qmin:
                raise NotImplementedError("Detector range error! Bank ID must > 0, Qmin must > 0, Qmax must > Qmin")
            detrangedict[bankid] = (qmin, qmax)
        # ENDFOR

        return detrangedict


    def checkBankConsistency(self, dictlist):
        """ Check whether all given dictionay with same Bank (i.e., key)

        Exception:
        1. If they don't have the consistency in bank IDs
        """
        # 1. Number of banks 
        dict0 = dictlist[0]
        for idict in xrange(1, len(dictlist)):
            dicti = dictlist[idict]
            if len(dict0.keys()) != len(dicti.keys()):
                raise NotImplementedError("0th (%s) and %d-th (%s) has different number of banks" % (
                    str(dict0.keys()), idict, str(dicti.keys())))
            # ENDIF
        # ENDFOR

        # 2. Bank IDs
        for idict in xrange(1, len(dictlist)):
            dicti = dictlist[idict]
            for bankid in dict0.keys():
                if not dicti.has_key(bankid):
                    raise NotImplementedError("0-th has BankID %s which cannot be found in %-th that has Bank IDs %s" % (
                        bankid, idict, str(dicti.keys())))
        # ENDFOR

        return


    def setCharacterInformation(self, characterfilename):
        """ Set the characterization file information

        If characterization file is not given, tehn 
        use default values.

        self.characterizeDict

        """ 
        self.characterDict = {}
        # frequency(Hz) center_wavelength(angstrom) bank_num vanadium_run empty_run d_min(angstrom) d_max(angstrom)
        if characterfilename == "":
            # Default
            self.characterDict[1] = {"frequency": 60, "center_wavelength": 0.533, "d_min": 0.1, "d_max":  2.20}
            self.characterDict[2] = {"frequency": 60, "center_wavelength": 1.066, "d_min": 0.2, "d_max":  3.20}
            self.characterDict[3] = {"frequency": 60, "center_wavelength": 1.599, "d_min": 0.5, "d_max":  4.25}
            self.characterDict[4] = {"frequency": 60, "center_wavelength": 2.665, "d_min": 1.1, "d_max":  6.50}
            self.characterDict[5] = {"frequency": 60, "center_wavelength": 3.731, "d_min": 1.5, "d_max":  8.50}
            self.characterDict[6] = {"frequency": 60, "center_wavelength": 4.797, "d_min": 1.5, "d_max":  10.3}
        else: 
            # Parse characterrise file r
            raise NotImplemetnedError("To be implemented soon!")

        return

    def sortRuns(self, runnumbers):
        """ Sort runs to a dictionary
        """
        # 1. Get workspace
        workspaces = []
        for runnum in runnumbers:
            ws = mtd[runnum]
            if ws is None:
                raise NotImplementedError("Run %s is not available in Mantid" % (runnum)) 
            else: 
                workspaces.append(ws)
        # ENDFOR

        # 2. Check unit and check
        for ws in workspaces:
            unit = ws.getAxis(0).getUnit().name()
            if unit != "MomentumTransfer":
                ConvertUnits(InputWorkspace=ws, outputWorkspace=ws, Target="MomentumTransfer", Emode="Elastic")
        # ENDFOR

        # 3. Find bank
        wsdict = {}
        for ws in workspaces:
            qmin = ws.dataX(0)[0]
            qmax = ws.dataX(0)[-1]
            dmin = 2*math.pi/qmax
            dmax = 2*math.pi/qmin
            
            dminbanklist = []
            dmaxbanklist = []
            deltamin = 0.0
            deltamax = 0.0
            for bankid in self.characterDict.keys():
                tempmin = abs(dmin-self.characterDict[bankid]["d_min"])
                tempmax = abs(dmax-self.characterDict[bankid]["d_max"]) 
                if len(dminbanklist) == 0:
                    # Initial setup
                    dminbanklist.append(bankid)
                    dmaxbanklist.append(bankid)
                    deltamin = tempmin
                    deltamax = tempmax 
                else:
                    # Compare
                    if abs(tempmin-deltamin) < 1.0E-8:
                        # case as ==
                        dminbanklist.append(bankid)
                    elif tempmin < deltamin:
                        # case as <
                        dminbanklist = [bankid]
                        deltamin = tempmin
                    # ENDIF

                    if abs(tempmax-deltamax) < 1.0E-8:
                        # case as ==
                        dmaxbanklist.append(bankid)
                    elif tempmax < deltamax:
                        # case as < 
                        dmaxbanklist = [bankid]
                        deltamax = tempmax
                # ENDIF
            # ENDFOR

            # Determine the bank

            bankset = set(dminbanklist) & set(dmaxbanklist)

            # Print out debug information
            print "Workspace %s: dmin = %f,  dmax = %f " % (str(ws), dmin, dmax)
            print "Candidates selected by dmin:"
            for bid in dminbanklist:
                print "\tBank %d: dmin = %f, dmax = %f" % (bid, self.characterDict[bid]["d_min"], self.characterDict[bid]["d_max"])
            print "Candidates selected by dmax:"
            for bid in dmaxbanklist:
                print "\tBank %d: dmin = %f, dmax = %f" % (bid, self.characterDict[bid]["d_min"], self.characterDict[bid]["d_max"])

            # Setup!!!
            if len(bankset) != 1:
                raise NotImplementedError("Bank %s are both selected" % (bankset))
            bindex = bankset.pop()
            wsdict[bindex] = ws

            print "Selected Bank %d\n" % (bindex)
        # ENDFOR

        return wsdict

    def unifySameBankWorkspaces(self, bankid):
        """ Rebin if necessary
        (1) Rebin if necessary
        (2) Convert to point data if not
        """
        samws = self.samwsdict[bankid] 
        vanws = self.vanwsdict[bankid] 
        canws = self.canwsdict[bankid] 
        bkgws = self.bkgwsdict[bankid] 

        samminq = samws.dataX(0)[0]
        sammaxq = samws.dataX(0)[-1]
        vanminq = vanws.dataX(0)[0]
        vanmaxq = vanws.dataX(0)[-1]
        canminq = canws.dataX(0)[0]
        canmaxq = canws.dataX(0)[-1]
        bakminq = bkgws.dataX(0)[0]
        bakmaxq = bkgws.dataX(0)[-1]

        # 1. Determine rebin necessary or not
        torebin = False

        minminq = min(samminq, vanminq, canminq, bakminq)
        maxminq = max(samminq, vanminq, canminq, bakminq)
        if abs(minminq-maxminq) > 1.0E-8: 
            torebin = True

        if torebin is False:
            minmaxq = min(sammaxq, vanmaxq, canmaxq, bakmaxq)
            maxmaxq = max(sammaxq, vanmaxq, canmaxq, bakmaxq)
            if abs(minmaxq-maxmaxq) > 1.0E-8: 
                torebin = True

        print "Rebin workspace = %s" % (torebin)

        # 2. Rebin if necessary
        if torebin is True:
            Rebin(InputWorkspace=samws, OutputWorkspace=samws, Params="%f,%f,%f"%(maxminq,-0.0004,minmaxq))
            Rebin(InputWorkspace=vanws, OutputWorkspace=vanws, Params="%f,%f,%f"%(maxminq,-0.0004,minmaxq))
            Rebin(InputWorkspace=canws, OutputWorkspace=canws, Params="%f,%f,%f"%(maxminq,-0.0004,minmaxq))
            Rebin(InputWorkspace=bakws, OutputWorkspace=bakws, Params="%f,%f,%f"%(maxminq,-0.0004,minmaxq))

        print "Finished Unifying Bank %d's Workspaces" % (bankid)

        return

    def processVanadium(self, vanws):
        """ Process vanadium
        """
        # 1. Record bin and range information
        qmin = vanws.dataX(0)[0]
        qmax = vanws.dataX(0)[-1]
        numhist = len(vanws.dataX(0))

        # 2. Remove peak
        ConvertUnits(InputWorkspace=vanws, OutputWorkspace=vanws, Target="dSpacing")
        ConvertToMatrixWorkspace(InputWorkspace=vanws, OutputWorkspace=vanws)
        StripVanadiumPeaks(InputWorkspace=vanws, OutputWorkspace=vanws, PeakWidthPercent=5.0)
        ConvertUnits(InputWorkspace=vanws, OutputWorkspace=vanws, Target="MomentumTransfer")

        # 3. Rebin if have to
        curqmin = vanws.dataX(0)[0]
        curqmax = vanws.dataX(0)[-1]
        if abs(curqmin-qmin) > 1.0E-8 or abs(curqmax-qmax) > 1.0E-8:
            Rebin(InputWorkspace=vanws, OutputWorkspace=vanws, Params="%f, %f, %f"%(qmin,-0.0004,qmax))
        # ENDIF
        
        return vanws


    def calSofQ(self, bankid, factor):
        """ Calculate S(Q) for one bank

        """
        print "Calculating S(Q) of Bank %d" % (bankid)

        samws = self.samwsdict[bankid] 
        vanws = self.vanwsdict[bankid] 
        canws = self.canwsdict[bankid] 
        bkgws = self.bkgwsdict[bankid] 

        # 0. Convert To PointData from Histogram if applied
        ConvertToPointData(InputWorkspace=samws, OutputWorkspace=samws)
        ConvertToPointData(InputWorkspace=vanws, OutputWorkspace=vanws)
        ConvertToPointData(InputWorkspace=canws, OutputWorkspace=canws)
        ConvertToPointData(InputWorkspace=bkgws, OutputWorkspace=bkgws)

        # 1. Container: Background removal and smooth
        canws -= bkgws
        canws = self.smoothHistogram(canws)

        # 2. Vanadium: Smooth (Vanadium has not background involved)
        vanws = self.smoothHistogram(vanws) 

        # 3. Sample
        bkgws = self.smoothHistogram(bkgws)
        samws -= bkgws

        # 4. S(Q) = Sample-Can
        soqwsname = "SofQ_Bank_"+str(bankid)
        CloneWorkspace(InputWorkspace=samws, OutputWorkspace=soqwsname)
        soqws = mtd[soqwsname]
        soqws -= canws

        # 5. Convert to MatrixWorkspace
        ConvertToMatrixWorkspace(InputWorkspace=soqws, OutputWorkspace=soqws)

        # 6. S(Q) = I_s/I_v
        soqws /= vanws

        # 7. Factor
        soqws *= factor

        print "Finished Calculation of S(Q) of Bank %d" % (bankid)

        return soqws


    def smoothHistogram(self, ws):
        """ Smooth histogram
        (1) Using FFTSmooth
        """
        smoothparams = self.smoothparams
        FFTSmooth(InputWorkspace=ws, OutputWorkspace=ws, Filter="Butterworth", 
                Params=smoothparams, IgnoreXBins=True, AllSpectra=True)
    
        return ws


    def calTotalScatteringFactor(self):
        """ Calculate total scattering factor
        Factor = (density_v X-section_v volume_v)/(density_s X-section_s volume_s)
        """
        import math

        density_sample   = self.infodict["sample"]["Density"]*0.60  # default value
        r_sample         = self.infodict["sample"]["CylinderRadius"]
        h_sample         = self.infodict["sample"]["Height"]
        scatterxs_sample = self.infodict["sample"]["ScatteringXSection"]
        volume_sample    = math.pi*r_sample**2*h_sample

        density_van   = self.infodict["vanadium"]["Density"]*0.60  # default value
        r_van         = self.infodict["vanadium"]["CylinderRadius"]
        h_van         = self.infodict["vanadium"]["Height"]
        scatterxs_van = self.infodict["vanadium"]["ScatteringXSection"]
        volume_van    = math.pi*r_sample**2*h_sample

        factor = density_van*scatterxs_van*volume_van/(density_sample*scatterxs_sample*volume_sample)

        print "Total scattering factor = %f (60 percent of sample density)" % (1.0/factor)

        return factor


    def blendMultBankSofQ(self, soqwsdict, soqwsname):
        """ Blend multi-bank S(Q) to a single S(Q)

        Return: Blended S(Q) Workspace
        """
        print "Blending S(Q) is started"

        # 1. Rebin to constant bin, extend range
        dq = self.SofQdQ
        unitybinparam = "%f,%f,%f" % (-dq*0.5, dq, 100)
        for bankid in soqwsdict.keys():
            soqws = soqwsdict[bankid]
            # 1.1 Rebin to constant binning
            qmin, qmax = self.SofQUserRange[bankid]
            newwsname = str(soqws)+"_const"
            soqws = self.reBin(str(soqws), newwsname, qmin, qmax, dq)

            # 1.2 Extend region to (0, 100)
            CloneWorkspace(InputWorkspace=soqws, OutputWorkspace=str(soqws)+"_Saved")
            Rebin(InputWorkspace=soqws, OutputWorkspace=soqws, Params=unitybinparam)
            self._setExtendedRegionToM1(soqws, qmin, qmax)
            soqwsdict[bankid] = soqws
        # ENDFOR
      
        # 2. Blend
        self._blendSofQ(soqwsdict, soqwsname, self.SofQUserRange)
        finalsoqws = mtd[soqwsname]

        print "Blending S(Q) is finished successfully"

        return finalsoqws

    def _setExtendedRegionToM1(self, workspace, qmin, qmax):
        """ Set the extended region of a Workspace to the status for 
        blending, i.e. for any data point with Q outside [qmin, qmax], 
        E[Q] = -1
        """
        if qmin > qmax:
            raise NotImplementedError("Input Error!")

        datax = workspace.dataX(0)
        datae = workspace.dataE(0)

        # 1. Set [q0, qmin)
        qindex = 0
        while datax[qindex] < qmin:
            datae[qindex] = -1.0
            qindex += 1
        # ENDWHILE

        # 2. Set (qmax, qf]
        qindex = len(datax)-1
        while datax[qindex] > qmax:
            datae[qindex] = -1.0
            qindex -= 1
        # ENDWHILE

        return


    def _blendSofQ(self, workspacedict, blendedsoqwsname, ranges):
        """ Blend S(Q) from multiple banks
    
        Arguments:
         - workspaces:   Workspace list
         - resultwsname: string
         - ranges:       list of 2-tuple of range of each bank
    
        Requirement:
          (1) All input workspaces must have same range and step
        """
        # 1. Set up constant and generate result workspce
        bankids = workspacedict.keys()
        datasize = len(workspacedict[bankids[0]].dataX(0))
        CloneWorkspace(InputWorkspace=workspacedict[bankids[0]], OutputWorkspace=blendedsoqwsname)
        bsoqws = mtd[blendedsoqwsname]
    
        # 2. Blending: Pixel by Pixel
        for qindex in xrange(datasize):
            # Per Pixel
            temps = 0.0
            tempe = -0.0
    
            for bindex in bankids:
                q = workspacedict[bindex].dataX(0)[qindex]
                s = workspacedict[bindex].dataY(0)[qindex]
                e = workspacedict[bindex].dataE(0)[qindex]
    
                # 2.1 Check whether e < 0
                if e > 0:
                    temps += s/(e*e)
                    tempe += 1/(e*e)
            # END-FOR
    
            # 2.3 Combine all data
            if tempe > 0:
                blende = 1/math.sqrt(tempe)
                blends = temps/tempe
            else:
                blende = 1.0
                blends = -0.0
    
            bsoqws.dataY(0)[qindex] = blends
            bsoqws.dataE(0)[qindex] = blende
    
        # END-FOR
    
        # 3. Taking care of low-Q
        # for bid in bankids:
        #     print "Bank %d:  Qmin = %f" % (bid, ranges[bid][0])

        qmin = ranges[bankids[0]][0] 
        for ri in xrange(1, len(bankids)):
            if ranges[bankids[ri]][0] < qmin:
                qmin = ranges[bankids[ri]][0]
        print "qmin = %f" % (qmin)
    
        self._extendToZero(bsoqws, qmin)
    
        return bsoqws


    def _extendToZero(self, inpws, qstart):
        """ Extend S(Q) to Q=0 from input Qmin
        """
        import math
        
        # FIXME - the proper algorithm should use Y = a Q^2 to fit the first several data points

        qs = inpws.dataX(0)
        ss = inpws.dataY(0)
        es = inpws.dataE(0)
       
        # 1. Find index of qstart-1
        iq0 = -1
        for iq in xrange(len(qs)-1):
            if qstart > qs[iq] and qstart <= qs[iq+1]:
                # In case of q_i < qstart < q_i+1
                if es[iq] <= 0.0:
                    iq0 = iq+1
                elif abs(qs[iq+1]-qstart) < abs(qstart-qs[iq]):
                    iq0 = iq+1
                else:
                    iq0 = q0
                break
            # ENDIF
        # ENDFOR

        if iq0 == -1:
            raise NotImplementedError("Workspace %s: Qstart = %f is out of range (%f, %f)" % (str(inpws), 
                qstart, qs[0], qs[-1]))

        # 2. Extrapolate
        cof = ss[iq0]/(qs[iq0]*qs[iq0])
        for qi in xrange(iq0):
            ss[qi] = cof*qs[qi]*qs[qi]
            es[qi] = math.sqrt(abs(ss[qi]))

        return



    def reBin(self, sourcewsname, targetwsname, x0, xf, dx):
        """ main rebin
        """
        print "POWGEN Rebin Starts"

        # 1. Name issue if sourcename == targetname
        if sourcewsname == targetwsname:
            newsourcename = sourcename+"_Original"
            RenameWorkspace(sourcewsname, newsourcename)
            sourcews = mtd[newsourcename]
        else: 
            sourcews = mtd[sourcewsname]
    
        # 1. Rebin 
        xarr = sourcews.dataX(0)
        yarr = sourcews.dataY(0)
        earr = sourcews.dataE(0)

        print "  On Workspace %s To (%f, %f, %f) from (%f, %f)" % (sourcewsname, x0, xf, dx,
                xarr[0], xarr[-1])
        if (x0 < xarr[0]) or (xf > xarr[-1]):
            print "WARNING!!!  User defined region (%f, %f) is outside of original data region (%f, %f)" % (x0, xf,
                    xarr[0], xarr[-1])
            print "            Some data points cannot be defined correctly"


        newx, newy, newe = self._rebin(xarr, yarr, earr, x0, xf, dx)
    
        # 2. Fill array with zero
        newx, newy, newe = self._fillArray(newx, newy, newe)
    
        # 3. Creat workspace
        CreateWorkspace(targetwsname, newx, newy, newe, 1, "MomentumTransfer")
        newws = mtd[targetwsname]
   
        print "Rebin is finished successfully"
    
        return newws

    def _rebin(self, X, Y, E, x0, xf, dx):
        """ rebin data set X, Y, E to a new set as
        (x0, xf) with constant space dx
    
        Input criteria
        1. X must be in ascending order
    
        Important:
        (1) if there is a data point corresponding to empty bin, then 
            its error is set to -0.0
        """
        import math
    
        # 0. check
        if X[0] >= X[1]:
            raise NotImplementedError("X is not in ascending order")
        if xf <= x0:
            raise NotImplementedError("x0 must be smaller than xf")
        if len(X) != len(Y) or len(Y) != len(E):
            raise NotImplementedError("X, Y, E are not of same size")
    
        # 1. create new list
        newlen = int((xf-x0)/dx)+1
        nX = []
        nY = []
        nE = []
        nBin = []
        for i in xrange(newlen):
            q = i*dx+x0
            nX.append(q)
            nY.append(0.0)
            nE.append(0.0)
            nBin.append(0)
    
        # 2. rebin
        for i in xrange(len(X)):
            q = X[i]
            if q >= x0 and q <= xf:
                # data in rebin() range
                newqindex = int((q-x0+0.5*dx)/dx)
                if newqindex >= newlen or newqindex < 0:
                    print "Point %d (q = %f) is out of defined region" % (newqindex, q)
                    continue
    
                # bin
                nBin[newqindex] += 1
                nY[newqindex] += Y[i]
                nE[newqindex] += E[i]*E[i]
            else:
                # data outside rebin() range
                pass
        # END-FOR
    
        # 3. Clean
        for i in xrange(newlen):
            if nBin[i] > 0:
                nY[i] = nY[i]/nBin[i]
                nE[i] = math.sqrt(nE[i])/nBin[i]
                if abs(nE[i]) < 1.0E-15:
                    nE[i] = -0.0
            else:
                nE[i] = -0.0
    
        return (nX, nY, nE)


    def _fillArray(self, xs, ys, es):
        """ Fill array Y by intropolation, if its uncertainty is <= 0.0
        """
        # 1. Locate x0 as min(x) with e > 0
        ix0 = -1
        for xi in xrange(len(ys)):
            if es[xi] > 0:
                ix0 = xi
                break
        if ix0 < 0:
            raise NotImplementedError("Impossible")
        xlow = xs[ix0]
        ylow = ys[ix0]
        elow = es[ix0]
    
        # 2. Locate xf as max(x) with e > 0
        ixf = -1
        for xi in xrange(len(ys)):
            if es[len(ys)-1-xi] > 0:
                ixf = len(ys)-1-xi
                break
        if ixf < 0:
            raise NotImplementedError("Impossible")
        xhigh = xs[ixf]
        yhigh = ys[ixf]
        ehigh = es[ixf]
    
        # 3. Fill
        numfill = 0
        inzero = False
        for xi in xrange(len(es)):
            if es[xi] <= 0.0:
                # Record points with E[i] <= 0
                if inzero is True:
                    iendzero = xi
                else:
                    inzero = True
                    istartzero = xi
                    iendzero = xi
                # END-IF-ELSE
                numfill += 1
            else:
                # Optionally treat previous E[i] <= 0 data points
                if inzero is True:
                    # Time to fill the array
                    inzero = False
                    if istartzero == 0: 
                        # Extrapolate to 0
                        for fi in xrange(istartzero, iendzero+1):
                            fraction = xs[fi]/xlow
                            ys[fi] = fraction*ylow
                            es[fi] = fraction*elow
                        # END-FOR
    
                    elif iendzero == len(xs)-1:
                        # Extraploate to MAX(x)
                        xmax = xs[len(ys)-1]
                        for fi in xrange(istartzero, iendzero+1):
                            fraction = (xmax-xs[fi])/(xmax-xhigh)
                            ys[fi] = fraction*yhigh
                            es[fi] = fraction*ehigh
                            pass
                        # END-FOR
    
                    else:
                        # Intrapolate
                        ix0 = istartzero-1
                        ixf = iendzero+1
                        for fi in xrange(istartzero, iendzero+1):
                            fraction = (xs[fi]-xs[ix0])/(xs[ixf]-xs[ix0])
                            ys[fi] = ys[ix0]+fraction*(ys[ixf]-ys[ix0])
                            es[fi] = es[ix0]+fraction*(es[ixf]-es[ix0])
                        # END-FOR
                # END-IF
            # END-IF-ELSE
        # END-FOR
    
        print "Number of Filled Points = %d" % (numfill)
    
        return (xs, ys, es)


mtd.registerPyAlgorithm(PowgenSoQCalculator())

