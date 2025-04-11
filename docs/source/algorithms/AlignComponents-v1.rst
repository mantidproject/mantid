.. algorithm::

.. summary::

.. relatedalgorithms::

.. properties::

Description
-----------

This algorithm will take a table of peak-center positions (TOF units) for every pixel,
and minimize the following quantity by moving and rotating instrument components.

.. math:: \sum_i^{N_d}\sum_j^{M_i} \frac{|DIFC_i * TOF_{i,j} - d^{exp}_j|}{d^{exp}_j}

where the sums is for all :math:`N_d` detector pixels in a given instrument component and all
:math:`M_i` peak centers observed at detector pixel :math:`i`. The quantity to be summed is
the absolute value of the fractional difference between the observed peak center in d-spacing units
for detector pixel :math:`i` and peak :math:`j`, :math:`DIFC_i * TOF_{i,j}`, and a reference
experimental value for the d-spacing of peak :math:`j`, :math:`d^{exp}_j`. The conversion from
peak center in TOF units (:math:`TOF_{i,j}`) to d-spacing units is carried out via the
geometrical parameter :math:`DIFC_i` -- see :ref:`Unit Factory` for more information
regarding this parameter. As we change the location and orientations of the instrument components
during the minimization, the geometrical parameter :math:`DIFC_i` is bound to change.

Below's an example of the table of peak-center positions in TOF units for a sample having two peaks
with reference peak centers of 5.1483 and 5.2070 Angstroms for an instrument consisting of one bank
with four pixels:

===== ======= =======
detid @5.1483 @7.2070
===== ======= =======
1     10000.0 nan
2     10010.0 nan
3     nan     6000.0
4     10030.0 6010.0
===== ======= =======

The first pixel contains the 5.1483A peak at :math:`TOF = 10000.0 \mu s` and the 7.2070A peak is not
observed. Similarly for the second pixel. The third pixels observes the second peak but not the first,
and the fourth pixel observes both peaks.

It is required that the reference peak centers in d-spacing have at least a precision of 5 digits.

ComponentList
#############

The *ComponentList* can include any instrument component that can be
moved and rotated by :ref:`algm-MoveInstrumentComponent` and
:ref:`algm-RotateInstrumentComponent`. For example in POWGEN you can
list *bank46* or *Column4* (which includes banks 42-46) or *Group3*
(which all the banks in Column 3 and 4). In the case of a component
group it is treated as one object and not individual banks. In some
instruments you can also specify individual tubes or pixel, *e.g.*
*bank20/tube3* and *bank20/tube3/pixel7*, although that is not the
intention of the algorithm. You can list multiple components which
will be refined in turn (*e.g.* in the *Align the Y rotation of bank26
and bank46 in POWGEN* usage example below).

Masking
#######

The only masking that is on taken into account when minimising the
difference in *DIFC* is the masking in the workspace of the
MaskWorkspace property of AlignComponents.

Fitting Sample/Source
#####################

When fitting the sample or source position it uses the entire
instrument and moves in the directions that you select. All rotation
options are ignored. You can use a masking workspace to mask part of
the instrument you don't want to use to align the sample/source
position (*e.g.* in the *Align sample position in POWGEN* usage
example below).

The source and sample positions (in that order) are aligned before any
components are aligned.

Displacements Table
###################
This table lists changes in position and orientation for each component
other than the source and sample.

- `DeltaR`: change in distance from Component to Sample (in mili-meter)
- `DeltaX`: change in X-coordinate of Component (in mili-meter)
- `DeltaY`: change in Y-coordinate of Component (in mili-meter)
- `DeltaZ`: change in Z-coordinate of Component (in mili-meter)

Changes in Euler Angles are understood once a Euler convention is selected. If
`YXZ` is selected, then:

- `DeltaAlpha`: change in rotation around the Y-axis (in degrees)
- `DeltaBeta`: change in rotation around the X-axis (in degrees)
- `DeltaGamma`: change in rotation around the Z-axis (in degrees)

.. categories::

.. sourcelink::
