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

If we define a the :math:`i^{th}` spectrum with bins :math:`i`. The unweighted sum is just

.. math:: Signal[j] = \displaystyle\Sigma_{i \in spectra} Signal_i[j]

The weighted sum (property ignored for event workspaces), the sum is defined, for :math:`Error_i[j] \neq 0`

.. math:: Signal[j] = NSpectra \times \displaystyle\Sigma_{i \in spectra} \left(\frac{Signal_i[j]}{Error_i^2[j]}\right) / \Sigma_{i \in spectra}\left(\frac{1}{Error_i^2[j]}\right)

If the weights contributing to the sum are equal, these result in the same value.

The algorithm adds to the **OutputWorkspace** three additional
properties (Log values). The properties (Log) names are:
**NumAllSpectra**, **NumMaskSpectra** and **NumZeroSpectra**,
where:

* NumAllSpectra is the number of spectra contributed to the sum

* NumMaskSpectra is the spectra dropped from the summations because they are masked.

  * If monitors are not included in the summation, they are not counted here.

* NumZeroSpectra is the number of zero bins in histogram workspace or empty spectra for event workspace.

  * These spectra are dropped from the summation of histogram workspace when WeightedSum property is set to True.

Assuming **pWS** is the output workspace handle, from Python these
properties can be accessed by the code:

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
