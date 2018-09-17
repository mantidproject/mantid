.. algorithm::

.. summary::

.. relatedalgorithms::

.. properties::

Description
-----------

Takes a workspace as input and sums all of the spectra within it
maintaining the existing bin structure and units. Any masked spectra are
ignored. The result is stored as a new workspace containing a single
spectra.

If we define a the :math:`i^{th}` spectrum with bins :math:`j`. The unweighted sum is just (``WeightedSum=False``)

.. math:: Signal[j] = \displaystyle\Sigma_{i \in spectra} Signal_i[j]

The weighted sum (``WeightedSum=True`` and ``MultiplyBySpectra=True``, ignored for event workspaces), the sum is defined (skipping :math:`Signal_i[j]` when :math:`Error_i[j] == 0`),

.. math:: Signal[j] = NSpectra \times \displaystyle\Sigma_{i \in spectra} \left(\frac{Signal_i[j]}{Error_i^2[j]}\right) / \Sigma_{i \in spectra}\left(\frac{1}{Error_i^2[j]}\right)

:math:`NSpectra` is the number of spectra contributing to that bin. If the weights contributing to the sum are equal, these result in the same value. This should be used for unnormalized (e.g. not divided by vanadium spectrum) data. If the data has been normalized (e.g. divided by vanadium spectrum for total scattering) then multiplying by the number of spectra contributing to the bin is incorrect, use ``WeightedSum=True`` and ``MultiplyBySpectra=False`` to sum as

.. math:: Signal[j] = \displaystyle\Sigma_{i \in spectra} \left(\frac{Signal_i[j]}{Error_i^2[j]}\right) / \Sigma_{i \in spectra}\left(\frac{1}{Error_i^2[j]}\right)

The algorithm adds to the ``OutputWorkspace`` three additional
properties (Log values). The properties (Log) names are:

* ``NumAllSpectra`` is the number of spectra contributed to the sum

* ``NumMaskSpectra`` is the spectra dropped from the summations because they are masked. Monitors are not included in this total if ``IncludeMonitors=False``.

* ``NumZeroSpectra`` is the number of zero bins in histogram workspace or empty spectra for event workspace. These spectra are dropped from the summation of histogram workspace when ``WeightedSum=True``.

Assuming ``pWS`` is the output workspace handle, from Python these
properties can be accessed using:

.. code-block:: python

    nSpectra       = pWS.getRun().getLogData("NumAllSpectra").value
    nMaskedSpectra = pWS.getRun().getLogData("NumMaskSpectra").value
    nZeroSpectra   = pWS.getRun().getLogData("NumZeroSpectra").value

Usage
-----
**Example - a simple example of running SumSpectra.**

.. testcode:: ExSumSpectraSimple

    ws = CreateSampleWorkspace("Histogram", Random=True)
    print("Workspace has %d spectra" % ws.getNumberHistograms())

    ws = SumSpectra(ws)
    print("Workspace has %d spectra" % ws.getNumberHistograms())

Output:

.. testoutput:: ExSumSpectraSimple

    Workspace has 200 spectra
    Workspace has 1 spectra

**Example - running SumSpectra with a list of indicies.**

.. testcode:: ExSumSpectraListOfIndicies

    ws = CreateSampleWorkspace("Histogram", Random=True)
    print("Workspace has %d spectra" % ws.getNumberHistograms())

    ws = SumSpectra(ws, ListOfWorkspaceIndices='0-3, 10-13')
    print("Workspace has %d spectra" % ws.getNumberHistograms())

Output:

.. testoutput:: ExSumSpectraListOfIndicies

    Workspace has 200 spectra
    Workspace has 1 spectra

**Example - a running SumSpectra with a start and end index.**

.. testcode:: ExSumSpectraStartEnd

    ws = CreateSampleWorkspace("Histogram", Random=True)
    print("Workspace has %d spectra" % ws.getNumberHistograms())

    ws = SumSpectra(ws, StartWorkspaceIndex=0, EndWorkspaceIndex=9)
    print("Workspace has %d spectra" % ws.getNumberHistograms())

Output:

.. testoutput:: ExSumSpectraStartEnd

    Workspace has 200 spectra
    Workspace has 1 spectra

**Example - a running SumSpectra in weighted sum mode.**

.. testcode:: ExSumSpectraWeighted

    ws = CreateSampleWorkspace("Histogram", Random=True)
    print("Workspace has %d spectra" % ws.getNumberHistograms())

    ws = SumSpectra(ws, WeightedSum=True)
    print("Workspace has %d spectra" % ws.getNumberHistograms())

Output:

.. testoutput:: ExSumSpectraWeighted

    Workspace has 200 spectra
    Workspace has 1 spectra



.. categories::

.. sourcelink::
