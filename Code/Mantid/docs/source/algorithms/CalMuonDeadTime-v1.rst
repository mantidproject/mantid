.. algorithm::

.. summary::

.. alias::

.. properties::

Description
-----------

Calculate Muon deadtime for each spectra in a workspace.

Define:

| `` ``\ :math:`{\displaystyle{N}}`\ `` = true count``
| `` ``\ :math:`{\displaystyle{N_0}}`\ `` = true count at time zero``
| `` ``\ :math:`{\displaystyle{M}}`\ `` = measured count``
| `` ``\ :math:`{\displaystyle{t_{dead}}}`\ `` = dead-time``
| `` ``\ :math:`{\displaystyle{t_{bin}}}`\ `` = time bin width``
| `` ``\ :math:`{\displaystyle{t_{\mu}}}`\ `` = Muon decay constant``
| `` ``\ :math:`{\displaystyle{F}}`\ `` = Number of good frames``

The formula used to calculate the deadtime for each spectra:

.. math:: M\exp \left( \frac{t}{t_{\mu}} \right)=N_0 - M*N_0*(\frac{t_{dead}}{t_{bin}*F})

where :math:`\displaystyle{M\exp ( t/t_{\mu})}` as a function of
:math:`{\displaystyle{M}}` is a straight line with an intercept of
:math:`{\displaystyle{N_0}}` and a slope of
:math:`{\displaystyle{N_0*(\frac{t_{dead}}{t_{bin}*F})}}`.

Usage
-----

.. include:: ../usagedata-note.txt

**Example - Calculating dead times for a file**  

.. testcode:: CalMuonDeadTime

    ws = Load("MUSR00015189")
    #CalMuonDeadTime outputs two workspaces so catch them both
    (wsOut,wsFitted) = CalMuonDeadTime('ws_1')

    print ("First five dead times:")
    for i in range(5):
        print ("  Spectrum %i -> %.4f" % (wsOut.column(0)[i],wsOut.column(1)[i]))

Output:

.. testoutput:: CalMuonDeadTime

    First five dead times:
      Spectrum 1 -> -0.3164
      Spectrum 2 -> -0.2928
      Spectrum 3 -> -0.2772
      Spectrum 4 -> -0.3181
      Spectrum 5 -> -0.5304

.. categories::

.. sourcelink::
