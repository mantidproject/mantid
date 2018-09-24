.. algorithm::

.. summary::

.. relatedalgorithms::

.. properties::

Description
-----------

This algorithm fits a series of spectra with the same function. Each
spectrum is fit independently and the result is a table of fitting
parameters unique for each spectrum. The sources for the spectra are
defined in the Input property. The Input property expects a list of
spectra identifiers separated by semicolons (;). An identifier is itself
a comma-separated list of values. The first value is the name of the
source. It can be either a workspace name or a name of a file (RAW or
Nexus). If it is a name of a :ref:`WorkspaceGroup <WorkspaceGroup>` all its
members will be included in the fit. The second value selects a spectrum
within the workspace or file. It is an integer number with a prefix
defining the meaning of the number: "sp" for a spectrum number, "i" for
a workspace index, or "v" for a range of values on the numeric axis
associated with the workspace index. For example, sp12, i125, v0.5:2.3.
If the data source is a file only the spectrum number option is
accepted. The third value of the spectrum identifier is optional period
number. It is used if the input file contains multiperiod data. In case
of workspaces this third parameter is ignored. This are examples of
Input property

| `` "test1,i2; MUSR00015189.nxs,sp3; MUSR00015190.nxs,sp3; MUSR00015191.nxs,sp3"``
| `` "test2,v1.1:3.2"``
| `` "test3,v" - fit all spectra in workspace test3``

Internally PlotPeakByLogValue uses :ref:`algm-Fit` algorithm to perform
fitting and the following properties have the same meaning as in
:ref:`algm-Fit`: Function, StartX, EndX, Minimizer, CostFunction. Property
FitType defines the way of setting initial values. If it is set to
"Sequential" every next fit starts with parameters returned by the
previous fit. If set to "Individual" each fit starts with the same
initial values defined in the Function property.

LogValue property specifies a log value to be included into the output.
If this property is empty the values of axis 1 will be used instead.
Setting this property to "SourceName" makes the first column of the
output table contain the names of the data sources (files or
workspaces).

Output workspace format
#######################

.. figure:: /images/PlotPeakByLogValue_Output.png
   :alt: PlotPeakByLogValue_Output.png

   PlotPeakByLogValue\_Output.png

In this example a group of three Matrix workspaces were fitted with a
:ref:`Gaussian <func-Gaussian>` on a linear background.

The output workspace is a table in which rows correspond to the spectra
in the order they (spectra) appear in the Input property. The first
column of the table has the log values. It is followed by pairs of
columns with parameter values and fitting errors. If a parameter was
fixed or tied the error will be zero. Here is an example of the output
workspace:

Minimizer setup
###############

It is possible to supply a fully configured minimizer via the Minimizer
property, this can also include several flags that are used to indicate the
workspace name and workspace index that has been fitted (useful for fitting using
a minimizer that output workspaces (i.e. FABADA)).

The possible flags are:

:code:`$wsname`
  The name of the workspace being fitted

:code:`$wsindex`
  The index of the spectrum being fitted

:code:`$basename`
  A convenience flag for :code:`$wsname_$wsindex` (the same format the fit
  result and parameter output takes)

:code:`$outputname`
  The output workspace name (i.e. the value of the OutputWorkspace property).

Usage
-----

**Example - fitting a single spectrum of in a workspace:**

.. testcode:: ExPlotPeakByLogValueSimple

    ws = CreateSampleWorkspace()
    function = "name=Gaussian,Height=10.0041,PeakCentre=10098.6,Sigma=48.8581;name=FlatBackground,A0=0.3"
    peaks = PlotPeakByLogValue(ws, function, Spectrum=1)

**Example - sequentially fitting a workspace:**

.. testcode:: ExPlotPeakByLogValueSeq

    import numpy as np

    ws = CreateSampleWorkspace()
    function = "name=Gaussian,Height=10.0041,PeakCentre=10098.6,Sigma=48.8581;name=FlatBackground,A0=0.3"

    #create string of workspaces to fit (ws,i0; ws,i1, ws,i2 ...)
    workspaces = [ws.name() + ',i%d' % i for i in range(ws.getNumberHistograms())]
    workspaces = ';'.join(workspaces)

    peaks = PlotPeakByLogValue(workspaces, function, Spectrum=1)

    #get peak centres for comparison
    peak_centres = peaks.column('f0.PeakCentre')
    ref = np.empty(len(peak_centres))
    ref.fill(10098.6)

    print(np.allclose(ref, peak_centres, 1e-3))

Output:

.. testoutput:: ExPlotPeakByLogValueSeq

    True

.. categories::

.. sourcelink::
