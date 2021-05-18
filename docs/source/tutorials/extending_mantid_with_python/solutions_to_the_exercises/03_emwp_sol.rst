.. _03_emwp_sol:

====================
Exercise 3 Solutions
====================

The aim of this exercise is to write a small algorithm that wraps a small
script that focuses some powder diffraction data.

.. code-block:: python

    # Write an algorithm called PowerDiffractionReduce
    #  - The algorithm should have 3 properties:
    #  - Filename: A FileProperty for a TOF data file to load (ignore extensions)
    #  - CalFilename: A FileProperty for a cal file (ignore extensions)
    #  - OutputWorkspace: An output WorkspaceProperty to hold the final result.


    # The steps the algorithm should perform are
    #  - Use the Load algorithm to load the TOF data
    #  - Apply the calibration file using ApplyDiffCalc
    #  - Run ConvertUnits on the TOF data to convert to dSpacing
    #  - Run DiffractionFocusing on the previous output & focus the data using the same cal file from the earlier step
    #   (called a grouping file here)
    #  - Set the output from the DiffractionFocussing algorithm as the output of PowerDiffractionReduce.
    #  - Delete the temporary reference using DeleteWorkspace


    # To test the algorithm, execute the script that contains the algorithm to register it with Mantid. It will then
    # show up in the list of algorithms. Use the following inputs:
    #  - Filename: HRP39182.RAW
    #  - CalFilename: hrpd_new_072_01_corr.cal
    #  - OutputWorkspace: focussed

    from mantid.kernel import *
    from mantid.api import *

    # Class definition
    class DiffractionPowderReduce(PythonAlgorithm):

        def category(self):
            return "Examples"

        def PyInit(self):
            # 2 input properties
            self.declareProperty(FileProperty(name="Filename", defaultValue="", action=FileAction.Load),
                                 doc="TOF data filename")
            self.declareProperty(FileProperty(name="CalFilename", defaultValue="", action=FileAction.Load),
                                 doc="TOF data filename")

            # 1 Output property
            self.declareProperty(WorkspaceProperty(name="OutputWorkspace", defaultValue="",
                                                   direction=Direction.Output))

        def PyExec(self):
            from mantid.simpleapi import Load, ApplyDiffCal, ConvertUnits, DiffractionFocussing, DeleteWorkspace

            # Load file to workspace
            _tmpws = Load(Filename=self.getPropertyValue("Filename"))

            # Apply Calibration and Convert Units
            calfile = self.getProperty("CalFilename").value
            ApplyDiffCal(InstrumentWorkspace=_tmpws, CalibrationFile=calfile)
            _tmpws = ConvertUnits(InputWorkspace=_tmpws, Target="dSpacing")

            # Focus
            _tmpws = DiffractionFocussing(InputWorkspace=_tmpws, GroupingFileName=calfile)

            # Store reference after algorithm has gone
            self.setProperty("OutputWorkspace", _tmpws)

            DeleteWorkspace(_tmpws)

    # Register algorithm with Mantid
    AlgorithmFactory.subscribe(DiffractionPowderReduce)
