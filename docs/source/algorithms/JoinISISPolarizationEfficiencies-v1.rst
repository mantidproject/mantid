.. algorithm::

.. summary::

.. relatedalgorithms::

.. properties::

Description
-----------

The inputs to this algorithm are single-spectra workspaces containing polarization efficiencies. They are combined and interpolated if
neccessary to form a valid matrix workspace. The spectra of the output workspace are labeled with the names of the corresponding
input properties.


Usage
-----

.. testcode:: Example
    
    # Create input workspaces which can have different sizes
    ws1 = CreateWorkspace([1, 2, 3], [1, 1])
    ws2 = CreateWorkspace([2, 3, 4, 5], [1, 1, 1])

    # Combine them in a single workspace
    efficiencies = JoinISISPolarizationEfficiencies(Pp=ws1, Ap=ws2)
    print('Number of spectra = {}'.format(efficiencies.getNumberHistograms()))
    print('Number of bins    = {}'.format(efficiencies.blocksize()))
    print('Label of first  spectrum: {}'.format(efficiencies.getAxis(1).label(0)))
    print('Label of second spectrum: {}'.format(efficiencies.getAxis(1).label(1)))

Output:

.. testoutput:: Example 

    Number of spectra = 2
    Number of bins    = 3
    Label of first  spectrum: Pp
    Label of second spectrum: Ap


.. categories::

.. sourcelink::
