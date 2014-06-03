.. algorithm::

.. summary::

.. alias::

.. properties::

Description
-----------

This algorithm is designed to normalise a workspace via detector
efficiency functions. **For this algorithm to work, the Instrument
Defintion File `IDF <IDF>`__ must have fitting functions on the
component tree**. The setup information for this, as well as some
examples, are provided below.

At a high-level, the algorithm does this:

#. Extract a detector efficiency function :math:`e = f(\lambda)`
#. Using the bin boundaries on the input workspace, calculate efficiency
   Y and E values and generate a new workspace from the results
#. Divide the input workspace by the efficiency workspace

Prerequisites
-------------

The Input Workspace
###################

#. The input workspace must be a MatrixWorkspace
#. The input workspace must have X-units of Wavelength, run
   :ref:`algm-ConvertUnits` on your input workspace if it is not
   already in Wavelength.
#. The input workspace must be a histogram workspace run
   :ref:`algm-ConvertToHistogram` on your input workspace
   if it is not already histogrammed.

The Instrument Definition File
##############################

Background
##########

In brief, the components in the IDF file form a tree structure.
Detectors and Instruments are both types of component. Detectors are
ultimately children of Instruments in the tree structure. For a more
complete description see `IDF <IDF>`__. The tree structure of the
components, mean that fitting functions do not necessarily have to be
assigned on a detector-by-detector basis. Applying a fit function to the
instrument, will ensure that all subcomponents (including detectors),
pick-up that function. However, functions assigned to lower-level
components (such as detectors) take precidence over and exising
functions that might exist on parent components (such as the
instrument). You can even, have some parameters for a function provided
against the detector, and pick up defaults from the bank, or instrument
if they have been specified there.

Recommended Working
###################

The IDF is intended to be a definitive description of the components in
the instrument at any time. This should be the most generic form of the
instrument setup possible. To complement this, you may provide
additional Instrument Parameter files, which can be used to overload
settings in the IDF for purposes of configuration and calibration. **We
strongly recommend that fitting functions are provided via Instrument
Parameter Files rather than directly in the IDF**. This will give you
more flexibility to change your fitting functions without the problems
of synchronising the IDF across Mantid, and possible corruption
resulting from ad-hoc changes.

Instrument Parameter Files that take the form
{InstrumentName}\_Parameters.xml and live in the Instrument directory of
Mantid are automatically loaded along with the IDF when a workspace is
loaded into Mantid. However, you can apply any number of additional
parameter files over the top of an existing workspace using
:ref:`algm-LoadParameterFile`.

Examples
########

Applying a LinearFunction to the whole instrument, hard-coded with A1=2
and A0=1. Fictional instrument is called basic\_rect.

.. code-block:: xml

      <parameter-file instrument = "basic_rect" date = "2012-01-31T00:00:00">
        <component-link name="basic_rect">
        <parameter name="LinearBackground:A0" type="fitting">
          <formula eq="1" result-unit="Wavelength"/>
          <fixed />
        </parameter>
        <parameter name="LinearBackground:A1" type="fitting">
          <formula eq="2" result-unit="Wavelength"/>
          <fixed />
        </parameter>
        </component-link>
      </parameter-file>

Applying the same LinearFunction to two different detectors, with
different coefficients is shown below:

.. code-block:: xml

    <parameter-file instrument = "basic_rect" date = "2012-01-31T00:00:00">
    <component-link name="bank1(0,0)">
       <parameter name="LinearBackground:A0" type="fitting">
           <formula eq="0" result-unit="Wavelength"/>
           <fixed />
       </parameter>
       <parameter name="LinearBackground:A1" type="fitting">
           <formula eq="1" result-unit="Wavelength"/>
           <fixed />
       </parameter>
    </component-link>
    <component-link name="bank2(0,0)">
       <parameter name="LinearBackground:A0" type="fitting">
           <formula eq="1" result-unit="Wavelength"/>
           <fixed />
       </parameter>
       <parameter name="LinearBackground:A1" type="fitting">
           <formula eq="1" result-unit="Wavelength"/>
           <fixed />
       </parameter>
    </component-link>
    </parameter-file>

In the following the LinearFunction A0 coefficient is set globally for
all detectors at the instrument level, while the A1 coefficient is
provided for each detector. In this way the Algorithm sees a complete
definition for the Linear function (both A1 and A0) from two incomplete
definitions on different components in the tree.

.. code-block:: xml

    <parameter-file instrument = "basic_rect" date = "2012-01-31T00:00:00">
    <component-link name="basic_rect">
    <parameter name="LinearBackground:A0" type="fitting">
       <formula eq="3" result-unit="Wavelength"/>
       <fixed />
    </parameter>
    </component-link>
    <component-link name="bank1(0,0)">
    <parameter name="LinearBackground:A1" type="fitting">
       <formula eq="0" result-unit="Wavelength"/>
       <fixed />
    </parameter>
    </component-link>
    <component-link name="bank2(0,0)">
    <parameter name="LinearBackground:A1" type="fitting">
       <formula eq="1" result-unit="Wavelength"/>
       <fixed />
    </parameter>
    </component-link>
    </parameter-file>

.. categories::
