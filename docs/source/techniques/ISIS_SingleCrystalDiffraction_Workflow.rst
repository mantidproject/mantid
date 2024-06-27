.. _isis-single-crystal-diffraction-ref:

=================================================
ISIS Single Crystal Diffraction Reduction Scripts
=================================================

.. contents:: Table of Contents
    :local:

Introduction
------------

The reduction of single-crystal diffraction data from the SXD and WISH instruments at ISIS is
achieved with the use of instrument specific classes (``SXD`` and ``WishSX`` respectively).

The instrument specific classes inherit from a base class ``BaseSX`` that contains code common to the
workflow of all single-crystal diffraction reduction instruments - the user shouldn't interact directly with this class.

Basic Usage
-----------

During beamtime, users may want to load data, find peaks and optimise UB matrices etc. without performing the
time-consuming normalisation and correction procedures necessary when reducing the data for refinement.

These simple operations have been made concise with the use of `static` methods in the reduction classes - i.e.
the user can call these methods without a creating an instance of the class. These static methods can be
combined in a script with mantid algorithms in the usual way.

Here is an example using the ``SXD`` class that finds peaks and then removes duplicates

..  code-block:: python

  from mantid.simpleapi import *
  from Diffraction.single_crystal.sxd import SXD

  ws = Load(Filename='SXD33335.nxs', OutputWorkspace='SXD33335')

  # find peaks using SXD static method - determines peak threshold from
  # the ratio of local variance over mean
  peaks_ws = SXD.find_sx_peaks(ws, ThresholdVarianceOverMean=2.0)
  SXD.remove_duplicate_peaks_by_qlab(peaks_ws, q_tol=0.05)

  # find a UB
  FindUBUsingFFT(PeaksWorkspace=peaks_ws, MinD=1, MaxD=10)

  # use another static method to remove duplicates by hkl index
  SXD.remove_duplicate_peaks_by_hkl(peaks_ws)

.. _isis-single-crystal-diffraction-sxd-ref:

SXD Workflow
------------

.. _sxd-detector_calibration-ref:

1. Detector Calibration
^^^^^^^^^^^^^^^^^^^^^^^

The positions of the SXD detector panels require calibration - typically using an NaCl crystal (standard sample).
The updated positions are stored in an .xml file, the path to the file is returned by `calibrate_sxd_panels`.

..  code-block:: python

  sxd = SXD()
  wsname = sxd.load_run(32863)
  peaks_ws = sxd.find_sx_peaks(wsname)
  sxd.remove_peaks_on_detector_edge(peaks_ws, 2)

  # initial UB
  FindUBUsingFFT(PeaksWorkspace=peaks_ws , MinD=1, MaxD=10)
  # force lattice parameters to be correct
  SelectCellWithForm(PeaksWorkspace=peaks_ws, FormNumber=1, Apply=True, Tolerance=0.15)
  OptimizeLatticeForCellType(PeaksWorkspace=peaks_ws, Apply=True, Tolerance=0.15)
  IndexPeaks(PeaksWorkspace=peaks_ws, Tolerance=0.15, CommonUBForAll=False)
  CalculateUMatrix(PeaksWorkspace=peaks_ws, a=5.6402, b=5.6402, c=5.6402,
                   alpha=90, beta=90, gamma=90)

  save_dir = r"<put-correct-save-path>"
  detcal_path = sxd.calibrate_sxd_panels(wsname, peaks_ws, save_dir, tol=0.2)

2. Reduce a sequence of runs
^^^^^^^^^^^^^^^^^^^^^^^^^^^^

Once the detectors have been calibrated a sequence of runs can be reduced.

a. Setting up the SXD object
============================

In order to reduce a sequence of runs, an instance of the ``SXD`` class needs to be initiated with a vanadium and empty
run number and optionally the detector calibration file from :ref:`sxd-detector_calibration-ref`.

If sample absorption is to be corrected for, the sample shape/geometry and material can be set using ``set_sample``
as in :ref:`algm-SetSample` - note the default units for the number density are `formula units per cubic Angstrom`.

Typically each run in a sequence corresponds to a different sample orientation, in which case it is necessary to set the
goniometer axes using ``set_goniometer`` as defined in :ref:`algm-SetGoniometer`.
The goniometer angles are provided later.

..  code-block:: python

    sxd = SXD(vanadium_runno=32857, empty_runno=32856, detcal_path=detcal_path)
    sxd.set_sample(Geometry={'Shape': 'CSG', 'Value': sxd.sphere_shape},
                   Material={'ChemicalFormula': 'Na Cl', 'SampleNumberDensity': 0.0223})
    sxd.set_goniometer_axes([0,1,0,1])  # ccw rotation around vertical

b. Running the reduction
========================

The reduction proceeds as follows:

- Process vanadium
- Load and normalise data
- Find peaks
- Optimise a UB
- Integrate peaks
- Export peaks for refinement

..  code-block:: python

    from mantid.simpleapi import *
    from os import path
    from Diffraction.single_crystal.base_sx import PEAK_TYPE, INTEGRATION_TYPE
    from Diffraction.single_crystal.sxd import SXD

    # Setup SXD
    ###########

    sxd = SXD(vanadium_runno=32857, empty_runno=32856, detcal_path=detcal_path)
    sxd.set_sample(Geometry={'Shape': 'CSG', 'Value': sxd.sphere_shape},
                   Material={'ChemicalFormula': 'Na Cl', 'SampleNumberDensity': 0.0223})
    sxd.set_goniometer_axes([0,1,0,1])  # ccw rotation around vertical

    # load and normalise data
    #########################

    runs = range(32863,32865)
    sxd.process_vanadium()
    sxd.process_data(runs, "wccr") # wccr is the goniometer motor log name - one arg for each axis added
    # if there was no log value then the angles have to be set with a sequence e.g.
    # sxd.process_data(runs, [0,45])

    # Find peaks and optimise UBs
    ##############################

    for run in runs:
        ws = sxd.get_ws(run)
        peaks = sxd.find_sx_peaks(ws)
        sxd.remove_peaks_on_detector_edge(peaks, 2)
        sxd.set_peaks(run, peaks)
    # find UB with consistent indexing across runs
    sxd.find_ub_using_lattice_params(global_B=True, tol=0.15,
                                     a=5.6402, b=5.6402, c=5.6402,
                                     alpha=90, beta=90, gamma=90)
    sxd.calibrate_sample_pos(tol=0.15)  # calibrates each run independently

    # Integrate and save
    ####################

    # input arguments for skew integration
    save_dir = "/babylon/Public/UserName"
    skew_args= {'UseNearestPeak': True, 'IntegrateIfOnEdge': False,
                'LorentzCorrection': True,
                'NVacanciesMax': 2, 'NPixPerVacancyMin': 2, 'NTOFBinsMin': 2,
                'UpdatePeakPosition': True, 'OptimiseMask': True,
                'GetTOFWindowFromBackToBackParams': False,
                'BackScatteringTOFResolution': 0.06, 'ThetaWidth': 1.5,
                'ScaleThetaWidthByWavelength': True,
                'OptimiseXWindowSize': True, 'ThresholdIoverSigma': 15}

    # integrate and save each run
    peak_type = PEAK_TYPE.FOUND
    integration_type = INTEGRATION_TYPE.SKEW
    for run in runs:
        skew_args = {**skew_args, 'OutputFile': path.join(save_dir, f"{run}_{peak_type.value}_int.pdf")}
        sxd.integrate_data(integration_type, peak_type, run=run, **skew_args)
        sxd.remove_non_integrated_peaks(sxd.get_peaks(run, peak_type, integration_type))
        sxd.calc_absorption_weighted_path_lengths(peak_type, integration_type, run=run, ApplyCorrection=True)
        sxd.save_peak_table(run, peak_type, integration_type, save_dir, save_format='SHELX')

    # save combined table
    sxd.save_all_peaks(peak_type, integration_type, save_dir=save_dir, save_format='SHELX')

.. _isis-single-crystal-diffraction-wish-ref:

WISH Workflow
-------------

1. Preliminary integration
^^^^^^^^^^^^^^^^^^^^^^^^^^

During beamtime users may want to check that a run has been counted sufficiently. This
involves saving the data with a different file extension e.g. ``.s01``.

This example shows how to load data with specific file extension and perform an integration in Q-space.

..  code-block:: python

    from mantid.simpleapi import *
    from os import path
    from Diffraction.wish.wishSX import WishSX

    # integration parameters
    intPeaksMDArgs = {'ellipsoid': True, 'fixQAxis': True, 'fixMajorAxisLength': True, 'useCentroid': True, 'MaskEdgeTubes': False}

    ws = WishSX.load_run(run, file_ext=".s01")
    # convert data to Q for integration
    WishSX.mask_detector_edges(ws, nedge=16, ntubes=2)
    wsMD = WishSX.convert_ws_to_MD(ws, frame="Q (lab frame)")
    # find peaks and integrate
    peaks = WishSX.find_sx_peaks(ws)
    peaks_int = WishSX().integrate_peaks_MD_optimal_radius(wsMD, peaks, peaks+"_int", ws=ws, **intPeaksMDArgs)

Note that the method ``integrate_peaks_MD_optimal_radius`` requires an instance of the class ``WishSX()`` - this is
because the optimal radius for the integration depends on the instrument, but the method is defined in the base class as
it is common to both SXD and WISH.

The default file extension (used in ``process_vanadium`` and ``process_data``) is stored as an attribute on the class
which can be set at instantiation as follows

..  code-block:: python

    wish = WishSX(file_ext=".s01")

It can also set at any point directly (e.g. after the vanadium run has been processed and to reduce a single run for
preliminary refinement).

..  code-block:: python

    wish.file_ext = ".s01"

2. Finding the UBs for a sequence of runs
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

The WISH workflow finds UBs before the reduction and exports the UB matrix for every run post rotation by the goniometer
matrix. The UBs are then loaded in the subsequent reduction, which then doesn't require the goniometer information, and
the predicted peak positions are integrated (in contrast to SXD where the found peaks are integrated)..

Here is an example that finds peaks, integrates them in Q-space and filters by I/sigma to only use the strongest peaks
in the UB optimisation. The data were collected using the WISH goniometer with 2 axes of rotation (phi and omega), in
this case a positive angle (omega) correposnds to a counter-clockwise rotation around the vertical axis as viewed from
the top - i.e. the goniometer axis is ``Axis0=str(omegas[irun])+',0,1,0,1'``.
For data collected in the Newport with only a vertical axis of rotation a positive angle corresponds to a clockwise
rotation and the correct goniometer axis is ``Axis0=str(omegas[irun])+',0,1,0,-1'`` (note the sign of the last number
has changed).

..  code-block:: python

    from mantid.simpleapi import *
    from os import path
    from Diffraction.wish.wishSX import WishSX

    # lattice parameters
    a, b, c, alpha, beta, gamma = 12.2738, 12.2738, 12.2738, 90.0, 90.0, 90.0

    # integration parameters
    intPeaksMDArgs = {'ellipsoid': True, 'fixQAxis': True, 'fixMajorAxisLength': True, 'useCentroid': True, 'MaskEdgeTubes': False}

    # goniometer angles (one for each run)
    omegas = [7.0, 50.0, 153.0]
    phis = [181.0, 247.0, 93.0]
    runs = range(42730, 42733)

    pk_ws_list = []
    for irun, run in enumerate(runs):
        ws = WishSX.load_run(run)
        SetGoniometer(Workspace=ws, Axis0=str(omegas[irun])+',0,1,0,1',
                      Axis1=str(phis[irun]) + ',1,1,0,1')
        # SetGoniometer(Workspace=ws, Axis0=str(omegas[irun])+',0,1,0,-1') # if using newport
        # convert data to Q for integration
        WishSX.mask_detector_edges(ws, nedge=16, ntubes=2)
        wsMD = WishSX.convert_ws_to_MD(ws, frame="Q (lab frame)")
        # find peaks and integrate
        peaks = WishSX.find_sx_peaks(ws)
        peaks_int = WishSX().integrate_peaks_MD_optimal_radius(wsMD, peaks, peaks+"_int", ws=ws, **intPeaksMDArgs)
        peaks_int = peaks_int.name()
        # filter to get strong peaks only
        FilterPeaks(InputWorkspace=peaks_int, OutputWorkspace=peaks_int, FilterVariable="Signal/Noise",
                    FilterValue=10, Operator=">")
        # get a rough UB and keep indexed
        FindUBUsingFFT(peaks_int, MinD=1, MaxD=20)
        SelectCellOfType(PeaksWorkspace=peaks_int, Centering='I', Apply=True)
        IndexPeaks(peaks_int, RoundHKLs=True, CommonUBForAll=False)
        CalculateUMatrix(peaks_int, a=a, b=b, c=c, alpha=alpha, beta=beta, gamma=gamma)
        WishSX.remove_unindexed_peaks(peaks_int)
        pk_ws_list.append(peaks_int)

    # find a UB per run with consistent indexing across the sequence
    FindGlobalBMatrix(PeakWorkspaces=pk_ws_list, a=a, b=b, c=c, alpha=alpha, beta=beta, gamma=gamma, Tolerance=0.15)

    # save the UBs
    save_dir = "/babylon/Public/UserName"
    for irun, peaks in enumerate(pk_ws_list):
        SaveIsawUB(InputWorkspace=peaks, Filename=path.join(save_dir,  f'{runs[irun]}.mat'),
                   RotateByGoniometerMatrix=True)


3. Reduce a sequence of runs
^^^^^^^^^^^^^^^^^^^^^^^^^^^^

After a UB matrix has been optimised for each run, the entire sequence of runs can now be reduced.

The method ``predict_peaks`` predicts peaks based on the `enum` class ``PEAK_TYPE``:

- ``FOUND``
- ``PREDICT``
- ``PREDICT_SAT``

The method takes the keyword arguments for :ref:`algm-PredictPeaks`
and :ref:`algm-PredictFractionalPeaks` for ``PEAK_TYPE.PREDICT`` and ``PEAK_TYPE.PREDICT_SAT`` respectively.

The integration algorithm called by ``integrate_data`` will integrate the peak table based on the ``PEAK_TYPE`` supplied
using an agorithm determined by the `enum` class ``INTEGRATION_TYPE``:

- ``MD``
- ``MD_OPTIMAL_RADIUS``
- ``SKEW``

The Q-space integration methods are ``MD`` a ``MD_OPTIMAL_RADIUS`` - the latter estimates an appropriate peak radius for
each peak, the former method requires the user to supply the peak radius to ``integrate_data`` using keyword arguments
as in :ref:`algm-IntegratePeaksMD-v2`.

a. Skew Integration (no satellites)
===================================

..  code-block:: python

    from mantid.simpleapi import *
    from os import path
    from Diffraction.single_crystal.base_sx import PEAK_TYPE, INTEGRATION_TYPE
    from Diffraction.wish.wishSX import WishSX

    # Setup WishSX
    ##############

    wish = WishSX(vanadium_runno=43526)
    # set sample material
    sphere = '''<sphere id="sphere">
                <centre x="0.0"  y="0.0" z="0.0" />
                <radius val="0.0009"/>
                </sphere>'''  # sphere radius 0.9mm
    wish.set_sample(Geometry={'Shape': 'CSG', 'Value': sphere_GGG},
                    Material={'ChemicalFormula': 'Ca3-Ga2-Ge3-O12', 'SampleNumberDensity': 0.001086})

    # load vanadium
    ###############

    wish.process_vanadium()

    # Integrate and save
    ####################

    save_dir = "/babylon/Public/UserName"
    # integration parameters
    intPeaksSkewArgs = {'UseNearestPeak': True, 'IntegrateIfOnEdge': False,
                        'NRowsEdge': 5, 'NColsEdge':2,
                        'NVacanciesMax': 2, 'NPixPerVacancyMin': 2, 'NTOFBinsMin': 2,
                        'UpdatePeakPosition': True, 'OptimiseMask': True,
                        'GetTOFWindowFromBackToBackParams': True, 'NFWHM': 10,
                        'LorentzCorrection': True, 'ScaleThetaWidthByWavelength': True,
                        'OptimiseXWindowSize':True, 'ThresholdIoverSigma': 80}

    for run in range(42730, 42733):
        wish.process_data([run])
        # load UB
        wish.load_isaw_ub(path.join(save_dir, f'{run}.mat'), tol=0.15, run=run)
        # predict peaks
        wish.predict_peaks(MinDSpacing=0.75, WavelengthMin=0.85,
                           ReflectionCondition='Body centred', run=run)
        # Integrate
        skew_kwargs = {'OutputFile': path.join(save_dir, f'{run}_integrated.pdf'),
                       **intPeaksSkewArgs}
        wish.integrate_data(INTEGRATION_TYPE.SKEW, PEAK_TYPE.PREDICT, run=run, **skew_kwargs)
        wish.save_peak_table(run, PEAK_TYPE.PREDICT, INTEGRATION_TYPE.SKEW,
                             save_dir=save_dir, save_format="Jana")
        wish.delete_run_data(run)  # delete raw workspace to save memory

    wish.save_all_peaks(PEAK_TYPE.FOUND, INTEGRATION_TYPE.SKEW, save_dir=save_dir, save_format="jana")

b. Q Integration (with satellites)
==================================

..  code-block:: python

    from mantid.simpleapi import *
    from os import path
    from Diffraction.single_crystal.base_sx import PEAK_TYPE, INTEGRATION_TYPE
    from Diffraction.wish.wishSX import WishSX

    # Setup WishSX
    ##############
    wish = WishSX(vanadium_runno=43526)
    # set sample material
    sphere = '''<sphere id="sphere">
                <centre x="0.0"  y="0.0" z="0.0" />
                <radius val="0.0009"/>
                </sphere>'''  # sphere radius 0.9mm
    wish.set_sample(Geometry={'Shape': 'CSG', 'Value': sphere},
                    Material={'ChemicalFormula': 'Ca3-Ga2-Ge3-O12', 'SampleNumberDensity': 0.001086})

    # load vanadium
    ###############

    wish.process_vanadium()

    # Load runs one at a time and integrate
    #######################################

    save_dir = "/babylon/Public/UserName"
    # integration parameters
    intPeaksMDArgs = {'ellipsoid': True, 'fixQAxis': True, 'fixMajorAxisLength': True, 'useCentroid': True, 'MaskEdgeTubes': False}

    for run in range(42730, 42733):
        wish.process_data([run])
        # load UB
        wish.load_isaw_ub(path.join(save_dir, f'{run}.mat'), tol=0.15, run=run)
        # predict peaks
        # main
        wish.predict_peaks(MinDSpacing=0.75, WavelengthMin=0.85,
                           ReflectionCondition='Body centred', run=run)
        # satellite
        wish.predict_peaks(peak_type=PEAK_TYPE.SATELLITE, run=run,
                           ModVector1="0.5,0,0", ModVector2="0,0.5,0", ModVector3="0,0,0.5",
                           MaxOrder=1, CrossTerms=False, RequirePeaksOnDetector=True,
                           ReflectionCondition='Body centred')
        # filter satellite peaks by wavelength and d-spacing
        sat_peaks = self.get_peaks(run, PEAK_TYPE.PREDICT_SAT)
        FilterPeaks(InputWorkspace=sat_peaks, OutputWorkspace=sat_peaks, FilterVariable="Wavelength",
                    FilterValue=0.85, Operator=">")
        FilterPeaks(InputWorkspace=sat_peaks, OutputWorkspace=sat_peaks, FilterVariable="DSpacing",
                    FilterValue=0.75, Operator=">")
        # Integrate and save
        wish.convert_to_MD(run=run)  # convert to QLab (default)
        for peak_type in [PEAK_TYPE.PREDICT, PREDICT_SAT]:
            wish.integrate_data(INTEGRATION_TYPE.MD_OPTIMAL_RADIUS, peak_type, run=run, **intPeaksMDArgs)
            wish.save_peak_table(run, peak_type, INTEGRATION_TYPE.MD_OPTIMAL_RADIUS,
                                 save_dir=save_dir, save_format="Jana")
        wish.delete_run_data(run, del_MD=False)  # delete raw workspace to save memory

    for peak_type in [PEAK_TYPE.PREDICT, PREDICT_SAT]:
        wish.save_all_peaks(peak_type, INTEGRATION_TYPE.MD_OPTIMAL_RADIUS, save_dir=save_dir, save_format="jana")

.. categories:: Techniques
