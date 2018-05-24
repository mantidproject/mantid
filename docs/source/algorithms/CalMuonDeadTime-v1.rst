.. algorithm::

.. summary::

.. relatedalgorithms::

.. properties::

Description
-----------

Calculate Muon deadtime for each spectra in a workspace.

Define:

| `` ``\ :math:`{\displaystyle{N}}`\ `` = true count``
| `` ``\ :math:`{\displaystyle{N_0}}`\ `` = true count at time zero``
| `` ``\ :math:`{\displaystyle{M}}`\ `` = measured count``
| `` ``\ :math:`{\displaystyle{t_{\mathrm{dead}}}}`\ `` = dead-time``
| `` ``\ :math:`{\displaystyle{t_{\mathrm{bin}}}}`\ `` = time bin width``
| `` ``\ :math:`{\displaystyle{t_{\mu}}}`\ `` = Muon decay constant``
| `` ``\ :math:`{\displaystyle{F}}`\ `` = Number of good frames``

The formula used to calculate the deadtime for each spectra:

.. math:: M\exp \left( \frac{t}{t_{\mu}} \right)=N_0 - M*N_0*(\frac{t_{\mathrm{dead}}}{t_{\mathrm{bin}}*F})

where :math:`\displaystyle{M\exp ( t/t_{\mu})}` as a function of
:math:`{\displaystyle{M}}` is a straight line with an intercept of
:math:`{\displaystyle{N_0}}` and a slope of
:math:`{\displaystyle{N_0*(\frac{t_{\mathrm{dead}}}{t_{\mathrm{bin}}*F})}}`.

The number of good frames is obtained from the sample log ``goodfrm`` in the input workspace.
This log must be present for the algorithm to run successfully.

Usage
-----

.. include:: ../usagedata-note.txt

**Example - Calculating dead times for a file**  

.. testcode:: CalMuonDeadTime

    ws = Load("MUSR00015189")
    #CalMuonDeadTime outputs two workspaces so catch them both
    (wsOut,wsFitted) = CalMuonDeadTime('ws_1')

    print("First five dead times:")
    for i in range(5):
        print("  Spectrum {} -> {:.4f}".format(wsOut.column(0)[i], wsOut.column(1)[i]))

Output:

.. testoutput:: CalMuonDeadTime

    First five dead times:
      Spectrum 1 -> -0.3135
      Spectrum 2 -> -0.2902
      Spectrum 3 -> -0.2746
      Spectrum 4 -> -0.3151
      Spectrum 5 -> -0.5266

.. categories::

.. sourcelink::
