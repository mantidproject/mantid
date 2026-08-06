.. _Engineering_Diffraction_TestGuide-ref:

Engineering Diffraction Testing
=================================

.. contents::
  :local:

Overview
^^^^^^^^

The Engineering Diffraction interface allows scientists using the EnginX and IMAT instruments to
interactively process their data. There are 5 tabs in total:

- ``Run Processing`` - where a cerium oxide run is entered to calibrate the subsequent data, which
  can also be focussed into a single spectrum.
- ``Absorption Correction`` - where the experimental data can be corrected for beam attenuation.
- ``Fitting`` - where peaks can be fitted on focused data.
- ``Texture`` - where pole figures can be generated for experimental runs and their fitted peaks.
- ``GSAS II`` - run a basic refinement on the
  `GSASIIscriptable API <https://gsas-ii.readthedocs.io/en/latest/GSASIIscriptable.html>`_.

**This interface is now covered by an automated UI test suite.** What used to be fourteen manual
tests here is driven by ``QTest`` against the real interface - real view, real presenter, real model,
real algorithms and real files on disk. Only what would block waiting for a user is stood in for: the
generated algorithm dialogs, the error popups, and the external GSAS-II process.

The sections below cover what is automated (and where), and the short list of checks that still have
to be done by hand.

The automated suite
^^^^^^^^^^^^^^^^^^^

The tests live in ``Testing/SystemTests/tests/qt/AutomatedUITest/EngineeringDiffraction`` and are
registered with CTest under the ``qt.AutomatedUITest.EngineeringDiffraction`` prefix (only when the
build has ``ENABLE_WORKBENCH`` on), so the whole suite is::

    ctest -R qt.AutomatedUITest.EngineeringDiffraction

A single module can be run on its own with::

    python Testing/SystemTests/scripts/systestrunner.py <path-to-the-module>.py False

+--------------------------------------+------------------------------------------------------------+
| Module                               | Replaces                                                   |
+======================================+============================================================+
| ``EngDiffGuiRunProcessingTest.py``   | Test 1 (calibration, focusing, settings, plot output,      |
|                                      | reopening the interface) and Test 2 (RB number).           |
+--------------------------------------+------------------------------------------------------------+
| ``EngDiffGuiCroppingTest.py``        | Test 3 (cropping), plus every region-of-interest option    |
|                                      | and the texture grouping save layout.                      |
+--------------------------------------+------------------------------------------------------------+
| ``EngDiffGuiImatTest.py``            | Running the calibration and focus on IMAT, and the         |
|                                      | per-instrument configuration.                              |
+--------------------------------------+------------------------------------------------------------+
| ``EngDiffGuiCorrectionTest.py``      | Test 4 (absorption correction), including the sample       |
|                                      | shape, material, orientation and gauge volume dialogs.     |
+--------------------------------------+------------------------------------------------------------+
| ``EngDiffGuiFittingTest.py``         | Tests 5-11 (focused data, browse filters, run removal,     |
|                                      | background subtraction, fit browser, sequential and        |
|                                      | serial fitting).                                           |
+--------------------------------------+------------------------------------------------------------+
| ``EngDiffGuiTextureTest.py``         | Test 12 (pole figures).                                    |
+--------------------------------------+------------------------------------------------------------+
| ``EngDiffGuiGsas2Test.py``           | Tests 13 and 14 (GSAS-II, single and multiple files).      |
+--------------------------------------+------------------------------------------------------------+

Two conventions are worth knowing before changing them:

- Each class checks a long list of largely independent observations, so a failing assertion is
  *recorded* rather than raised (see ``AutomatedUITestBase.check``). One run therefore reports every
  regression rather than only the first. Preconditions - "the run resolved", "the worker finished" -
  are still plain assertions, so a broken precondition fails fast instead of cascading.
- The ENGIN-X and IMAT run data is **fabricated in-test** rather than loaded from the archive (see
  ``create_synthetic_ceria_and_vanadium``). IMAT has no ceria/vanadium pair in the repository at all,
  and for ENGIN-X the real runs are large and slow while what these tests assert - the interface, the
  reported state and the on-disk save layout - does not depend on the counts being real. The
  numerical correctness of the calibrate/focus chain is covered separately by ``EnginXScriptTest``.

What still has to be tested manually
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

These cannot be automated in the system test suite, and should be checked on an ENGIN-X IDAaaS
instance running ``MantidWorkbenchNightly`` before a release.

GSAS-II against a real installation
-----------------------------------

``EngDiffGuiGsas2Test`` replaces the call into the GSAS-II subprocess with canned output, so it
verifies everything the interface does around GSAS-II but not GSAS-II itself. On an IDAaaS ENGIN-X
instance, with GSAS-II installed in the expected location:

1. On ``Run Processing``, un-tick ``Set Calibration Region of Interest``, enter ``305738`` as the
   ``Calibration Sample #`` and ``307521`` as the ``Vanadium #``, and click ``Calibrate``.
2. Enter ``305761`` as the ``Sample Run #`` and click ``Focus``.
3. On the ``GSAS II`` tab, check the ``Instrument Group`` and ``Focused Data`` paths were pre-filled
   from the calibration and focus. Select ``FE_GAMMA`` as the ``Phase`` and enter a ``Project Name``.
4. Click ``Refine in GSAS II`` and confirm a real refinement runs and is plotted.
5. Repeat with the fitting range narrowed, and with the ``bank_1`` files, and confirm the refined
   parameters are sensible for the data rather than merely present.

Data from the ISIS archive
--------------------------

The automated suite fabricates its run data and never touches the archive, so archive loading is not
exercised. Load a recent ENGIN-X run through ``Run Processing`` and confirm the file finder resolves
it from a run number. Note that run numbers quoted in older documentation may have been deleted.

Visual inspection
-----------------

The suite asserts that figures are created and that the data behind them is right, but not what they
look like. Worth a glance by eye:

- the calibration plot (``Plot Calibrated Workspace``) and the focused output plot,
- the pole figures on the ``Texture`` tab, for the projection, the axes labels and the colour scale,
- the sample shape viewer, for the shape and the orientation of the sample axes,
- the GSAS-II refinement plot.

Adding to the suite
^^^^^^^^^^^^^^^^^^^

New interface behaviour should get a check in the relevant module rather than a step here. The
shared harness is in ``Testing/SystemTests/tests/qt/AutomatedUITest``:

- ``qt_interaction_helpers.py`` - widget-level helpers with no dependency on the system test
  framework, so they can be used from ordinary unit tests too.
- ``automated_ui_test_base.py`` - the cross-interface harness: settings isolation, waiting on
  asynchronous tasks without deadlocking, neutralising modal dialogs, and the soft-assertion
  ``check`` context manager. A suite for another interface should subclass this.
- ``EngineeringDiffraction/eng_diff_gui_test_base.py`` - the Engineering-specific setup and the
  synthetic data fixture.
