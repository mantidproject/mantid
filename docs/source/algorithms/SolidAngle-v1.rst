.. algorithm::

.. summary::

.. relatedalgorithms::

.. properties::

Description
-----------

The algorithm calculates solid angles from the sample position of the
input workspace for all of the spectra selected. If several detectors
have been mapped to the same spectrum then the solid angles of this
detectors will be summed to provide the solid angle for the spectrum.
The solid angle of a detector that has been masked or marked as dead is
considered to be 0 steradians.

This algorithms can happily accept :ref:`ragged workspace <Ragged_Workspace>`
as an input workspace. The result would
be a ragged output workspace whose X axis values match the lowest and
highest of each the input spectra.

Note: The Solid angle calculation assumes that the path between the
sample and detector is unobstructed by another other instrument
components.


Usage
-----

**Example:**

.. testcode:: solidAngle
    
    ws = CreateSampleWorkspace()
    wsOut = SolidAngle(ws)

    print("Solid angle of Spectra 1 in Bank 1: %.2e" % wsOut.readY(0)[0])
    print("Solid angle of Spectra 101 in Bank 2: %.2e" % wsOut.readY(100)[0])


Output:

.. testoutput:: solidAngle
    :options: +NORMALIZE_WHITESPACE

    Solid angle of Spectra 1 in Bank 1: 6.40e-08
    Solid angle of Spectra 101 in Bank 2: 1.60e-08

.. categories::

.. sourcelink::
