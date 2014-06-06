.. algorithm::

.. summary::

.. alias::

.. properties::

Description
-----------

Apply deadtime correction to each spectra of a workspace. Define:

| `` ``\ :math:`{\displaystyle{N}}`\ `` = true count``
| `` ``\ :math:`{\displaystyle{M}}`\ `` = measured count``
| `` ``\ :math:`{\displaystyle{t_{dead}}}`\ `` = dead-time``
| `` ``\ :math:`{\displaystyle{t_{bin}}}`\ `` = time bin width``
| `` ``\ :math:`{\displaystyle{F}}`\ `` = Number of good frames``

Then this algorithm assumes that the InputWorkspace contains measured
counts as a function of TOF and returns a workspace containing true
counts as a function of the same TOF binning according to

.. math:: N = \frac{M}{(1-M*(\frac{t_{dead}}{t_{bin}*F}))}

.. categories::
