.. algorithm::

.. summary::

.. alias::

.. properties::

Description
-----------

Loads data of a Nexus file obtained from an ILL reflectometry instrument `D17 <https://www.ill.eu/instruments-support/instruments-groups/instruments/d17/description/instrument-layout/>`_ or `Figaro <https://www.ill.eu/instruments-support/instruments-groups/instruments/figaro/description/instrument-layout/>`_ into a `Workspace2D <http://www.mantidproject.org/Workspace2D>`_. Both time-of-flight and monochromatic instrument configurations are supported. In general, this loader reads detector and monitor counts and adds x-axis and error values. The output workspace contains histogram data. The x-axis can have units in time-of-flight or wavelength with non-varying and varying bins, respectively. The conversion to wavelength uses the algorithm :ref:`algm-ConvertUnits`.
The sample logs associated to the output workspace contain two additional entries, :literal:`Facility` and :literal:`stheta` (unit radian). While :literal:`Facility` is the ILL, the variable :literal:`stheta` is the computed Bragg angle and can serve directly as input for the algorithm :ref:`algm-ConvertToReflectometryQ` if desired.

Please note that input properties of :literal:`LoadILLReflectometry` are enabled and disabled where necessary only when calling this algorithm via the graphical user interface. This algorithm can be called via the generic load algorithm :ref:`algm-Load`.

Time of flight axis
-------------------

The chopper values are used for computing the time-of-flight axis :math:`x` according to the following equation:

.. math:: x_{i} = \left( i + 0.5 \right) w_{\mathrm{channel}} + \Delta_{t, \mathrm{tof}} -  60^{\circ} \cdot \frac{ p_{\mathrm{off}} - 45^{\circ} + \Omega_{c2} - \Omega_{c1} + \Delta_{\mathrm{open}} }{ 2 \cdot 360^{\circ} \cdot v_{c1} } \cdot 10^{6},

with the following variables: channel width :math:`w_{\mathrm{channel}}`, time-of-flight delay :math:`\Delta_{t, \mathrm{tof}}`, offset :math:`p_{\mathrm{off}}`, phase of second chopper :math:`\Omega_{c2}`, phase of first chopper :math:`\Omega_{c1}`, open offset :math:`\Delta_{\mathrm{open}}`, velocity of first chopper :math:`v_{c1}`. 

Direct beam measurement
-----------------------

A direct beam reference can be used when calculating the Bragg angle. In this case, the direct beam file should be loaded separately with this algorithm and the :literal:`OutputBeamPosition` output property used to obtain a special `Table Workspaces`_ containing information on the direct beam position. This workspace can be further given as the :literal:`BeamPosition` input to proceeding loads:

.. code-block:: python

   LoadILLReflectometry('directbeam.nxs', 'direct_beam_ws', 'beam_position_ws')
   LoadILLReflectometry('sample1.nxs', 'sample1_ws', BeamPosition='beam_position_ws')
   LoadILLReflectometry('sample2.nxs', 'sample2_ws', BeamPosition='beam_position_ws')
   # ...

An initial offset :math:`\Delta_{o}` of the direct beam will be considered when determining the detector position:

.. math:: \Delta_{o} = \phi_{\mathrm{detector}, d} - 2 \cdot \phi_{\mathrm{sample}, d},

where :math:`\phi_{\mathrm{detector}, d}`, and :math:`\phi_{\mathrm{sample}, d}` are the detector and sample angle, respectively.
With :literal:`ScatteringType` is :literal:`coherent`, an estimation of the peak position of the beam is required. This will be realised by curve fitting using a Gaussian peak as initial guess via the algorithm :ref:`algm-Fit`.

Detector position
-----------------

This loader will update the detector position from what is defined in the instrument definition files (see `D17_definition.xml <https://github.com/mantidproject/mantid/blob/master/instrument/D17_Definition.xml>`_ or `Figaro_definition.xml <https://github.com/mantidproject/mantid/blob/master/instrument/Figaro_Definition.xml>`_ for more information).
First, the detector will be moved to the current sample-detector distance. Then, the detector will be rotated around the :math:`y` axis depending on the scattering angle :math:`\theta`, which is the Bragg angle multiplied by the factor two. Finally, an initial offset :math:`\Delta_{o}` of the direct beam (only when :literal:`BeamPosition` is the given) will be added to the scattering angle:

.. math:: \theta = \theta + \Delta_{o}.

A user defined rotation of the detector can be realised via the option :literal:`BraggAngle`. Then, the detector will be rotated by :literal:`BraggAngle` disregarding any other options.

Bragg angle computation
-----------------------

The following equation is implemented in order to calculate the Bragg angle :math:`\rho` (unit radian) to take into account :literal:`incoherent` and :literal:`coherent` scattering:

.. math:: \rho = \gamma - s \cdot \frac{ \arctan ( \alpha ) + s \cdot  \arctan ( \beta ) }{2},

where :math:`\gamma, s, \alpha, \beta`, are placeholders for an angle, a signed factor, an angle depending on the direct or reflected beam, an angle depending on the reflected beam, respectively. The angles can be calculated via:

.. math:: \alpha, \beta = \left( \omega - c_{\mathrm{pixel}} \right)  \frac{w_{\mathrm{pixel}}}{d_{\mathrm{detector}}},

depending on a detector peak position :math:`\omega`, the centre pixel :math:`c_{\mathrm{pixel}}`, the pixel width :math:`w_{\mathrm{pixel}}` and the detector distance :math:`d_{\mathrm{detector}}`.

The following table explains the different options for :math:`\gamma, s, \alpha, \beta`.

For the input option :literal:`BeamPosition`, the centre angle :math:`\phi_{c}` is defined as 

.. math:: \phi_{c} = \pm \frac{\phi_{\mathrm{detector}, r} - \phi_{\mathrm{detector}, d}}{2},

where the beginning sign is positive for :literal:`D17`. It depends on the reflection (up (+) or down (-)) for :literal:`Figaro`. Furthermore, :math:`\phi_{\mathrm{detector}, r}` is the input detector angle of the reflected beam and :math:`\phi_{\mathrm{detector}, d}` is the detector angle of the direct beam. Other entries of the table describe the sample angle of the reflected beam :math:`\phi_{\mathrm{sample}, r}`, the fitted detector peak position of the reflected beam (fittedR) and the direct beam (fittedD), the maximum detector peak position of the reflected beam (peakR) and the direct beam (peakD).

The sample-detector distance :math:`d_{\mathrm{detector}}` for each instrument. While :literal:`D17` takes the :literal:`det/value`, we calculate the difference :literal:`DTR/value - DTR/offset_value` for :literal:`Figaro`.

+-----------+-----------------+---------------------------------------------------------------+------------------------------------------------------------------------------+
|           | Scattering type | incoherent                                                    | coherent                                                                     |
+-----------+-----------------+-----------------+----------------+--------------+-------------+----------------------------------+--------------+--------------+-------------+
|           |                 | :math:`\gamma`  |:math:`s`       |:math:`\alpha`|:math:`\beta`| :math:`\gamma`                   |:math:`s`     |:math:`\alpha`|:math:`\beta`| 
+===========+=================+=================+================+==============+=============+==================================+==============+==============+=============+
| D17       | sample angle    | \-              |  \-            | \-           | \-          | :math:`\phi_{\mathrm{sample}, r}`| -1.0         | peakR        | fittedR     |
+-----------+-----------------+-----------------+----------------+--------------+-------------+----------------------------------+--------------+--------------+-------------+
|           | detector angle  | :math:`\phi_{c}`| -1.0           | fittedD      | fittedR     | :math:`\phi_{c}`                 | -1.0         | fittedD      | peakR       |
+-----------+-----------------+-----------------+----------------+--------------+-------------+----------------------------------+--------------+--------------+-------------+
| Figaro    | sample angle    | \-              |  \-            | \-           | \-          | :math:`\phi_{\mathrm{sample}, r}`| +/- 1.0      | peakR        | fittedR     |
+-----------+-----------------+-----------------+----------------+--------------+-------------+----------------------------------+--------------+--------------+-------------+
|           | detector angle  | :math:`\phi_{c}`| +/- 1.0        | fittedD      | fittedR     | :math:`\phi_{c}`                 | +/- 1.0      | fittedD      | peakR       |
+-----------+-----------------+-----------------+----------------+--------------+-------------+----------------------------------+--------------+--------------+-------------+

The pre-procedure before fitting the detector peak position can be described by the following example Python code:

::

  Load(Filename='ILLD17-161876-Ni.nxs', OutputWorkspace='ILLD17-161876-Ni', XUnit='TimeOfFlight')
  ExtractMonitors(InputWorkspace='ILLD17-161876-Ni', DetectorWorkspace='Detectors', MonitorWorkspace='Monitors')
  Integration(InputWorkspace='Detectors', OutputWorkspace='Integrated', StartWorkspaceIndex=2)
  Transpose(InputWorkspace='Integrated', OutputWorkspace='Transposed')

Description of Nexus file and corresponding workspace SampleLog entries
-----------------------------------------------------------------------

The following table summarizes the Nexus file entries partially required by the loader: the choice of the chopper is Chopper or VirtualChopper for :literal:`D17` and two choppers are selected out of four existing choppers for :literal:`Figaro`.
A new :literal:`SampleLog` entry for the incident energy :literal:`Ei` with unit meV will be created for :literal:`D17`.

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
| wavelength (D17)  |                                       |                             | **new** SampleLog entry incident energy Ei in meV  | Å           |
+-------------------+---------------------------------------+-----------------------------+----------------------------------------------------+-------------+
| theta (Figaro)    |                                       |                             | sign determines reflection up/down                 | degree      |
+-------------------+---------------------------------------+-----------------------------+----------------------------------------------------+-------------+


Usage
-----

.. include:: ../usagedata-note.txt

**Example - Load ILL D17 Nexus file:**

.. testcode:: LoadILLReflectometry

   import numpy

   # Optional settings
   config['default.facility'] = 'ILL'
   config['default.instrument'] = 'D17'
   
   # Search for data file in UnitTest directory
   config.appendDataSearchDir('../UnitTest')

   # Load ILL D17 data file (TOF mode) into a workspace 2D using default input options:
   ws1 = Load('ILLD17-161876-Ni.nxs')

   print("Workspace {} has {} dimensions and {} histograms.").format(ws1.name(), ws1.getNumDims(), ws1.getNumberHistograms())

   # Load ILL d17 data file (TOF mode) into a workspace 2D using a user-defined angle of 30 degrees:
   ws2 = Load('ILLD17-161876-Ni.nxs', BraggAngle = 30.0)

   # The Sample Log entry stheta will be the user defined angle of 30 degrees:
   angleBragg = ws2.getRun().getProperty("stheta").value * 180. / numpy.pi

   print("The detector of workspace {} was rotated by {} degrees.").format(ws2.name(), 2. * angleBragg)

   instrument = ws2.getInstrument()
   samplePos = ws2.getInstrument().getSample().getPos()
   sourcePos = ws2.getInstrument().getSource().getPos()

   # The scattering angle 
   scatteringAngleLeft = ws2.getDetector(256/2+1).getTwoTheta(samplePos, (samplePos - sourcePos))*180.0/numpy.pi
   scatteringAngleRight = ws2.getDetector(256/2+2).getTwoTheta(samplePos, (samplePos - sourcePos))*180.0/numpy.pi

   print("The scattering angles are {} and {} for detectors {} and {}, respectively.").format(scatteringAngleLeft, scatteringAngleRight, 174, 175)

Output:

.. testoutput:: LoadILLReflectometry	
 
   Workspace ws1 has 2 dimensions and 258 histograms.
   The detector of workspace ws2 was rotated by 60.0 degrees.
   The scattering angles are 59.988956665 and 60.011043335 for detectors 174 and 175, respectively. 

.. testcleanup:: LoadILLReflectometry
    
   DeleteWorkspace(ws1)
   DeleteWorkspace(ws2)

.. categories::

.. sourcelink::
