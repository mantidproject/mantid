.. algorithm::

.. summary::

.. alias::

.. properties::

Description
-----------

Integration sums up spectra in a `Workspace <Workspace>`__ and outputs a
`Workspace <Workspace>`__ that contains only 1 value per spectrum (i.e.
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
the Range\_lower and Range\_upper properties should be used. No
rebinning takes place as part of this algorithm: if the values given do
not coincide with a bin boundary then the first bin boundary within the
range is used. If a value is given that is beyond the limit covered by
the spectrum then it will be integrated up to its limit. The data that
falls outside any values set will not contribute to the output
workspace.

EventWorkspaces
###############

If an `EventWorkspace <EventWorkspace>`__ is used as the input, the
output will be a `MatrixWorkspace <MatrixWorkspace>`__.
:ref:`algm-Rebin` is recommended if you want to keep the workspace as an
EventWorkspace.

.. categories::
