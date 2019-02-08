.. algorithm::

.. summary::

.. relatedalgorithms::

.. properties::

Description
-----------

Given an initial UB at some goniometer configuration, this algorithm facilitates
the 'linking' of the UBs across orientations - in other words, ensuring the
continuity of the indexing of reflections throughout reciprocal space allowing
for grouping of reflections and refinement.

On chopper instruments, when the sample is lowered into the
blockhouse there is often no possibility to adjust its position. When rotating the
crystal via the goniometer, since the crystal is likely not centred exactly, the
predicted peaks from the initial UB often do not capture the data. As well as 
consistently indexing the peaks, the algorithm also effectively carries out a U
matrix correction that accounts for sample miscentering. Use of this algorithm 
will result in a seperate UB matrix for each orientation which can then be used 
for integration. 

The algorithm requires a set of predicted peaks that have been generated from
the initial UB via goniometer rotation, a set of observed (found) peaks, and
the lattice parameters in order to calculate the B matrix. A search within a
Q-envelope is carried out in which all peaks within the envelope are screened
as potential 'matches' to the observed peaks by comparing dspacing values.

This approach works well in simple cases, but for large unit cells and
modulated structures where lots of peaks with small and similar dspacings
appear, it is useful to make use of the NumPeaks parameter. This sorts the
predicted peaks from largest to smallest dspacing and defines which peaks are
used for the first cycle. As the refinement of the UB is carried out, more
peaks at shorter d are added, encoded by the PeakIncrement parameter. This adds
a defined number of peaks after each cycle.

The main limitation of this approach is that the lattice parameters of the sample should be known accurately. It is recommended that at least 10 iterations are carried out, but in cases where the crystal is well centred and the goniometer angles are known accurately fewer iterations are necessary. 

Useage
-----------

**Example:**

.. code-block:: python 

    # WISH single crystal
    # demonstration of linkedUBs algorithm on D10 Ruby (cyle 18/1)

    import numpy as np
    from math import sin, cos, pi
    from mantid.simpleapi import *

    # lattice parameters
    a = 4.764407
    b = 4.76440
    c = 13.037904
    alpha = 90
    beta = 90
    gamma = 120

    # parameters for PredictPeaks 
    MinDSpacing = 0.5
    MaxDSpacing = 20
    MinWavelength = 0.8
    MaxWavelength = 9.3
    ReflectionCondition='Primitive'

    # parameters for LinkedUBs
    QTolerance = 0.5
    QDecrement = 0.95 
    DTolerance = 0.02 
    NumPeaks = 25
    PeakIncrement = 10
    Iterations = 10
    DeleteWorkspace = False


    # calculate phi axis at given omega
    def CalculatePhiAxis(omega,chi):
        """
        calculates the vector associated with the phi axis ready for SetGoniometer
        -omega axis is taken as (0,1,0)  in xyz right handed cartesian system
        -the phi axis is first defined at omega = 270
        -examples of useage:
        -phi axis at (270.54) is given by CalculatePhiAxis(0,54)
        -phi axis at (0,54) is given by CalculatePhiAxis(-90,54)
        -phi axis at (90,54) is given by CalculatePhiAxis(-180,54)
        """    
        # omega axis
        ux_omega = 0
        uy_omega = 1
        uz_omega = 0
        u_omega = np.array([ux_omega,uy_omega,uz_omega])
        # angle between phi axis and beam
        chi = chi * pi/180
        # phi axis at omega = 270
        ux_phi = cos(chi)
        uy_phi = sin(chi)
        uz_phi = 0
        u_phi = np.array([ux_phi, uy_phi, uz_phi])
        # the omega angle at which to calculate the phi axis 
        omega_axis_setting = omega * pi/180
        # rotation around omega axis
        c_omega = cos(omega_axis_setting)
        s_omega = sin(omega_axis_setting) 
        r1 = np.array([c_omega + ux_omega**2 * (1-c_omega), ux_omega * uy_omega * (1-c_omega) - uz_omega * s_omega, ux_omega * uz_omega * (1-c_omega) + uy_omega * s_omega])
        r2 = np.array([uy_omega * ux_omega * (1-c_omega) + uz_omega * s_omega, c_omega + uy_omega**2 * (1-c_omega), uy_omega * uz_omega * (1-c_omega) - ux_omega * s_omega])
        r3 = np.array([uz_omega * ux_omega * (1-c_omega) - uy_omega * s_omega, uz_omega * uy_omega * (1 - c_omega) + ux_omega * s_omega, c_omega + uz_omega**2 * (1-c_omega)])
        r = np.vstack([r1, r2, r3])
        # phi axis vector at given omega is the product of the rotation matrix and the initial phi axis vector at omega = 270
        u_phi_rotated = np.dot(r, u_phi)
        u_phi_rotated = np.round(u_phi_rotated,5)
        return u_phi_rotated 


    # calculate phi axis at omega = 270 
    u_phi_x, u_phi_y, u_phi_z = CalculatePhiAxis(0,54)

    # load and process 41598
    LoadRaw(Filename='/archive/NDXWISH/Instrument/data/cycle_18_1/WISH00041598.raw', OutputWorkspace='WISH00041598')
    CropWorkspace(InputWorkspace='WISH00041598', OutputWorkspace='WISH00041598', XMin=6000, XMax=99000)
    ConvertUnits(InputWorkspace='WISH00041598', OutputWorkspace='WISH00041598', Target='dSpacing', ConvertFromPointData=False)

    # load and process 41599
    LoadRaw(Filename='/archive/NDXWISH/Instrument/data/cycle_18_1/WISH00041599.raw', OutputWorkspace='WISH00041599')
    CropWorkspace(InputWorkspace='WISH00041599', OutputWorkspace='WISH00041599', XMin=6000, XMax=99000)
    ConvertUnits(InputWorkspace='WISH00041599', OutputWorkspace='WISH00041599', Target='dSpacing', ConvertFromPointData=False)

    # find peaks on 41598 and 41599
    FindSXPeaks(InputWorkspace='WISH00041598', PeakFindingStrategy='AllPeaks', ResolutionStrategy='AbsoluteResolution', XResolution=0.2, PhiResolution=2, TwoThetaResolution=2, OutputWorkspace='WISH00041598_find_peaks')

    FindSXPeaks(InputWorkspace='WISH00041599', PeakFindingStrategy='AllPeaks', ResolutionStrategy='AbsoluteResolution', XResolution=0.2, PhiResolution=2, TwoThetaResolution=2, OutputWorkspace='WISH00041599_find_peaks')

    # find and optimise UB on 41598 using lattice parameters
    FindUBUsingLatticeParameters(PeaksWorkspace='WISH00041598_find_peaks', a=a, b=b, c=c, alpha=alpha, beta=beta, gamma=gamma, NumInitial=10, Tolerance=0.1, Iterations=10)

    PredictPeaks(InputWorkspace='WISH00041598_find_peaks', WavelengthMin=MinWavelength, WavelengthMax=MaxWavelength, MinDSpacing=MinDSpacing, ReflectionCondition=ReflectionCondition, OutputWorkspace='WISH00041598_predict_peaks')

    OptimizeLatticeForCellType(PeaksWorkspace='WISH00041598_predict_peaks', CellType='Hexagonal', Apply=True)

    CopySample(InputWorkspace='WISH00041598_predict_peaks', OutputWorkspace='WISH00041598', CopyName=False, CopyMaterial=False, CopyEnvironment=False, CopyShape=False)

    # set gonio on 41598 and predict the peaks of 41599 
    SetGoniometer(Workspace='WISH00041598', Axis0='0,0,1,0,1', Axis1='25,{},{},{},-1'.format(u_phi_x, u_phi_y, u_phi_z))
    PredictPeaks(InputWorkspace='WISH00041598', WavelengthMin=MinWavelength, WavelengthMax=MaxWavelength, MinDSpacing=MinDSpacing, ReflectionCondition=ReflectionCondition, OutputWorkspace='WISH00041599_predict_peaks')

    # linkedUBs
    LinkedUBs(QTolerance=QTolerance,
              QDecrement=QDecrement, 
              DTolerance=DTolerance, 
              NumPeaks=NumPeaks,
              PeakIncrement=PeakIncrement,
              Iterations=Iterations, 
              a=a, 
              b=b, 
              c=c, 
              alpha=alpha,
              beta=beta,
              gamma=gamma,
              MinWavelength=MinWavelength,
              MaxWavelength=MaxWavelength,
              MinDSpacing=MinDSpacing, 
              MaxDSpacing=MaxDSpacing,
              ReflectionCondition=ReflectionCondition,
              Workspace='WISH00041599', 
              ObservedPeaks='WISH00041599_find_peaks', 
              PredictedPeaks='WISH00041599_predict_peaks', 
              LinkedPeaks='WISH00041599_linked_peaks', 
              LinkedPredictedPeaks='WISH00041599_linked_peaks_predicted',
              DeleteWorkspace=DeleteWorkspace)

.. categories::

.. sourcelink::
