# flake8: noqa
import mantid.simpleapi as mantid


def SaveFullprofSX(Pklist, filename):
    if type(Pklist) != list:
        Pklist = [Pklist]
    w = []
    for p in Pklist:
        w.append(mtd[p])
        f = open(filename, "w")
        f.write("TITLE\n")
        f.write("(3i4,2f12.2,i5,4f10.4)\n")
        f.write("  0 0 0\n")
        f.write("#  h   k   l      Fsqr       s(Fsqr)   Cod   Lambda\n")
        for j, k in enumerate(w):
            for i in range(0, k.rowCount()):
                ratio = k.row(i)['Intens'] / (k.row(i)['SigInt'] + 0.001)
                if k.row(i)['Wavelength'] >= 1.1 and k.row(i)['Intens'] > 0 and ratio > 3.0 and k.row(i)[
                    'DSpacing'] >= 1.0:
                    d = (k.row(i)['h'], k.row(i)['k'], k.row(i)['l'], k.row(i)['Intens'], k.row(i)['SigInt'], j + 1,
                         k.row(i)['Wavelength'])
                    s = "%4i%4i%4i%12.2f%12.2f%5i%10.4f" % d
                    f.write(s + "\n")
    f.close()


def ProcessVana(rnum, cycle):
    # Preparation of the V/Nd sphere run for SX normalization
    mantid.LoadRaw(Filename='/archive/NDXWISH/Instrument/data/cycle_' + cycle + '/WISH000' + str(rnum) + '.raw',
                   OutputWorkspace='Vana', LoadMonitors='Separate')
    mantid.CropWorkspace(InputWorkspace='Vana', OutputWorkspace='Vana', XMin=6000, XMax=99000)
    mantid.NormaliseByCurrent(InputWorkspace='Vana', OutputWorkspace='Vana')
    mantid.ConvertUnits(InputWorkspace='Vana', OutputWorkspace='Vana', Target='Wavelength')
    # create Abs Correction for V
    shape = '''<sphere id="V-sphere">
    <centre x="0.0"  y="0.0" z="0.0" />
    <radius val="0.0025"/>
    </sphere>'''
    mantid.CreateSampleShape('Vana', shape)
    mantid.SetSampleMaterial('Vana', SampleNumberDensity=0.0719, ScatteringXSection=5.197, AttenuationXSection=4.739,
                             ChemicalFormula='V0.95 Nb0.05')
    mantid.AbsorptionCorrection(InputWorkspace='Vana', OutputWorkspace='Abs_corr', ElementSize=0.5)
    # SphericalAbsorption(InputWorkspace='WISH00038428', OutputWorkspace='Abs_corr_sphere', SphericalSampleRadius=0.25)
    # correct Vanadium run for absorption
    mantid.Divide(LHSWorkspace='Vana', RHSWorkspace='Abs_corr', OutputWorkspace='Vana_Abs')
    mantid.DeleteWorkspace('Vana')
    mantid.DeleteWorkspace('Abs_corr')
    # Smoot data with redius 3
    mantid.SmoothNeighbours(InputWorkspace='Vana_Abs', OutputWorkspace='Vana_smoot', Radius=3)
    mantid.DeleteWorkspace('Vana_Abs')
    # SmoothData38428
    mantid.SmoothData(InputWorkspace='Vana_smoot', OutputWorkspace='Vana_smoot1', NPoints=300)
    mantid.DeleteWorkspace('Vana_smoot')
    # crop between 0.75 and 9.3 in lambda
    # CropWorkspace(InputWorkspace='WISH00038428_smoot1', OutputWorkspace='WISH00038428_smoot1', XMin=0.75, XMax=9.3)


def Norm_data(rnum, cycle):
    # Load and normalize SX data
    mantid.LoadRaw(
        Filename='/archive/Instruments$/NDXWISH/Instrument/data/cycle_' + cycle + '/WISH000' + str(rnum) + '.raw',
        OutputWorkspace='WISH000' + str(rnum), LoadMonitors='Separate')
    # ConvertToEventWorkspace(InputWorkspace='WISH000'+str(rnum), OutputWorkspace='WISH000'+str(rnum))
    mantid.CropWorkspace(InputWorkspace='WISH000' + str(rnum), OutputWorkspace='WISH000' + str(i), XMin=6000,
                         XMax=99000)
    mantid.ConvertUnits(InputWorkspace='WISH000' + str(rnum), OutputWorkspace='WISH000' + str(rnum),
                        Target='Wavelength')
    mantid.NormaliseByCurrent(InputWorkspace='WISH000' + str(rnum), OutputWorkspace='WISH000' + str(rnum))
    # normalize By vanadium PredictPeaks
    mantid.CropWorkspace(InputWorkspace='WISH000' + str(rnum), OutputWorkspace='WISH000' + str(rnum), XMin=0.75,
                         XMax=9.3)
    mantid.RebinToWorkspace(WorkspaceToRebin='Vana_smoot1', WorkspaceToMatch='WISH000' + str(rnum),
                            OutputWorkspace='Vana_smoot1')
    mantid.Divide(LHSWorkspace='WISH000' + str(rnum), RHSWorkspace='Vana_smoot1', OutputWorkspace='WISH000' + str(rnum))
    # remove spike in the data above 1e15 and -1e15
    mantid.ReplaceSpecialValues(InputWorkspace='WISH000' + str(rnum), OutputWorkspace='WISH000' + str(rnum), NaNValue=0,
                                InfinityValue=0, BigNumberThreshold=1e15, SmallNumberThreshold=-1e15)
    # Convert to Diffraction MD and Lorentz Correction
    mantid.ConvertToDiffractionMDWorkspace(InputWorkspace='WISH000' + str(rnum),
                                           OutputWorkspace='WISH000' + str(rnum) + '_MD',
                                           LorentzCorrection=True)


def Find_Peaks(rnum, long_cell, threshold, MaxPeaks, centroid_radius, edge_pixel):
    mantid.PeakDistanceThreshold = 0.9 * 3.14 / long_cell
    mantid.FindPeaksMD(InputWorkspace='WISH000' + str(rnum) + '_MD', PeakDistanceThreshold=PeakDistanceThreshold,
                       MaxPeaks=MaxPeaks, DensityThresholdFactor=threshold,
                       OutputWorkspace='WISH000' + str(rnum) + '_find_peaks', EdgePixels=edge_pixel)
    mantid.CentroidPeaksMD(InputWorkspace='WISH000' + str(rnum) + '_MD', PeakRadius=centroid_radius,
                           PeaksWorkspace='WISH000' + str(rnum) + '_find_peaks',
                           OutputWorkspace='WISH000' + str(rnum) + '_find_peaks')


def Find_UB_FFT(rnum, MinD, MaxD, Tolerance, Centering, CellType):
    mantid.FindUBUsingFFT(PeaksWorkspace='WISH000' + str(rnum) + '_find_peaks', MinD=MinD, MaxD=MaxD,
                          Tolerance=Tolerance)
    mantid.SelectCellOfType(PeaksWorkspace='WISH000' + str(rnum) + '_find_peaks', Centering=Centering, Apply=True,
                            CellType=CellType)
    # OptimizeCrystalPlacement(PeaksWorkspace='WISH000'+str(rnum)+'_find_peaks', ModifiedPeaksWorkspace='WISH000'
    # +str(rnum)+'_find_peaks', AdjustSampleOffsets=True)
    mantid.OptimizeLatticeForCellType(PeaMinWavelengthksWorkspace='WISH000' + str(rnum) + '_find_peaks', Apply=True,
                                      CellType=CellType, Tolerance=Tolerance)


def Find_UB_Latt(rnum, a, b, c, alpha, beta, gamma, Tolerance, Centering, CellType):
    mantid.FindUBUsingLatticeParameters('WISH000' + str(rnum) + '_find_peaks', a=a, b=b, c=c, alpha=alpha, beta=beta,
                                        gamma=gamma)
    mantid.SelectCellOfType(PeaksWorkspace='WISH000' + str(rnum) + '_find_peaks', Centering=Centering, Apply=True,
                            CellType=CellType)
    # OptimizeCrystalPlacement(PeaksWorkspace='WISH000'+str(rnum)+'_find_peaks', ModifiedPeaksWorkspace='WISH000'
    # +str(rnum)+'_find_peaks', AdjustSampleOffsets=True)
    mantid.OptimizeLatticeForCellType(PeaksWorkspace='WISH000' + str(rnum) + '_find_peaks', Apply=True,
                                      CellType=CellType,
                                      Tolerance=Tolerance)


def Predict_peak(rnum, MinWavelength, MinDSpacing, centroid_radius, ReflectionCondition):
    PredictPeaks(InputWorkspace='WISH000' + str(rnum) + '_find_peaks', WavelengthMin=MinWavelength,
                 MinDSpacing=MinDSpacing, ReflectionCondition=ReflectionCondition,
                 OutputWorkspace='WISH000' + str(rnum) + '_peaks')
    CentroidPeaksMD(InputWorkspace='WISH000' + str(rnum) + '_MD', PeakRadius=centroid_radius,
                    PeaksWorkspace='WISH000' + str(rnum) + '_peaks', OutputWorkspace='WISH000' + str(rnum) + '_peaks')


def Integrate_elips(rnum, RegionRadius, Scale, wDir, MinDSpacing, MinWavelength, UseOnePercentBackgroundCorrection):
    CloneWorkspace(InputWorkspace='WISH000' + str(rnum), OutputWorkspace='WISH000' + str(rnum) + '_Lorentz')
    ConvertUnits(InputWorkspace='WISH000' + str(rnum) + '_Lorentz', OutputWorkspace='WISH000' + str(rnum) + '_Lorentz',
                 Target='Wavelength')
    LorentzCorrection(InputWorkspace='WISH000' + str(rnum) + '_Lorentz',
                      OutputWorkspace='WISH000' + str(rnum) + '_Lorentz')
    IntegrateEllipsoidsTwoStep(InputWorkspace='WISH000' + str(rnum) + '_Lorentz',
                               PeaksWorkspace='WISH000' + str(rnum) + '_peaks', RegionRadius=RegionRadius,
                               OutputWorkspace='WISH000' + str(rnum) + '_elips_' + str(RegionRadius),
                               UseOnePercentBackgroundCorrection=UseOnePercentBackgroundCorrection)
    # SaveHKL(InputWorkspace='WISH000'+str(rnum)+'elips_'+str(RegionRadius), ScalePeaks=Scale, Filename=wDir+str(rnum)+
    # '_rad_'+str(RegionRadius)+'.hkl',MinDSpacing=MinDSpacing,MinWavelength=MinWavelength)


# ################# #
# User Defined Parameters      #3
# ################# #
Process_Vanadium = True
# Wotking Directory
wDir = '/home/zcm06287/NaCL_analysis/'
# Find Peaks
long_cell = 4
threshold = 100
MaxPeaks = 500
centroid_radius = 0.08
edge_pixel = 0
# Find UB 1=FFT 2=using Lattice 3=loadUB
FindUB = 3
# Parameter for Find UB_ FFT
# If centred cell use the primitive cell for the Min/Max.
MinD = 3
MaxD = 60
Tolerance = 0.2
# Cell parameter for Find UB_ Lattice
# If centred cell use the primitive cell
a = 4
b = 4
c = 4
alpha = 60
beta = 60
gamma = 60
Tolerance = 0.2
# The conventional cell type to use. Allowed values: [Cubic, Hexagonal, Rhombohedral, Tetragonal, Orthorhombic,
# Monoclinic, Triclinic]
CellType = 'Cubic'
# The centering for the conventional cell. Allowed values: [F, I, C, P, R]
Centering = 'F'
# Choose if integrate on predicted peaks(1) or on find peaks (2)
Integrate_predicted = 1
# Parameter For predicted peaks
# Select centering for the predicted peaks Allowed values: [Primitive, C-face centred, A-face centred, B-face centred,
#  Body centred, All-face centred, Rhombohedrally centred, obverse, Rhombohedrally centred, reverse,
#  Hexagonally centred, reverse?]
ReflectionCondition = 'All-face centred'
# min d spacing and lamda for the predicted peaks
MinDSpacing = 0.5
MinWavelength = 1.0
# Parameters for Intensity Vs Radius
RadiusEnd = 0.4
NumSteps = 20
# UseOnePercentBackgroundCorrection?
UseOnePercentBackgroundCorrection = False

if Process_Vanadium == True:
    ProcessVana(38428, '17_1')
else:
    LoadNexus(Filename=wDir + 'V_sphere_Coll_cycle16_5.nxs', OutputWorkspace='Vana_smoot1')

for i in range(38423, 38427):
    Norm_data(i, '17_1')
    Find_Peaks(i, long_cell, threshold, MaxPeaks, centroid_radius, edge_pixel)
    if FindUB == 1:
        Find_UB_FFT(i, MinD, MaxD, Tolerance, Centering, CellType)
    if FindUB == 2:
        Find_UB_Latt(i, a, b, c, alpha, beta, gamma, Tolerance, Centering, CellType)
    if FindUB == 3:
        LoadIsawUB(InputWorkspace='WISH000' + str(i) + '_find_peaks', Filename=wDir + str(i) + '.mat')
    if Integrate_predicted == 1:
        Predict_peak(i, MinWavelength, MinDSpacing, centroid_radius, ReflectionCondition)
    if Integrate_predicted == 2:
        CloneWorkspace(InputWorkspace='WISH000' + str(i) + '_find_peaks', OutputWorkspace='WISH000' + str(i) + '_peaks')
    PeakIntensityVsRadius(InputWorkspace='WISH000' + str(i) + '_MD', PeaksWorkspace='WISH000' + str(i) + '_peaks',
                          RadiusEnd=RadiusEnd, NumSteps=NumSteps, BackgroundInnerFactor=1, BackgroundOuterFactor=1.5,
                          OutputWorkspace='WISH000' + str(i) + '_Peaks_vs_radius')

r_int = [0.15]
for i in range(38222, 38227):
    for j in r_int:
        # integrate with peak radius r_int
        IntegratePeaksMD(InputWorkspace='WISH000' + str(i) + '_MD', PeakRadius=j, BackgroundInnerRadius=1.5 * j,
                         BackgroundOuterRadius=1.7 * j, PeaksWorkspace='WISH000' + str(i) + '_peaks',
                         OutputWorkspace='WISH000' + str(i) + '_MD_int_rad_' + str(j), ReplaceIntensity=False,
                         IntegrateIfOnEdge=False, UseOnePercentBackgroundCorrection=UseOnePercentBackgroundCorrection)
        # Save HKL
        # SaveHKL(InputWorkspace='WISH000' +str(i)+'_MD_int_rad_'+str(j), ScalePeaks=0.001, Filename=wDir+str(i)+
        # '_rad_'+str(j)+'.hkl',MinDSpacing=0.5,MinWavelength=1.2)

r_int = [0.3]
for i in range(38423, 38425):
    for j in r_int:
        Integrate_elips(i, j, 1, wDir, MinDSpacing, MinWavelength, UseOnePercentBackgroundCorrection)

SaveFullprofSX(['WISH00038222_elips_0.15', ], wDir + 'NaCl_Coll_38222_elips_bkg_corr.int')
