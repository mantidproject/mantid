.. algorithm::

.. summary::

.. alias::

.. properties::

Description
-----------

Loads data of a Nexus file obtained from an ILL reflectometry instrument `D17 <https://www.ill.eu/instruments-support/instruments-groups/instruments/d17/description/instrument-layout/>`_ or `Figaro <https://www.ill.eu/instruments-support/instruments-groups/instruments/figaro/description/instrument-layout/>`_ into a `Workspace2D <http://www.mantidproject.org/Workspace2D>`_. Both time-of-flight and monochromatic instrument configurations are supported. In general, this loader reads detector and monitor counts and adds x-axis and error values. The output workspace contains histogram data. The x-axis can have units in time-of-flight or wavelength with non-varying and varying bins, respectively. The conversion to wavelength uses the algorithm :ref:`algm-ConvertUnits`.
The sample logs associated to the output workspace contain two additional entries, :literal:`Facility` and :literal:`stheta` (unit radian). While :literal:`Facility` is the ILL, the variable :literal:`stheta` is the computed Bragg angle and can serve directly as input for the algorithm :ref:`algm-ConvertToReflectometryQ` if desired.

Time of flight axis
-------------------

The chopper values are used for computing the time-of-flight values for the bin edges :math:`x_i` by the following equation:

.. math:: x_{i} = \left( i + 0.5 \right) w_{\mathrm{channel}} + \Delta_{t, \mathrm{tof}} -  60^{\circ} \cdot \frac{ p_{\mathrm{off}} - 45^{\circ} + \Omega_{c2} - \Omega_{c1} + \Delta_{\mathrm{open}} }{ 2 \cdot 360^{\circ} \cdot v_{c1} } \cdot 10^{6},

with the following variables: channel width :math:`w_{\mathrm{channel}}`, time-of-flight delay :math:`\Delta_{t, \mathrm{tof}}`, offset :math:`p_{\mathrm{off}}`, phase of second chopper :math:`\Omega_{c2}`, phase of first chopper :math:`\Omega_{c1}`, open offset :math:`\Delta_{\mathrm{open}}` and velocity of first chopper :math:`v_{c1}`. 

Detector position
-----------------

This loader will update the detector position from what is defined in the instrument definition files. The detector will be moved to the current sample-detector distance and rotated around the origin either on the horizontal or vertical plane.

The rotation angle can be one of the following:

* The detector angle in the sample logs. This is the default behavior if :literal:`BraggAngle` and :literal:`BeamPosition` are not given.

* User-specified angle given by the :literal:`BraggAngle` input property. This option will always take precedence over other options.

* A calibrated detector angle. The calibration is done using a direct beam measurement. This option is triggered when the :literal:`BeamPosition` property is specified.

For the direct beam calibration, a direct beam file should be loaded separately and the :literal:`OutputBeamPosition` output property used to obtain a special :ref:`TableWorkspace <Table Workspaces>` containing information on the direct beam position. This workspace can be further given as the :literal:`BeamPosition` input to proceeding loads as exemplified in the following:

.. code-block:: python

   LoadILLReflectometry('directbeam.nxs', OutputWorkspace='direct_beam_ws', OutputBeamPosition='beam_position_ws')
   LoadILLReflectometry('sample1.nxs', OutputWorkspace='sample1_ws', BeamPosition='beam_position_ws')
   LoadILLReflectometry('sample2.nxs', OutputWorkspace='sample2_ws', BeamPosition='beam_position_ws')
   # ...

Direct beam calibration
#######################

The detector position calibration requires peak position fitting for both the direct and reflected beam data. Basically, the data is integrated using :ref:`algm-Integration`, transposed using :ref:`algm-Transpose` and a :ref:`func-Gaussian` is fitted by :ref:`algm-Fit`. The fitted peak position gives the angle between the centre of the detector and the direct or reflected beam:

.. math::
   \Delta = \tan^{-1} \frac{(i_{centre} - i_{fit}) d_{pix}}{l_{2}},

where :math:`i_{centre}` is the workspace index of the detector centre (127.5 for D17 and Figaro), :math:`i_{fit}` the fitted peak position, :math:`d_{pix}` the physical pixel width and :math:`l_{2}` the sample to detector centre distance.

The calibrated detector angle is then given by

.. math::
   \alpha = \alpha_{R} - \alpha_{D} - 2 \Delta_{D} + \Delta_{R},

where :math:`\alpha` denotes the detector angles while the subscript :math:`R` refers to the reflected beam and :math:`D` to the direct beam.

Source position
---------------

In the case of D17, this loader will move the source position ('chopper1' in the instrument definition file) on the z-axis to the position

.. math::
   z_{source} = -d_{ch} + \frac{1}{2} \delta_{ch},

where :math:`d_{ch}` is the :literal:`VirtualChopper.dist_chop_samp` sample log entry and :math:`\delta_{ch}` the :literal:`Distance.ChopperGap` entry.

Description of Nexus file and corresponding workspace SampleLog entries
-----------------------------------------------------------------------

The following table summarizes the Nexus file entries partially required by the loader: the choice of the chopper is Chopper or VirtualChopper for D17 and two choppers are selected out of four existing choppers for Figaro.

+-------------------+---------------------------------------+-----------------------------+----------------------------------------------------+-------------+
| Nexus entry       | D17                                   | Figaro                      | Description                                        | Unit        |
+===================+=======================================+=============================+====================================================+=============+
| acquisition_mode  |                                       |                             | If time of flight mode or not                      | \-          |
+-------------------+---------------------------------------+-----------------------------+----------------------------------------------------+-------------+
| data              | PSD_data                              | PSD_data                    |                                                    | \-          |
+-------------------+---------------------------------------+-----------------------------+----------------------------------------------------+-------------+
| instrument        | Chopper1/phase                        | CH1/phase                   | chopper phase                                      | degree      |
|                   +                                       +                             +                                                    +             +            
|                   | Chopper1/rotation_speed               | CH1/rotation_speed          | chopper speed                                      | rpm         |
|                   +---------------------------------------+-----------------------------+----------------------------------------------------+-------------+
|                   | Chopper2/phase                        | CH2/phase                   |                                                    | degree      |
|                   +                                       +                             +                                                    +             +  
|                   | Chopper2/rotation_speed               | CH2/rotation_speed          |                                                    | rpm         |
|                   +---------------------------------------+-----------------------------+----------------------------------------------------+-------------+
|                   |                                       | CH3/phase                   |                                                    | degree      |
|                   +                                       +                             +                                                    +             +  
|                   |                                       | CH3/rotation_speed          |                                                    | rpm         |
|                   +---------------------------------------+-----------------------------+----------------------------------------------------+-------------+ 
|                   |                                       | CH4/phase                   |                                                    | degree      |
|                   +                                       +                             +                                                    +             +  
|                   |                                       | CH4/rotation_speed          |                                                    | rpm         |
|                   +---------------------------------------+-----------------------------+----------------------------------------------------+-------------+
|                   |                                       | ChopperSetting/firstChopper | Number of selected first chopper                   | \-          |
|                   +                                       +                             +                                                    +             +  
|                   |                                       | ChopperSetting/secondChopper| Number of selected second chopper                  | \-          | 
|                   +---------------------------------------+-----------------------------+----------------------------------------------------+-------------+
|                   | VirtualChopper/chopper1_phase_average |                             |                                                    | degree      |
|                   +                                       +                             +                                                    +             +
|                   | VirtualChopper/chopper1_speed_average |                             |                                                    | rpm         |
|                   +                                       +                             +                                                    +             +  
|                   | VirtualChopper/chopper2_phase_average |                             |                                                    | degree      |
|                   +                                       +                             +                                                    +             +  
|                   | VirtualChopper/chopper2_speed_average |                             |                                                    | rpm         |
|                   +---------------------------------------+-----------------------------+----------------------------------------------------+-------------+
|                   | VirtualChopper/open_offset            | CollAngle/openOffset        |                                                    | degree      |
|                   +                                       +                             +                                                    +             +  
|                   | VirtualChopper/poff                   | CollAngle/poff              |                                                    | degree      |
|                   +---------------------------------------+-----------------------------+----------------------------------------------------+-------------+
|                   | det/offset_value                      | DTR/offset_value            | detector distance offset                           | millimeter  |
|                   +                                       +                             +                                                    +             +  
|                   | det/value                             | DTR/value                   | detector distance value                            | millimeter  |
|                   +---------------------------------------+-----------------------------+----------------------------------------------------+-------------+
|                   | PSD/detsize                           | PSD/detsize                 | detector size                                      | \-          |
|                   +                                       +                             +                                                    +             +  
|                   | PSD/detsum                            | PSD/detsum                  | sum of detector counts                             | \-          |
|                   +---------------------------------------+-----------------------------+----------------------------------------------------+-------------+
|                   | PSD/mmpx                              | PSD/mmpy                    | pixel width                                        | millimeter  |
|                   +---------------------------------------+-----------------------------+----------------------------------------------------+-------------+
|                   | PSD/time_of_flight_0                  | PSD/time_of_flight_0        | channel width                                      | microseconds|
|                   +                                       +                             +                                                    +             +  
|                   | PSD/time_of_flight_1                  | PSD/time_of_flight_1        | number of channels                                 | \-          |
|                   +                                       +                             +                                                    +             +  
|                   | PSD/time_of_flight_2                  | PSD/time_of_flight_2        | time-of-flight delay                               | microseconds|
|                   +---------------------------------------+-----------------------------+----------------------------------------------------+-------------+
|                   | dan/value                             | VirtualAxis/DAN_actual_angle| detector angle                                     | degree      |
|                   +---------------------------------------+-----------------------------+----------------------------------------------------+-------------+ 
|                   | san/value                             | CollAngle/actual_coll_angle | sample angle                                       | degree      |
+-------------------+---------------------------------------+-----------------------------+----------------------------------------------------+-------------+
| monitor1          | data                                  | data                        |                                                    | \-          |
+-------------------+---------------------------------------+-----------------------------+----------------------------------------------------+-------------+
| monitor2          | data                                  | data                        |                                                    | \-          |
+-------------------+---------------------------------------+-----------------------------+----------------------------------------------------+-------------+
| theta (Figaro)    |                                       |                             | sign determines reflection up/down                 | degree      |
+-------------------+---------------------------------------+-----------------------------+----------------------------------------------------+-------------+


Usage
-----

.. include:: ../usagedata-note.txt

**Example - Load ILL D17 Nexus file:**

.. testcode:: LoadDefaultOptions

   # Optional: set facility and default instrument
   config['default.facility'] = 'ILL'
   config['default.instrument'] = 'D17'

   # Load ILL D17 data file (TOF mode) into a workspace 2D using default input options:
   ws1 = Load('ILL/D17/317370.nxs')

   print("Workspace {} has {} dimensions and {} histograms.").format(ws1.name(), ws1.getNumDims(), ws1.getNumberHistograms())

Output:

.. testoutput:: LoadDefaultOptions

   Workspace ws1 has 2 dimensions and 258 histograms.

.. testcleanup:: LoadDefaultOptions

   AnalysisDataService.Instance().clear()

**Example - Specify user angle:**

.. testcode:: LoadBraggAngle

   import numpy

   # Load ILL d17 data file (TOF mode) into a workspace 2D using a user-defined angle of 30 degrees:
   ws2 = Load('ILL/D17/317370.nxs', BraggAngle = 5.5)

   # The original detector angle can be found in the sample logs:
   angleOrig = ws2.getRun().getProperty("dan.value").value

   # The Sample Log entry stheta will be the user defined angle of 30 degrees:
   angleBragg = ws2.getRun().getProperty("stheta").value * 180. / numpy.pi

   print("The detector of workspace {} was rotated to {} degrees.").format(ws2.name(), 2. * angleBragg)
   print("The nominal angle in the NeXus file was {:.2} degrees.".format(angleOrig))

Output:

.. testoutput:: LoadBraggAngle

   The detector of workspace ws2 was rotated to 5.5 degrees.
   The nominal angle in the NeXus file was 3.2 degrees.

.. testcleanup:: LoadBraggAngle

   AnalysisDataService.Instance().clear()

**Example - Calibration of detector angle by direct beam:**

.. testcode:: LoadDirectBeam

   import numpy

   directBeamWS = Load('ILL/D17/317369.nxs', OutputBeamPosition='beamPositionWS')

   beamPosWS = mtd['beamPositionWS']
   peakCentre = beamPosWS.cell('FittedPeakCentre', 0)
   print('Fitted direct beam maximum (in workspace indices): {:.5}'.format(peakCentre))

   reflectedBeamWS = Load('ILL/D17/317370.nxs', BeamPosition=beamPosWS)

   # Lets load the data without detector angle calibration just for reference

   refWS = Load('ILL/D17/317370.nxs')

   detAngle = numpy.degrees(reflectedBeamWS.getRun().getProperty('stheta').value)
   refAngle = numpy.degrees(refWS.getRun().getProperty('stheta').value)

   print('Uncalibrated detector angle: {:.4} degrees.'.format(refAngle))
   print('Detector angle after calibration using direct beam: {:.4} degrees.'.format(detAngle))

Output:

.. testoutput:: LoadDirectBeam

   Fitted direct beam maximum (in workspace indices): 202.18
   Uncalibrated detector angle: 1.591 degrees.
   Detector angle after calibration using direct beam: 1.627 degrees.

.. testcleanup:: LoadDirectBeam

   AnalysisDataService.Instance().clear()

.. categories::

.. sourcelink::
