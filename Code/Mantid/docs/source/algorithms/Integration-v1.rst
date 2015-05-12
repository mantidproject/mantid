.. algorithm::

.. summary::

.. alias::

.. properties::

Description
-----------

Integration sums up spectra in a :ref:`Workspace <Workspace>` and outputs a
:ref:`Workspace <Workspace>` that contains only 1 value per spectrum (i.e.
the sum). The associated errors are added in quadrature. The two X
values per spectrum are set to the limits of the range over which the
spectrum has been integrated. By default, the entire range is integrated
and all spectra are included.

Optional properties
###################

If only a portion of the workspace should be integrated then the
optional parameters may be used to restrict the range.
StartWorkspaceIndex & EndWorkspaceIndex may be used to select a
contiguous range of spectra in the workspace (note that these parameters
refer to the workspace index value rather than spectrum numbers as taken
from the raw file). If only a certain range of each spectrum should be
summed (which must be the same for all spectra being integrated) then
the RangeLower and RangeUpper properties should be used. No
rebinning takes place as part of this algorithm: if the values given do
not coincide with a bin boundary then the first bin boundary within the
range is used. If a value is given that is beyond the limit covered by
the spectrum then it will be integrated up to its limit. The data that
falls outside any values set will not contribute to the output
workspace.

EventWorkspaces
###############

If an :ref:`EventWorkspace <EventWorkspace>` is used as the input, the
output will be a :ref:`MatrixWorkspace <MatrixWorkspace>`.
:ref:`algm-Rebin` is recommended if you want to keep the workspace as an
EventWorkspace.

**Integraton for event workspaces refers to internal binning, provided by 
:ref:`algm-Rebin` or load algorithm and may ignore limits, provided as algorithm 
input. **  For example, attemtp to integrate loaded ISIS event workspace in the 
range [18000,20000] yields workspace inegrated in the range [0,200000],
assuming the data were collected in the time range [0,20000]. This happens because
the event data would have single hisogram workspace bin in range [0,20000].
To obtain integral in the desired range, user have to :ref:`algm-Rebin` first, 
and one of the binning intervals have to start from 18000 and another (or the same) 
end at 20000.



Usage
-----

.. testcode::

  # Create a workspace filled with a constant value = 1.0
  ws=CreateSampleWorkspace('Histogram','Flat background')
  # Integrate 10 spectra over all X values
  intg=Integration(ws,StartWorkspaceIndex=11,EndWorkspaceIndex=20)

  # Check the result
  print 'The result workspace has',intg.getNumberHistograms(),'spectra'
  print 'Integral of spectrum',11,'is',intg.readY(0)[0]
  print 'Integral of spectrum',12,'is',intg.readY(1)[0]
  print 'Integral of spectrum',13,'is',intg.readY(2)[0]
  print 'Integration range is [',intg.readX(0)[0],',',intg.readX(0)[1],']'

Output
######

.. testoutput::

  The result workspace has 10 spectra
  Integral of spectrum 11 is 100.0
  Integral of spectrum 12 is 100.0
  Integral of spectrum 13 is 100.0
  Integration range is [ 0.0 , 20000.0 ]

.. categories::
