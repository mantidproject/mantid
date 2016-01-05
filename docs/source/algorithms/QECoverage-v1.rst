.. algorithm::

.. summary::

.. alias::

.. properties::

Description
-----------
Creates a plot of the momentum transfer-energy transfer (Q-E) trajectories corresponding to the maximum and minimum detector angles for direct geometry spectrometers.

Usage
-----

.. testcode:: QECoverageTest

   # Multiple Ei up to 120meV on Merlin [default instrument]
   QECoverage('18,28,52,120')

   # Overplots with Maps coverage
   QECoverage('120','MAPS',PlotOver=True,PlotOverWindow='QECoverage-1')

.. categories::

.. sourcelink::
