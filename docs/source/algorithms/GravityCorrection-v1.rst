
.. algorithm::

.. summary::

.. relatedalgorithms::

.. properties::

Description
-----------

This algorithm performs a correction of time-of-flight values and their final angles, i.e. angles between the reflected beam and the sample, of neutrons by moving their counts to different detectors to cancel the gravitational effect on a chosen 2DWorkspace (notably the ILL Figaro reflectometer).
An initial computation of the final angle :math:`\theta_f` due to gravitation is required when the neutron flies from the source to the sample.
For the path from the sample to the detector, gravitation plays a role which can be cancelled.
Other properties of the :literal:`InputWorkspace` will be present in the :literal:`OutputWorkspace`.
Both cases, reflection up and down, can be treated.
Please take a look at the gravity correction for ILL reflectometers with the reduction software COSMOS, see [#Gutfreund]_.
Counts of neutrons that do not hit the detector after correction will not be considered in the :literal:`OutputWorkspace`, an information will be logged.
Please note that the output workspace likely has varying bins and consider a subsequent rebinning step (:ref:`algm-Rebin`).
The instrument definition can only contain the position in beam direction and the height of the slits will always be computed internally.
The potential output workspace adds " cancelled gravitation " to its title. This can be seen when visualising the data or reading the sample log information of the :literal::`OutputWorkspace`.
Negative time-of-flight values present in the :literal:`InputWorkspace` are not prohibited.
The input workspace can have a varying number of bins for each spectrum.
However, sizes of output workspace and input workspace match.
In conclusion, this algorithm supports a wide range of special input workspaces, i.e. which can be masked, ragged, and have data errors `Dx`.

Special measurements
--------------------

Direct beam measurements
~~~~~~~~~~~~~~~~~~~~~~~~

Pay attention that the following correction for direct beam measurements does not fulfill the requirements, i.e. no sample.
The algorithm will execute but updated final angles which are smaller than the actual final angle will not be treated.

Reflection down measurements
~~~~~~~~~~~~~~~~~~~~~~~~~~~~

The signed theta angle is considered to take into account reflection up and down measurements.

Support for automatic execution
-------------------------------

If the default slit names for :literal:`FirstSlitName` and :literal:`SecondSlitName` are given, this algorithm attempts to update the slit component names via :ref:`instrument parameter file (IPF) <InstrumentParameterFile>`,
where the parameters :literal:`Workflow.slit1` and :literal:`Workflow.slit2` are searched for the instrument's slit component names.

For example, if the instrument has slit components of names slit2 and slit3, which should be used for this algorithm, the following lines can be added to the parameter file:

.. code-block:: xml
   :linenos:

    <parameter name="Workflow.slit1" type="string">
        <value val="slit2" />
    </parameter>
    <parameter name="Workflow.slit2" type="string">
        <value val="slit3" />
    </parameter>

Currently the case for ILL reflectometers.

Requirements
------------

.. role:: red

- The x-axis of the :literal:`InputWorkspace` must have units :red:`time-of-flight` measured in micro-seconds.
- Those time-of-flight values, :math:`t_{\mbox{tof}}`, are valid for a neutron travel distance from source to detector and do not take gravitation into account.
- The instrument must consist of a :red:`source`, :red:`sample`, :red:`detector` and a collimeter with its :red:`slits` or two other known locations of the neutron flight path between source and sample position.
  Please note that the slit position in beam direction is sufficiant, the horizontal position is zero and the up position will be computed.
- The instrument must be defined in :red:`units metre`.
- The beam direction must be the axis direction of :literal:`X, Y` or :literal:`Z`.
- The algorithm did not already execute for the given :literal:`InputWorkspace`.

Introduction
------------

All following images visualize a single neutron flight and its correction which corresponds to a single count in the workspace.
Please note that the images do not represent a real physical behaviour and serve only to describe the algorithm.
For all following considerations, the sample centre is at position :math:`x_s` = 0 m, :math:`y_s` = 0 m.
The beam direction is along the :math:`x` axis and the up direction along the :math:`y` axis.
The neutron flies in beam direction and the horizontal axis does not play a role.
In a first step, it is necessary to take correct final angles due to gravitation into account.
Then, the time-of-flight axis will be updated accordingly.

Corrected final angles
----------------------

The following image shows schematically gravitational effects for a detector positioned normal to the beam direction.

.. figure:: /images/GravityCorrection1.png
   :align: center

The orange line indicates the assumed neutron flight path which is present in the :literal:`InputWorkspace`.
The following parabola describes the spatial position of the neutron travelling from source to the sample:

.. math:: y = y_0 - k \left( x - x_0 \right)^2

The neutron must travel via the slits :math:`s_{1}` and :math:`s_{2}`:

.. math:: x_0 = \frac{k(x_{s_1}^2 - x_{s_2}^2)+(y_{s_1}-y_{s_2})}{2k (x_{s_1}-x_{s_2})}

.. math:: y_0 = y_{\mbox{s}_1} + k \left( x_{\mbox{s}_1} - x_0 \right)^2,

where the y-coordinate of a slit, depending on the reflection up or down is defined by the initial, uncorrected, incident angle :math:`\theta_{f_{i}}` (orange in the above image), is

.. math:: y_{s_1} = sign( {\theta_{f_{i}}} ) \ x_{s_1} \ tan \left( {\theta_{f_{i}}} \right).

The characteristic inverse length :math:`k` is given by

.. math:: k = \frac{g}{2 v_i^2}.

The velocity :math:`v_{i} = \frac{s_1 + s_2}{t_{\mbox{tof}}}` is the initial neutron velocity, taking the real flight path into account as described in `Parabola arc length`_ and the initial, not updated time-of-flight values.

The detector can have an arbitrary position as shown in the following image.

.. figure:: /images/GravityCorrection3.png
   :align: center

The corrected final angle :math:`\theta_f` shown in this image can be computed by using the gradient of the parabola at sample center position :math:`x_s` = 0 m, :math:`y_s` = 0 m:

.. math:: \boxed{{\theta_f} =  sign( {\theta_{f_{i}}} ) \ atan \left( 2 k \sqrt{|\frac{y_0}{k}|} \right) =  sign( {\theta_{f_{i}}} ) \ atan \left( 2 k x_{0} \right)},

Updating a final angle for a count can be achieved by moving the count to the spectrum of the detector or detector group of the new final angle.

Corrected time-of-flight axis
-----------------------------

The neutron velocity is

.. math:: v_{N} =  \sqrt{ v_{i}^2 \mbox{cos}({\theta_f})^2 + ( v_{i} \mbox{sin}({\theta_f}) - gt )^2 }

and the neutron arrives at detector position

.. math:: x_{d} = v_{N} \mbox{cos}({\theta_f}) t

.. math:: y_{d} = v_{N} \mbox{sin}({\theta_f}) t - \frac{1}{2} g t^2.

The corrected neutron flight path is given by

.. math:: y = x \mbox{tan}({\theta_f}).

When neglecting gravitation :math:`g = 0 \frac{\mbox{m}}{\mbox{s}^2}`, the corrected time-of-flight values are given by

.. math:: \boxed{t = \frac{x_{d}}{s_{2} \mbox{cos}(\theta_f)} \cdot t_{0}},

where :math:`x_{d}` describes the detector :math:`x` position, :math:`s_{2}` is the parabola flight path from sample to detector and :math:`t_{0}` is the initial, uncorrected neutron travel time. Correspondingly, the corrected neutron count will be for detector position

.. math:: x_{d} = v_i \mbox{cos}({\theta_f}) t

.. math:: y_{d} = v_i \mbox{sin}({\theta_f}) t.

Parabola arc length
-------------------

The length :math:`s` of the parabola arc from source to sample

.. math:: s_1 = \int_{x_1}^{0} \sqrt(1 + \left( \frac{\partial y}{\partial x} \right)^2) dx

.. math:: s_1 = \int_{x_1}^{0} \sqrt(1 + \left( - 2 k \left( x - x_{0} \right) \right)

substituting :math:`2 k x = z` results in

.. math:: s_1 = \frac{1}{2k} \int_{2 k x_1}^{0} \sqrt(1 + z^{2}) dz

and with

.. math:: \frac{\partial tan(z)}{\partial z} = 1 + tan^{2}(z) = \frac{1}{cos^{2}(z)}

one can obtain finally

.. math:: s_1 = -\frac{1}{4k} \left((2k x_1) \sqrt(1+(2k x_1)^{2}) + ln | x + \sqrt(1 + (2k x_1)^{2}) | + constant \right).

Equivalently, the solution of the more general form needed for calculating the length from sample to detector

.. math:: s_2 = \int_{0}^{x_2} \sqrt(c + \left( \frac{\partial y_d}{\partial x} \right)^2) dx

is

.. math:: s_2 = \frac{1}{2} \left( (2k x_2) \sqrt(a+(2k x_2)^{2}) + a \ ln | \frac{(2k x_2)}{\sqrt{a}} + \sqrt(1 + \frac{(2k x_2)^{2}}{a}) | + constant \right).

Usage
-----

.. include:: ../usagedata-note.txt

.. testcode::

  # Load an ILL direct reflected Figaro File into a Workspace2D
  ws = LoadILLReflectometry('ILL/Figaro/592724.nxs')

  # Perform correction
  wsCorrected = GravityCorrection(ws)

  # Logarithm of the data
  gc = Logarithm(wsCorrected)

References
----------

.. [#Gutfreund] P. Gutfreund, T. Saerbeck, M. A. Gonzalez, E. Pellegrini, M. Laver, C. Dewhurst, R. Cubitt,
             `arXiv:1710.04139  <https://arxiv.org/abs/1710.04139>`_ **\[physics.ins-det\]**

.. categories::

.. sourcelink::
