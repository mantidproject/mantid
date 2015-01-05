Indirect Bayes
==============

.. contents:: Table of Contents
  :local:

Overview
--------

.. interface:: Bayes
  :align: right
  :width: 350

ResNorm
-------

.. warning:: This interface is only available on Windows

.. interface:: Bayes
  :widget: ResNorm

This tab creates a group 'normalisation' file by taking a resolution file and
fitting it to all the groups in the resolution (vanadium) data file which has
the same grouping as the sample data of interest.

The routine varies the width of the resolution file to give a 'stretch factor'
and the area provides an intensity normalisation factor.

The fitted parameters are in the group workspace with suffix _ResNorm with
additional suffices of Intensity & Stretch.

The fitted data are in the workspace ending in _ResNorm_Fit.

Options
~~~~~~~

Vanadium File
Resolution File
E Range
Van Binning
Verbose
Plot Result
Save Result

Quasi
-----

.. warning:: This interface is only available on Windows

.. interface:: Bayes
  :widget: Quasi

Options
~~~~~~~

Input
Resolution
Background
Elastic Peak
Sequential Fit
Fix Width
Use ResNorm
E Range
Sample Binning
Resolution Binning
Verbose
Plot Result
Save Result

Stretch
-------

.. warning:: This interface is only available on Windows

.. interface:: Bayes
  :widget: Stretch

This is a variation of the stretched exponential option of Quasi. For each
spectrum a fit is performed for a grid of β and σ values. The distribution of
goodness of fit values is plotted.

Options
~~~~~~~

Sample
Resolution
Background
Elastic Peak
Sequential Fit
E Range
Sample Binning
Sigma
Beta
Verbose
Plot Result
Save Result

JumpFit
-------

.. interface:: Bayes
  :widget: JumpFit

Options
~~~~~~~

Sample
Fit Funcion
Width
Q Range
Verbose
Plot Result
Save Result

.. categories:: Interfaces Indirect
