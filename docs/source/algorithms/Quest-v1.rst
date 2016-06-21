.. algorithm::

.. summary::

.. alias::

.. properties::

Description
-----------

This is a variation of the stretched exponential option of
`Quasi <http://www.mantidproject.org/IndirectBayes:Quasi>`__. For each spectrum a fit is performed
for a grid of :math:`\beta` and :math:`\sigma` values. The distribution of goodness of fit values
is plotted.

This routine was originally part of the MODES package. Note that this algorithm
uses F2Py and is currently only supported on windows.

Usage
-----
**Example - a basic example using Quest to fit a reduced workspace.**

.. code-block:: python

   sam = LoadNexusProcessed("irs26173_graphite002_red.nxs", OutputWorkspace='irs26173_graphite002_red')
   res = LoadNexusProcessed("irs26173_graphite002_res.nxs", OutputWorkspace='irs26173_graphite002_res')
   van = LoadNexusProcessed("irs26176_graphite002_red.nxs", OutputWorkspace='irs26176_graphite002_red')

   ResNorm(VanNumber='26176', ResNumber='26173', InputType='File', ResInputType='File', Instrument='irs', Analyser='graphite002', Plot='None', Version=1)
   Quest(SamNumber='26176', ResNumber='26173', ResNormNumber='26176', InputType='File', ResInputType='File', ResNormInputType='Workspace', Instrument='irs', Analyser='graphite002')

.. categories::

.. sourcelink::
