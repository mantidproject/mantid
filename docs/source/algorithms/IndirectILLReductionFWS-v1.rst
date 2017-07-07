.. algorithm::

.. summary::

.. alias::

.. properties::

Description
-----------

This algorithm performs Fixed Window Scan (FWS) data reduction (both Elastic and Inelastic) for IN16B indirect geometry instrument at ILL.
It uses internally the :ref:`IndirectILLEnergyTransfer <algm-IndirectILLEnergyTransfer>` algorithm.


Input
-----

Multiple files following the syntax given in
:py:obj:`MultipleFileProperty <mantid.api.MultipleFileProperty>`.

Output
------
A :ref:`WorkspaceGroup <WorkspaceGroup>` that contains as many workspaces as many distinct Doppler's energy values were present in input files list (including E=0 for EFWS).
Each Workspace in the group will have the given observable as the x-axis and as many bins, as many files were given corresponding to the same energy.
Y-axis will be detector angle, and the values would be the intensities integrated over the whole spectra (for EFWS) or over the two peaks
(symmetric around each peak) at the beginning and the end of the spectra (for IFWS).
Scanning observable can be any numeric or time-stamp-like string parameter.
See ``sample.*`` or e.g. ``start_time`` in Sample Logs.
``BackgroundRun`` s, ``CalibrationBackgroundRun`` s and ``CalibrationRun`` s will be averaged (default) or interpolated over all the observable points according to the option.
Interpolation is provided by :ref:`SplineInterpolation <algm-SplineInterpolation>`, which does cubic spline (or linear for 2 points only) interpolation inside the range and
flat extrapolation beyond the range. For the moment it does not give errors on interpolated results.
Interpolated subtraction or calibration is recommended only if there is a strong dependence on the observable and many measured points in the data to be interpolated.

Workflow
--------

.. diagram:: IndirectILLReductionFWS-v1_wkflw.dot

Usage
-----

**Example: EFWS+IFWS**

.. testsetup:: ExFixedWindowScans

   config['default.facility'] = 'ILL'
   config['default.instrument'] = 'IN16B'

.. testcode:: ExFixedWindowScans

    ws = IndirectILLReductionFWS(Run='ILL/IN16B/083072:083077.nxs')
    print "Result is now a WorkspaceGroup, which has %d workspaces, one per each energy value" % ws.getNumberOfEntries()
    print "first item, called %s corresponds to energy value of %s" % \
    (ws.getItem(0).getName(),ws.getItem(0).getName().split('_')[1])
    print "it has %d histograms and %d bins, one per each temperature" % \
    (ws.getItem(0).getNumberHistograms(),ws.getItem(0).blocksize())

Output:

.. testoutput:: ExFixedWindowScans

    Result is now a WorkspaceGroup, which has 3 workspaces, one per each energy value
    first item, called ws_0.0_red corresponds to energy value of 0.0
    it has 18 histograms and 2 bins, one per each temperature

.. testcleanup:: ExFixedWindowScans

   DeleteWorkspace('ws_red')

.. categories::

.. sourcelink::
