.. _Testing:

=======
Testing
=======

This is a series of tests for the different functionalities of Mantid. The tests are designed to
try to capture real-life use scenarios as closely as possible, whilst minimising the data and
time required for each test. Hopefully if Mantid can pass these tests then it will be usable in
*at least* a basic sense.

When testing, please initially follow the instructions as presented here. After you have given
a run through with the instructions try to play with the functionality that you are testing. For
example, give nonsense values, like strings where floats are expected, or negative values
where they should not be possible. Make sure that Mantid handles all situations gracefully.

When encountering issues during testing, please create issues. The procedure for issue
creation is outlined in :ref:`issue_tracking`.

.. toctree::
   :maxdepth: 1

   Core/Core
   Direct/ALFViewTests
   Direct/DGSReductionTests
   Direct/MSliceTestGuide
   SliceViewer/SliceViewer
   MuonAnalysis_test_guides/index
   ElementalAnalysis/ElementalAnalysisTests
   Indirect/DiffractionTests
   Indirect/DataReductionTests
   Inelastic/CorrectionsTests
   Inelastic/DataProcessorTests
   Inelastic/QENSFittingTests
   Inelastic/BayesFittingTests
   EngineeringDiffraction/EngineeringDiffractionTestGuide
   ErrorReporter-ProjectRecovery/ErrorReporterTesting
   ErrorReporter-ProjectRecovery/ProjectRecoveryTesting
   LiveData/LiveDataTests
   ReflectometryGUI/ReflectometryGUITests
   SANSGUI/ISISSANSGUITests
   General/SampleTransmissionCalculatorTestGuide
   Utility/FilterEventsInterfaceTest
   Documentation/DocumentationTest
