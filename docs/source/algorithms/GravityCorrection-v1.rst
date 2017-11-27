
.. algorithm::

.. summary::

.. alias::

.. properties::

Description
-----------

This algorithm performs a modification of time-of-flight values and their final angles, i.e. angles between the reflected beam and the sample, of neutrons by moving their counts to different detectors to cancel the gravitational effect on a chosen 2DWorkspace (notably the ILL Figaro reflectometer).
An initial computation of the final angle :math:`\theta_f` due to gravitation is required when the neutron flies from the source to the sample.
For the path from the sample to the detector, gravitation plays a role which can be cancelled.
Other properties of the :literal:`InputWorkspace` will be present in the :literal:`OutputWorkspace`.
Please take a look at the gravity correction for ILL reflectometers with the reduction software COSMOS here: Gutfreund et. al. Towards generalized data reduction on a time-of-flight neutron reflectometer.
Counts of neutrons that do not hit the detector after correction will not be considered in the :literal:`OutputWorkspace`, an information will be logged.
Finally, a workspace of corrected gravity contains a :literal:`SampleLog` entry called :literal:`GravityCorrected`.

Requirements
------------

.. role:: red

- The x-axis of the :literal:`InputWorkspace` must be in :red:`time-of-flight`.
- Those time-of-flight values, :math:`t_{\mbox{tof}}`, are valid for a neutron travel distance from source to detector and do not take gravitation into account.
- The instrument must consist of a :red:`source`, :red:`sample`, :red:`detector` and a collimeter with its :red:`slits` or two other known locations of the neutron flight path between source and sample position.
- The instrument must be defined in :red:`units metre`.

Corrected time-of-flight axis
-----------------------------

All following images visualize a single neutron flight and its correction.
The instrument will be moved virtually such that the sample centre is at position :math:`x_s` = 0 m, :math:`y_s` = 0 m for all following considerations.
Thus, it corresponds to a single count in the workspace.
The following two images shows schematically gravitational effects for a detector positioned normal to the beam direction.

.. figure:: /images/GravityCorrection1.png
   :align: center

The orange line indicates the assumed neutron flight path which is present in the :literal:`InputWorkspace`.
In a first step, it is necessary to take correct final angles due to gravitation into account.

The y-coordinate of one slit, depending on the reflection up or down defined by the initial incident angle :math:`\theta_{i, 0}`, is

.. math:: y_{s_1} = sign( \theta_{i, 0} ) x_{s_1} tan \left( \theta_{i, 0} \right).

The following parabola describes the spatial position of the neutron travelling from source to the sample:

.. math:: y = y_0 - k \left( x - x_0 \right)^2

The neutron must travel via the slits :math:`s_{1}` and :math:`s_{2}` :

.. math:: x_0 = \frac{k(x_{s_1}^2 - x_{s_2}^2)+(y_{s_1}-y_{s_2})}{2k (x_{s_1}-x_{s_2})}

.. math:: y_0 = y_{\mbox{s}_1} + k \left( x_{\mbox{s}_1} - x_0 \right)^2.

The final angle :math:`\theta_f` can be computed by using the gradient of the parabola at sample center position :math:`x_s` = 0 m, :math:`y_s` = 0 m:

.. math:: \theta_f =  atan \left( 2 k \sqrt{\frac{y_0}{k}} \right) = atan \left( -2 k x_{0} \right),

with :math:`k` being the characteristic inverse length

.. math:: k = \frac{g}{2 v_N^2}.

Then, the neutron flight path can be modified in terms of cancelling effects due to gravitation for a distance between sample and detector.

The neutron velocity is

.. math:: v_{N} =  \sqrt{ v_{s}^2 \mbox{cos}(\theta_f)^2 + ( v_{s} \mbox{sin}(\theta_f) - gt )^2 }

and the neutron arrives at detector position

.. math:: x_{d} = v_{N} \mbox{cos}(\theta_f) t

.. math:: y_{d} = v_{N} \mbox{sin}(\theta_f) t - \frac{1}{2} g t^2,

where :math:`d` and :math:`s` refer to the detector and the sample, respectively.
Neglecting gravitation, i.e. :math:`g_c = 0 \frac{\mbox{m}}{\mbox{s}^2}` gives

.. math:: y_{d, g_{c}} = v_N \mbox{sin}(\theta_f) t

with

.. math:: v_N = \frac{x_{\mbox{detector}} - x_{\mbox{source}}}{t_{\mbox{tof}}}.

A further generalization is required where the detector can have an arbitrary position as shown in the following image.

.. figure:: /images/GravityCorrection3.png
   :align: center

A detector analytical equation can be derived from known detector positions

.. math:: y = y_{a} + m_{a} x.

The corrected neutron flight path is given by

.. math:: y = x \mbox{tan}(\theta_f).

The neutron hits the detector at corrected position

.. math:: x_{d, c} = \frac{y_a}{\mbox{tan}(\theta_f) - m_a}

.. math:: y_{d, c} = x_{d, c} \mbox{tan}(\theta_f).

It hits the detector at time

.. math:: t_{d, i} = \frac{y_{d, i}}{v_N \mbox{sin}(\theta_f)}

at position

.. math:: x_{d, i} = v_N \mbox{cos}(\theta_f) t_{d, i}

.. math:: y_{d, i} = v_N \mbox{sin}(\theta_f) t_{d, i} - \frac{1}{2} g t_{d, i}^2.

All counts from detector position :math:`x_{d, i}, y_{d, i}` will be moved to position :math:`x_{d, c}, y_{d, c}`.
The corresponding time-of-flight values are given by

.. math:: t = \frac{x_{d, c}}{v_N \mbox{cos} (\theta_f)}

Usage
-----

Example - GravityCorrection

.. testcode:: General: workspace with instrument where the x axis is parrallel and in direction to the beam.

        # A workspace with an instrument defined, each pixel has a side length of 4 mm
        ws = CreateSampleWorkspace(WorkspaceType = 'Histogram',
                                   NumBanks = 2,
                                   NumMonitors = 0,
                                   BankPixelWidth = 10,
                                   XUnit = 'TOF',
                                   XMin = 0,
                                   XMax = 20000,
                                   BinWidth = 200,
                                   PixelSpacing = 0.008,
                                   BankDistanceFromSample = 5,
                                   SourceDistanceFromSample = 10)

        # Perform correction due to gravitation effects
        wsCorrected = GravityCorrection(ws, "slit1", "slit2")

Output:

.. testoutput:: General
    :options: +NORMALIZE_WHITESPACE

.. testcode:: ILL Figaro: workspace with instrument where the z axis is parallel and in direction to the beam.

        # Load an ILL Figaro File into a Workspace2D
        #if (!ws.getRun()->hasProperty("GravityCorrected"))
        ws = LoadILLReflectometry('ILL/Figaro/xxxx.nxs')

        # Perform correction due to gravitation effects
        wsCorrected = GravityCorrection(ws)

Output:

.. testoutput:: ILL Figaro
    :options: +NORMALIZE_WHITESPACE

.. categories::

