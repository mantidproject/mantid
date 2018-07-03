.. algorithm::

.. summary::

.. relatedalgorithms::

.. properties::

Description
-----------

Calculate muon deadtime for each spectra in a workspace, by fitting the **DataFitted** to the function detailed below between the times **FirstGoodData** and **LastGoodData**. Using the equation from :ref:`algm-ApplyDeadTimeCorr` and :math:`N(t) = N_0 {\rm exp} (- t / \tau)` results in;

.. math:: M(t)\exp \left( \frac{t}{\tau} \right)=N_0 - M(t)*N_0*\left(\frac{t_{\mathrm{dead}}}{t_{\mathrm{bin}}*F}\right)

where,

| :math:`{\displaystyle{N}(t)}` = true count (unused) as a function of time :math:`t`
| :math:`{\displaystyle{N_0}}` = true count at time zero
| :math:`{\displaystyle{M}(t)}` = measured count
| :math:`{\displaystyle{t_{\mathrm{dead}}}}` = deadtime (fitted)
| :math:`{\displaystyle{t_{\mathrm{bin}}}}` = time bin width
| :math:`{\displaystyle{\tau}}` = Muon lifetime
| :math:`{\displaystyle{F}}` = Number of good frames

Then, :math:`t_{\rm dead}` is found by fitting to the straight line :math:`\displaystyle{M\exp ( t/\tau)}` vs
:math:`{\displaystyle{M}}`, with intercept :math:`{\displaystyle{N_0}}` and slope :math:`{\displaystyle{N_0*\left(\frac{t_{\mathrm{dead}}}{t_{\mathrm{bin}}*F}\right)}}`.

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
