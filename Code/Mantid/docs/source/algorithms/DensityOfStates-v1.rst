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
    ws1 = DensityOfStates(File='squaricn.phonon', OutputWorkspace='squaricn_phonon')
    ws2 = DensityOfStates(File='squaricn.castep', OutputWorkspace='squaricn_castep')

    print CheckWorkspacesMatch(ws1, ws2)

Output:

.. testoutput:: ExDensityOfStatesSimple
  
    Success!

**Example - loading partial contributions of ions:**  

.. testcode:: ExDensityOfStatesPartial

    ws1 = DensityOfStates(File='squaricn.phonon',OutputWorkspace='squaricn', Ions=['H', 'C', 'O'])
    for name in ws1.getNames():
      print name

Output:

.. testoutput:: ExDensityOfStatesPartial
  
    squaricn_H
    squaricn_C
    squaricn_O

**Example - loading summed partial contributions of ions:**  

.. testcode:: ExDensityOfStatesPartialSummed

    ws1 = DensityOfStates(File='squaricn.phonon',OutputWorkspace='squaricn_partial_sum', Ions=['H', 'C', 'O'], SumContributions=True)
    ws2 = DensityOfStates(File='squaricn.phonon',OutputWorkspace='squaricn_total')

    print CheckWorkspacesMatch(ws1, ws2, Tolerance=1e-12)

Output:

.. testoutput:: ExDensityOfStatesPartialSummed
  
    Success!

.. categories::
