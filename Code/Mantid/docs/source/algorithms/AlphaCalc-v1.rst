.. algorithm::

.. summary::

.. alias::

.. properties::

Description
-----------

Returns the relative efficiency of the forward detector group compared
to the backward detector group. If ``Alpha`` is larger than 1 more counts
has been collected in the forward group.

This algorithm leave the input workspace unchanged. To group detectors
in a workspace use :ref:`algm-MuonGroupDetectors`.

Usage
-----

.. include:: ../usagedata-note.txt

**Example - Calculating Alpha of a MUSR run:**

.. testcode:: ExMUSRAlpha

   loaded = LoadMuonNexus('MUSR0015189.nxs', AutoGroup=True)

   first_period = loaded[0].getItem(0)

   alpha = AlphaCalc(first_period,
                     FirstGoodValue=0.55,
                     LastGoodValue=12.0)

   print 'Alpha value of the first period: {:.3f}'.format(alpha)

Output:

.. testoutput:: ExMUSRAlpha

   Alpha value of the first period: 1.339

.. categories::
