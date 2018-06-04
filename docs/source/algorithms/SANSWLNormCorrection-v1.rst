.. algorithm::

.. summary::

.. relatedalgorithms::

.. properties::

Description
-----------

This algorithm performs a wavelength normalization for SANS TOF instruments.

It uses the input workspaces group of several wavelengths and finds the K and B so

:math:`I_i(Scaled) = K_i * I_i(Original) - B_i`

For every workspace in the workspace group.

The output workspaces have the following suffixes:

- `_q_range_fit`: Workspace group with the range used for fitting. The fitting to find `K` and `B` is performed within this range.
- `_table`: Table with all the found parameters for every input workspace
- `trimmed_fit`: The Raw data with `K` and `B` applied.
- `trimmed_fit_averaged`:  A single `I(q)` with the raw data (with `K` and `B` applied) averaged.

Example of a configuration file
###############################

All properties in this file will have higher priortity. If the same property appears in the algorithm call
and in the file, the file takes precedence.

.. code-block:: ini

    ## The DEFAULT section is mandatory
    [DEFAULT]

    ## I(q, wavelength) non-scaled workspaces.[]
    #InputWorkspaces=

    ## Reference Workspace from the InputWorkspaceGroup. If empty uses the first position from the InputWorkspaceGroup
    #InputWorkspaceReference=

    ## Q ranges for fitting
    #Qmin=0.02
    #Qmax=0.05

    ## Elements to discard when averaging the data
    #DiscardBeginGlobal=10
    #DiscardEndGlobal=10

    ## Initial K and B values. Respect the format below.
    KList=1, 1.1, 1.2, 1.1, 1
    BList=

    ## If you want to set a new prefix for
    #OutputWorkspacePrefix=out

    ## Output directory if OutputDirectory exists in the Property Manager.
    #OutputDirectory=/tmp


.. categories::

.. sourcelink::
