.. algorithm::

.. summary::

.. relatedalgorithms::

.. properties::

Description
-----------
Reduces the data from the Pelican instrument from time of flight versus two theta to S(Q,w). Data can be corrected for background with an empty
can subtraction and normalised to vanadium (both optional). Output can be for a powder S(Q,w) or for a single crystal S(Q,w) in the latter case
an nxspe file is written. Data can be processed for the specified region of Q and w space. Data can also be processed for the lamda/2 option for
Pelican. As a default the output for a powder also includes the Q integrated data S(ω) and the energy integrated data S(Q).

The portion of the detector that is used is defined in the config file. A frame overlap option is also available, where for low temperature data,
a portion of the neutron energy gain spectrum can be used to extend the range of the neutron energy loss side.

Workflow
--------

.. diagram:: PelicanReduction-v1_wkflw.dot

Usage
-----

.. testcode:: PelicanReductionExample

    test = PelicanReduction('44464', EnergyTransfer='-2,0.05,2', MomentumTransfer='0,0.05,2', ConfigurationFile='pelican_doctest.ini')
    print('Workspaces in group = {}'.format(test.getNumberOfEntries()))
    gp = test.getNames()
    print('First workspace: {}'.format(gp[0]))

.. testoutput::  PelicanReductionExample

    Workspaces in group = 3
    First workspace: test_qw1_1D_dE

References
----------

.. sourcelink::

.. categories::
