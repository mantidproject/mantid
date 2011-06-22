################################################################################
#
#  Mantid-PDF Calculator for POWGEN and NOMAD
#
#  Version 0.1 Prototype. 
#    (1) Hard coded for POWGEN Ni
#
#  Last Edit Location:
#  * 2011.06.16 bafei
#  * 2011.06.16 MBP  
#
#
################################################################################

_DEBUGOUTPUT = True
_DBPATH = "/home/wzz/Projects/Mantid-Project/Tests/PDF-01/"

binningQ = (0.02,0.02,40)

HUGE = 1.0E18

def prototypemain():
    """ Main method
    """
    # sampleruns = ["PG3_2581_event.nxs", "PG3_2582_event.nxs"]
    # vanruns    = ["PG3_2548_event.nxs", "PG3_2577_event.nxs"]
    # canruns    = ["PG3_2583_event.nxs", "PG3_2584_event.nxs"]
    # bkgdruns   = ["PG3_2585_event.nxs", "PG3_2586_event.nxs"]

    sampleruns = ["PG3_2581_event.nxs"]
    vanruns    = ["PG3_2548_event.nxs"]
    canruns    = ["PG3_2583_event.nxs"]
    bkgdruns   = ["PG3_2585_event.nxs"]

    # sampleruns = ["PG3_2582_event.nxs"]
    # vanruns    = ["PG3_2577_event.nxs"]
    # canruns    = ["PG3_2584_event.nxs"]
    # bkgdruns   = ["PG3_2586_event.nxs"]

    calibfilename = "PG3_FERNS_2656_2011_03_20.cal"
    infodict = {
            "sample":{},
            "van": {},
            }

    infodict["van"] = { 
            "AttenuationXSection":  2.8, 
            "ScatteringXSection":   5.1, 
            "SampleNumberDensity":  0.0721, 
            "CylinderSampleRadius": 0.3175}

    infodict["sample"] = {
            "AttenuationXSection":  4.49, 
            "ScatteringXSection":   18.5, 
            "SampleNumberDensity":  0.107,    # density vr = 6.0 g/cm^3... desnity ni = 8.912 g/cm^2
            "CylinderSampleRadius": 0.400}

    calculatePDF(sampleruns, vanruns, canruns, bkgdruns, calibfilename, infodict)

    return


def calculatePDF(sampleruns, vanruns, canruns, backgroundruns, calibfilename, infodict):
    """ Main process to calculate PDF
    """
    global _DBPATH 
    # 1. Check validity and create some accessory Workspace
    if len(sampleruns) != len(vanruns) or len(vanruns) != len(canruns) or \
            len(canruns) != len(backgroundruns):
        raise NotImplementedError("Input run numbers are not match!")

    banksize = len(sampleruns)
    CreateGroupingWorkspace(InstrumentName='POWGEN', GroupNames='POWGEN', 
            OutputWorkspace="grouping")

    # 2. Import, focus, smooth and background removal
    samplesws = []
    for bid in xrange(banksize):
        # 2.1 Import and focus
        samplews = importFocus(sampleruns[bid], calibfilename, "sample_"+str(bid))
        vanws = importFocus(vanruns[bid], calibfilename, "van_"+str(bid))
        CloneWorkspace(InputWorkspace=vanws, OutputWorkspace="van_test_1")
        canws = importFocus(canruns[bid], calibfilename, "can_"+str(bid))
        bkgdws = importFocus(backgroundruns[bid], calibfilename, "background_"+str(bid))

        # Debug export data
        exportWorkspace(samplews, "Sample_1_Raw_"+str(bid))
        exportWorkspace(vanws,    "Van_1_Raw_"+str(bid))
        exportWorkspace(canws,    "Can_1_Raw_"+str(bid))
        exportWorkspace(bkgdws,   "Background_1_Raw_"+str(bid))


        # 2.2 Peak removal
        vanws = processVanadium(vanws)
        exportWorkspace(vanws, "Van_2_NoPeak_"+str(bid))

        # 2.3 Background removal
        canws = removeHistogram(canws, bkgdws)
        exportWorkspace(canws, "Can_3_NoBkgd_"+str(bid))
        canws = smoothHistogram(canws)
        exportWorkspace(canws, "Can_4_Smoothed_"+str(bid))

        vanws = removeHistogram(vanws, bkgdws)
        exportWorkspace(vanws, "Van_3_NoBkgd_"+str(bid))
        vanws = smoothHistogram(vanws)
        exportWorkspace(vanws, "Van_4_Smoothed_"+str(bid))

        bkgdws = smoothHistogram(bkgdws)
        exportWorkspace(bkgdws, "Background_3_Smoothed_"+str(bid))
        samplews = removeHistogram(samplews, bkgdws)
        exportWorkspace(samplews, "Sample_4_Smoothed_"+str(bid))

        # 2.4 Sample-Can
        print "Check 2.4:  Mantid Workspaces    "
        for wsname in mtd.keys():
            print wsname
        print "---------------------------------"
        samplews = removeHistogram(samplews, canws)
        exportWorkspace(samplews, "Sample_5_Pure_"+str(bid))

        # 2.5 Corrections (multiple, absorption)
        # vanws = correctDiffraction(vanws, infodict["van"])
        # samplews = correctDiffraction(samplews, infodict["sample"])

        # exportWorkspace(vanws, "Van_5_Corrected"+str(bid))
        # exportWorkspace(samplews, "Sample_6_Corrected"+str(bid))

        # 2.7 Make SoQ
        soqwsname = "soq_"+str(bid)
        CloneWorkspace(InputWorkspace=samplews, OutputWorkspace=soqwsname)
        soqws = mtd[soqwsname]
        soqws /= vanws
        exportWorkspace(soqws, "SoQ_1_"+str(bid))
        
        # 2.6 Output
        samplesws.append(soqws)
    # END-FOR: bid

    # 3. Blend and normalize again by lim_q->infty (s-b)/(v-b)
    range1 = (2.5, 4.5)
    range0 = (2.8, 40.0)
    # soqws = blendBanks(samplesws, "SoQ_Blended", [range0, range1])
    soqws = blendBanks(samplesws, "SoQ_Blended", [range0])

    factor = calSoqFactor()
    soqws /= factor

    exportWorkspace(soqws, "SoQ_2")

    # 4. Fourier transform
    gorws = pdfFourierTransform(soqws, "GoR")

    return 


def importFocus(runnumber, calibfilename, workspacename, instrument="POWGEN"):
    """ Import data from a run number and do diffraction focussing
        (1) Focus on Q-space
        (2) TOF -> q-space -> focus
    """
    global binningQ

    # 1. Construct file name 
    if runnumber.endswith(".nxs"): 
        filename = runnumber
    else:
        filename = runnumber+"_event.nxs"

    # 2. Load
    LoadEventNexus(Filename=filename, OutputWorkspace=workspacename,CompressTolerance=.01)
    # result: tof, bins = 1, histogram = many
    wksp = mtd[workspacename]

    # 3. Reduce
    # Cause Error: FilterBadPulses(InputWorkspace=wksp, OutputWorkspace=wksp)
    SortEvents(InputWorkspace=wksp)
    AlignDetectors(InputWorkspace=wksp, OutputWorkspace=wksp, CalibrationFile=calibfilename) 
    Rebin(wksp, wksp, Params=(0.,.001,20.))
    # result: d-spacing, bins = 20000, histogram = many

    # 3.5 Make POWGEN's peaks sharper
    if instrument == "POWGEN":
        ConvertUnits(InputWorkspace=wksp, OutputWorkspace=wksp, Target='TOF')
        UnwrapSNS(InputWorkspace=wksp, OutputWorkspace=wksp, LRef=62)   # 62 is flying path
        RemoveLowResTOF(InputWorkspace=wksp, OutputWorkspace=wksp, ReferenceDIFC=15000.00,
                K=3.22)     # ReferenceDIFC and K are given by Jason

    elif instrument == "NOMAD":
        pass

    else:
        raise NotImplementedError("Instrument %s is not supported")

    # 4. Focus
    ConvertUnits(InputWorkspace=wksp, OutputWorkspace=wksp, Target='MomentumTransfer')
    DiffractionFocussing(InputWorkspace=wksp, OutputWorkspace=wksp, 
            GroupingWorkspace="grouping", PreserveEvents=True)
    Rebin(InputWorkspace=wksp, OutputWorkspace=wksp, Params=binningQ)    
    # result: q-spacing, bins = 999, histogram = 1

    # 5. Normalize
    NormaliseByCurrent(InputWorkspace=wksp, OutputWorkspace=wksp)

    # 6. Convert to Matrix Workspace
    ConvertToMatrixWorkspace(wksp, wksp)
    
    return wksp


def processVanadium(vanws):
    """ Special treatment for Vanadium data
    """
    ConvertUnits(InputWorkspace=vanws, OutputWorkspace=vanws, Target="dSpacing")
    StripVanadiumPeaks(InputWorkspace=vanws, OutputWorkspace=vanws, PeakWidthPercent=5.0)
    ConvertUnits(InputWorkspace=vanws, OutputWorkspace=vanws, Target="MomentumTransfer")
    
    return vanws


def smoothHistogram(ws):
    """ Smooth histogram
    (1) Using FFTSmooth
    """
    smoothparams = "100,2"
    FFTSmooth(InputWorkspace=ws, OutputWorkspace=ws, Filter="Butterworth", 
            Params=smoothparams, IgnoreXBins=True, AllSpectra=True)

    return ws


def removeHistogram(samplews, bkgdws):
    """ remove background 
    (1) remove
    (2) compress
    """
    samplews -= bkgdws
    #CompressEvents(InputWorkspace=samplews, OutputWorkspace=samplews, 
    #        Tolerance=COMPRESS_TOL_Q)

    return samplews


def correctDiffraction(ws, infodict):
    """ Perform multiple scattering and absoprtion correction to a histogram
    """
    att = infodict["AttenuationXSection"]
    sca = infodict["ScatteringXSection"]
    sam = infodict["SampleNumberDensity"]
    cyl = infodict["CylinderSampleRadius"]
    MultipleScatteringCylinderAbsorption(InputWorkspace=ws, OutputWorkspace=ws,
            AttenuationXSection=att, ScatteringXSection=sca, SampleNumberDensity=sam,
            CylinderSampleRadius=cyl)

    return ws


def blendBanks(workspaces, resultwsname, ranges):
    """ Blend S(Q) from multiple banks

    Arguments:
     - workspaces:   Workspace list
     - resultwsname: string
     - ranges:       list of 2-tuple of range of each bank

    Requirement:
      (1) All input workspaces must have same range and step
    """
    import math
    global HUGE

    # 1. Check size
    ws0 = workspaces[0]
    datax0 = ws0.dataX(0)

    for iws in xrange(1, len(workspaces)):
        datax = workspaces[iws].dataX(0)
        if datax[0]!=datax0[0] or datax[-1]!=datax0[-1] or len(datax)!=len(datax0):
            print "Workspace %s's range is not same as that of workspace %s" % \
                    (str(workspaces[iws]), str(ws0))
            raise NotImplementedError("")
    # END-FOR

    if len(ranges)!=len(workspaces):
        raise NotImplementedError("Number of ranges is not same as number of Workspaces")

    # 2. Blending: Pixel by Pixel
    numws = len(workspaces)
    CloneWorkspace(InputWorkspace=ws0, OutputWorkspace=resultwsname)
    rws = mtd[resultwsname]

    for qindex in xrange(len(datax0)-1):
        # Per Pixel
        temps = 0.0
        tempe = 0.0

        for bindex in xrange(numws):
            q = workspaces[bindex].dataX(0)[qindex]
            s = workspaces[bindex].dataY(0)[qindex]
            e = workspaces[bindex].dataE(0)[qindex]

            # 2.1 set E to HUGE if out of range
            if q < ranges[bindex][0] or q > ranges[bindex][1]:
                s = 1.0
                e = HUGE

            # 2.2 Integrate
            try:
                temps += s/(e*e)
                tempe += 1/(e*e)
            except ZeroDivisionError:
                print "zero @ %d, %d: %f, %f" % (bindex, qindex, s, e)
                temps += 0
                tempe += 0
        # END-FOR

        # 2.3 Combine all data
        try:
            blende = 1/math.sqrt(tempe)
            blends = temps/tempe
        except ZeroDivisionError:
            blende = 1.0
            blends = 1.0

        rws.dataY(0)[qindex] = blends
        rws.dataE(0)[qindex] = blende

    return rws


def pdfFourierTransform(soqws, grworkspacename):
    """ PDF Fourier transform

    Thomas PDFgetN: Q = 2.8 ~ 62.97, 0.74 ~ 3.7... Combined S(q): Q = 0.74 ~ 40
    """ 
    PDFFourierTransform(InputWorkspace=soqws, OutputWorkspace=grworkspacename, 
            Rmax=20.0, DeltaR=0.02, Qmin=2.8, Qmax=40)
    
    grws = mtd[grworkspacename]

    return grws


def exportWorkspace(workspace, ofilename):
    """ Export workspace to an outer file
    """
    global _DBPATH

    filename = _DBPATH+ofilename+".dat"
    SaveAscii(Workspace=workspace, Filename=filename)

    return

def calSoqFactor():
    """ Calculate 
    lim q->infty (S/p - B_s/p) / (V/p - B_v/p) = 
        (sigma_s Omega_s V_s)/(sigma_v Omega_v V_v)
    where, 
    (1) P is proton charge
    (2) sigma is atom's density
    (3) Omega is atom's neutron scattering length
    (4) V is volume
    """
    # TODO  This is a fake/hardcoded piece of codes.  Should be replaced later

    ni_density = 8.908
    ni_scatterxs = 18.5
    ni_volume = 4*4*4
    
    v_density = 6.0
    v_scatterxs = 5.1
    v_volume = 3.75*3.75*3.75
    
    soq_infty = ni_density*ni_scatterxs*ni_volume/(v_density*v_scatterxs*v_volume)
    
    print soq_infty

    return soq_infty


if __name__=="__main__":
    prototypemain()
