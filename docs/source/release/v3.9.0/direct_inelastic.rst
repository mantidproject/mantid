========================
Direct Inelastic Changes
========================

.. contents:: Table of Contents
   :local:

Improvements
------------

- New algorithm :ref:`CalculateCountRate <algm-CalculateCountRate>` allows to calculate instrument counting rate as function of the experiment 
  time to be able to filter spurions, which may sometimes appear on ISIS instruments. It can also be used to evaluate changes
  of sample reflectivity as function of some slow changing experiment's parameter e.g. temperature, magnetic field or pressure.

- The previously inaccurate instrument definition for IN4 at ILL has been updated. The instrument geometry should now be more correct.

- :ref:`LoadILLTOF <algm-LoadILLTOF>` has been upgraded to version 2. The new version loads the TOF axis as defined by the 'time_of_flight' field in the NeXus file. Consequently, the *FilenameVanadium* and *WorkspaceVanadium* input properties were removed and no 'EPP' entry is added to the sample logs anymore.

- The Debye-Waller factor correction was applied incorrectly to the vanadium data in :ref:`ComputeCalibrationCoefVan <algm-ComputeCalibrationCoefVan>`. This, as well as too pessimistic error evaluation in the algorithm have been fixed.

- A new input property *Temperature* has been added to :ref:`ComputeCalibrationCoefVan <algm-ComputeCalibrationCoefVan>`.

- :ref:`PyChop` has been updated to reflect physical changes to LET's choppers. It will now also not output zero flux Ei reps for LET and Merlin in multi-rep mode. An additional tab is created if the "Instrument Scientist Mode" option is selected which shows the time widths at the detector due to the moderator and choppers for each calculation.

New features
------------

Algorithms
##########

- A utility algorithm :ref:`WorkflowAlgorithmRunner <algm-WorkflowAlgorithmRunner>` has been added to manage the running of certain data reduction workflows at ILL.
- New algorithm :ref:`CorrectTOFAxis <algm-CorrectTOFAxis>` enables the adjustment of the time-of-flight axis according to incident energy or reference workspace.

Crystal Field
-------------

- The peak widths can be fixed to or varied around values obtained from experimental or calculated instrument resolution function.
- The initial field parameters can be estimated using a Monte Carlo search algorithm (:ref:`EstimateFitParameters <algm-EstimateFitParameters>`)
- The crystal field heat capacity, magnetisation and susceptibility can now be calculated or fitted, using new functions
  :ref:`CrystalFieldHeatCapacity <func-CrystalFieldHeatCapacity>`, :ref:`CrystalFieldSusceptibility <func-CrystalFieldSusceptibility>`,
  :ref:`CrystalFieldMagnetisation <func-CrystalFieldMagnetisation>`, and :ref:`CrystalFieldMoment <func-CrystalFieldMoment>`.
  The Python interface :ref:`Crystal Field Python Interface` has also been updated to handle calculating and fitting these quantities.

`Full list of changes on GitHub <http://github.com/mantidproject/mantid/pulls?q=is%3Apr+milestone%3A%22Release+3.9%22+is%3Amerged+label%3A%22Component%3A+Direct+Inelastic%22>`_
