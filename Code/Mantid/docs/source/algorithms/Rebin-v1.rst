.. algorithm::

.. summary::

.. alias::

.. properties::

Description
-----------

The algorithm rebins data with new bin boundaries. The 'params' property
defines new boundaries in intervals :math:`x_i-x_{i+1}\,`. Positive
:math:`\Delta x_i\,` make constant width bins, whilst negative ones
create logarithmic binning using the formula
:math:`x(j+1)=x(j)(1+|\Delta x_i|)\,`

This algorithms is useful both in data reduction, but also in remapping
`ragged workspaces <Ragged Workspace>`__ to a regular set of bin
boundaries.

Unless the FullBinsOnly option is enabled, the bin immediately before
the specified boundaries :math:`x_2`, :math:`x_3`, ... :math:`x_i` is
likely to have a different width from its neighbours because there can
be no gaps between bins. Rebin ensures that any of these space filling
bins cannot be less than 25% or more than 125% of the width that was
specified.

Example Rebin param strings
~~~~~~~~~~~~~~~~~~~~~~~~~~~

-0.0001
    From min(TOF) to max(TOF) among all events in Logarithmic bins of
    0.0001
0,100,20000
    From 0 rebin in constant size bins of 100 up to 20,000
2,-0.035,10
    From 10 rebin in Logarithmic bins of 0.035 up to 10
0,100,10000,200,20000
    From 0 rebin in steps of 100 to 10,000 then steps of 200 to 20,000

For EventWorkspaces
~~~~~~~~~~~~~~~~~~~

If the input is an `EventWorkspace <EventWorkspace>`__ and the "Preserve
Events" property is True, the rebinning is performed in place, and only
the X axes of the workspace are set. The actual Y histogram data will
only be requested as needed, for example, when plotting or displaying
the data.

If "Preserve Events" is false., then the output workspace will be
created as a `Workspace2D <Workspace2D>`__, with fixed histogram bins,
and all Y data will be computed immediately. All event-specific data is
lost at that point.

For Data-Point Workspaces
~~~~~~~~~~~~~~~~~~~~~~~~~

If the input workspace contains data points, rather than histograms,
then Rebin will automatically use the
:ref:`_algm-ConvertToHistogram` and
:ref:`_algm-ConvertToPointData` algorithms before and after
the rebinning has taken place.

FullBinsOnly option
~~~~~~~~~~~~~~~~~~~

If FullBinsOnly option is enabled, each range will only contain bins of
the size equal to the step specified. In other words, the will be no
space filling bins which are bigger or smaller than the other ones.

This, however, means that specified bin boundaries might get amended in
the process of binning. For example, if rebin *Param* string is
specified as "0, 2, 4.5, 3, 11" and FullBinsOnly is enabled, the
following will happen:

-  From 0 rebin in bins of size 2 **up to 4**. 4.5 is ignored, because
   otherwise we would need to create a filling bin of size 0.5.
-  **From 4** rebin in bins of size 3 **up to 10**.

Hence the actual *Param* string used is "0, 2, 4, 3, 10".

.. categories::
