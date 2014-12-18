.. algorithm::

.. summary::

.. alias::

.. properties::

Description
-----------

Calculates phonon densities of states, Raman and IR spectrum from the
output of CASTEP code obtained in the form of .phonon and .castep files.

Usage
-----

**Example - loading data from phonon & castep files:**

.. testcode:: ExDensityOfStatesSimple

    #loading the same data from a castep and phonon file
    phonon_ws = DensityOfStates(File='squaricn.phonon')
    castep_ws = DensityOfStates(File='squaricn.castep')

    print CheckWorkspacesMatch(phonon_ws, castep_ws)

Output:

.. testoutput:: ExDensityOfStatesSimple

    Success!

**Example - loading partial contributions of ions:**

.. testcode:: ExDensityOfStatesPartial

    squaricn = DensityOfStates(File='squaricn.phonon', Ions=['H', 'C', 'O'])

    for name in squaricn.getNames():
      print name

Output:

.. testoutput:: ExDensityOfStatesPartial

    squaricn_H
    squaricn_C
    squaricn_O

**Example - loading summed partial contributions of ions:**

.. testcode:: ExDensityOfStatesPartialSummed

    sum_ws = DensityOfStates(File='squaricn.phonon', Ions=['H', 'C', 'O'], SumContributions=True)
    total_ws = DensityOfStates(File='squaricn.phonon')

    print CheckWorkspacesMatch(total_ws, sum_ws, Tolerance=1e-12)

Output:

.. testoutput:: ExDensityOfStatesPartialSummed

    Success!

.. categories::
