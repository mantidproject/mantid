.. algorithm::

.. summary::

.. relatedalgorithms::

.. properties::

Description
-----------

This performs a one lorentzian convolution fit, and then performs a two lorentzian
convolution fit on the sample workspace.

Usage
-----

**Example - IndirectTwoPeakFit**

.. code-block:: python

    # Produce the reduced file used in the two peak fit
    EnergyWindowScan(InputFiles='92762', Instrument='OSIRIS', Analyser='graphite', Reflection='002',
                     SpectraRange='963,980', ElasticRange='-0.02,0.02', InelasticRange='0.4,0.5',
                     TotalRange='-0.5,0.5', ReducedWorkspace='__reduced_group', ScanWorkspace='__scan_workspace')

    # Perform a two peak fit
    IndirectTwoPeakFit(SampleWorkspace='osiris92762_graphite002_red', EnergyMin=-0.5, 
                       EnergyMax=0.5, OutputName='osiris92762_graphite002_two_peak_fit')

.. categories::

.. sourcelink::
  :cpp: None
  :h: None
