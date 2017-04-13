.. algorithm::

.. summary::

.. alias::

.. properties::

Description
-----------

Loads data of a Nexus file obtained from an ILL reflectometry instrument `D17 <https://www.ill.eu/instruments-support/instruments-groups/instruments/d17/description/instrument-layout/>`_ or `Figaro <https://www.ill.eu/instruments-support/instruments-groups/instruments/figaro/description/instrument-layout/>`_ into a `Workspace2D <http://www.mantidproject.org/Workspace2D>`_.
In general, this loader reads detector and monitor counts and adds x-axis and error values. The output workspace is a histogram. The x-axis can have units in time-of-flight or wavelength with non-varying and varying bins, respectively. The conversion to wavelength uses the algorithm :ref:`algm-ConvertUnits`.
The SampleLog contains two additional entries, :literal:`Facility` and :literal:`stheta` (unit radiant). While Facility is the ILL, the variable :literal:`stheta` is the computed Bragg angle and can serve directly as input for the algorithm :ref:`algm-ConvertToReflectometryQ` if desired.
The computation of the Bragg angle requires an input file for `DirectBeam`, if :literal:`InputAngle` is the :literal:`detector angle`. These options and the option :literal:`sample angle` with :literal:`ScatteringType` is :literal:`coherent` require an estimation of the peak position of the beam(s). This will be realised by curve fitting using an initial Gaussian peak function via the algorithm :ref:`algm-Fit`.
Please note that input properties are enabled and disabled where necessary only when calling this algorithm via the graphical user interface. This algorithm can be called via the generic load algorithm :ref:`algm-Load`.
The TOF (and monochromatic) instrument configurations are supported.

Detector position
-----------------

For each input file :literal:`Filename`, the detector position will be updated, which was initially defined by the corresponding instrument definition file, see `D17_definition.xml <https://github.com/mantidproject/mantid/blob/master/instrument/D17_Definition.xml>`_ and `Figaro_definition.xml <https://github.com/mantidproject/mantid/blob/master/instrument/Figaro_Definition.xml>`_ for more information.
The detector will be moved to the current sample-detector distance (D17) or xxx(Figaro).
Then, the detector will be rotated around the :math:`y` axis depending to the scattering angle, which is the Bragg angle multiplied by the factor two, calculated based on the user's choice of :literal:`InputAngle` for the options :literal:`sample angle` and :literal:`detector angle` and :literal:`ScatteringType`. The initial offset of the direct beam will be added to the scattering angle:

.. math:: \Delta_{direct} = \psi_{detector} - 2.0 \psi_{sample}, 

where :math:`\Delta_{direct}`, :math:`\psi_{detector}`, and :math:`\psi_{sample}` are the offset of the direct beam, detector angle, and sample angle, respectively.
A user defined rotation of the detector should be realised via the option :literal:`InputAngle` = :literal:`user defined`. Then, the detector will be rotated by :literal:`BraggAngle` disregarding options of the :literal:`ScatteringType` and without modifications due to an offset angle.

Bragg angle computation
-----------------------

The following equation is implemented in order to calculate the Bragg angle :math:`\rho` (unit radiant) to take into account :literal:`incoherent` and :literal:`coherent` scattering:

.. math:: \rho = \gamma - s 0.5 ( atan ( \alpha )) + s ( atan ( \beta ) ),

where :math:`\gamma, s, \alpha, \beta`, are an angle, a signed factor, an angle depending on the direct or reflected beam, an angle depending on the reflected beam, respectively:

.. math:: \alpha = \left( \omega - centre_{pixel} \right)  \frac{width_{pixel}}{distance_{detector}}

The peak position fit can be described by the following Python code:

::

Load(Filename='ILLD17-161876-Ni.nxs', OutputWorkspace='ILLD17-161876-Ni', XUnit='TimeOfFlight')
Integration(InputWorkspace='ILLD17-161876-Ni', OutputWorkspace='Integrated', StartWorkspaceIndex=2)
Transpose(InputWorkspace='Integrated', OutputWorkspace='Transposed')

--------------------------------------------------------------------------------------------------------

The following table explains the different options for :math:`s`, ...
Using the sample angle :math:`\phi_{sample}`, the  

The centre angle is defined as

.. _math:: \phi_{centre} = +/- \phi_detector \frac{\phi_{detector} - \phi_detector\left(direct beam \right)}{2}

fitted peak position reflected beam: fittedR
fitted peak position direct beam: fittedD
maximum peak position reflected beam: peakR
maximum peak position direct beam: peakD
+/- depends on figaro reflection down

+-----------------+-------------------------------------------------------------+-------------------------------------------------------------+
| Scattering type | incoherent                                                  | coherent                                                    |
+-----------------+----------------------+---------+--------------+-------------+----------------------+---------+--------------+-------------+
|                 | :math:`\gamma`       |:math:`s`|:math:`\alpha`|:math:`\beta`| :math:`\gamma`       |:math:`s`|:math:`\alpha`|:math:`\beta`| 
+=================+======================+=========+============================+======================+=========+==============+=============+
| sample angle    | 0.0                  |  0.0    |                            | :math:`\phi_{sample}`| -1.0    | peakR        | fittedR     |
+-----------------+----------------------+---------+--------------+-------------+----------------------+---------+--------------+-------------+
| detector angle  | :math:`\phi_{centre}`| -1.0    | fittedD      | fittedR     | :math:`\phi_{centre}`| -1.0    | fittedD      | peakR       |
+-----------------+----------------------+---------+--------------+-------------+----------------------+---------+--------------+-------------+


Description of Nexus file entries and corresponding workspace SampleLog entries
-------------------------------------------------------------------------------

The following table summarizes the Nexus file entries partially required by the loader.
The chopper values are used for computing the time-of-flight axis according to the following eqaution:
.. math:: \left( i + 0.5 \right) width_{channel} + delay_{tof} - 1.0e^{+6} 60.0 \frac{poff - 45.0 + phase_{chopper2} - phase_{chopper1} + openOffset}{2.0 \cdot 360 \cdot speed_{chopper1}}.
If a D17 Nexus file does not have entries for the VirtualChopper, the Chopper values will be used instead.

+-------------------+---------------------------------------+-----------------------------+----------------------------+-------------+
| Nexus entry       | D17                                   | Figaro                      | Description                | Unit        |
+-------------------+---------------------------------------+-----------------------------+----------------------------+-------------+
| acquisition_mode                                                                                                                   |
+-------------------+---------------------------------------------------------------------+----------------------------+-------------+
| data              | PSD_data                                                            |                            |             |
+-------------------+---------------------------------------+-----------------------------+----------------------------+-------------+
| instrument        | Chopper1/phase                        | CH1/phase                   | chopper phase              |             |
|                   +                                       +                             +                            |             |            
|                   | Chopper1/rotation_speed               | CH1/rotation_speed          | chopper speed              |             |
|                   +---------------------------------------+-----------------------------+----------------------------+-------------+
|                   | Chopper2/phase                        | CH2/phase                   |                            |             |
|                   +                                       +                             +                            |             |  
|                   | Chopper2/rotation_speed               | CH2/rotation_speed          |                            |             |
|                   +---------------------------------------+-----------------------------+----------------------------+-------------+
|                   |                                       | CH3/phase                   |                            |             |
|                   +                                       +                             +                            |             |  
|                   |                                       | CH3/rotation_speed          |                            |             |
|                   +---------------------------------------+-----------------------------+----------------------------+-------------+ 
|                   |                                       | CH4/phase                   |                            |             |
|                   +                                       +                             +                            |             |  
|                   |                                       | CH4/rotation_speed          |                            |             |
|                   +---------------------------------------+-----------------------------+----------------------------+-------------+
|                   |                                       | ChopperSetting/firstChopper |                            |             |
|                   +                                       +                             +                            |             |  
|                   |                                       | ChopperSetting/secondChopper|                            |             | 
|                   +---------------------------------------+-----------------------------+----------------------------+-------------+
|                   | VirtualChopper/chopper1_phase_average |                             |                            |             |
|                   +                                       +                             +                            +             |
|                   | VirtualChopper/chopper1_speed_average |                             |                            |             |
|                   +                                       +                             +                            +             |  
|                   | VirtualChopper/chopper2_phase_average |                             |                            |             |
|                   +                                       +                             +                            +             |  
|                   | VirtualChopper/chopper2_speed_average |                             |                            |             |
|                   +                                       +                             +                            +             |  
|                   | VirtualChopper/dist_chop_samp         |                             | chopper-sample distance    | millimeter  |
|                   +---------------------------------------+-----------------------------+----------------------------+-------------+
|                   | VirtualChopper/open_offset            | CollAngle/openOffset        |                            |             |
|                   +                                       +                             +                            +             |  
|                   | VirtualChopper/poff                   | CollAngle/poff              |                            |             |
|                   +---------------------------------------+-----------------------------+----------------------------+-------------+
|                   |                                       | DTR/offset_value            | detector distance offset   | millimeter  |
|                   +                                       +                             +                            +             |  
|                   | det/value                             | DTR/value                   | detector distance value    | millimeter  |
|                   +---------------------------------------+-----------------------------+----------------------------+-------------+
|                   | PSD/detsize                           |                             | detector size              |             |
|                   +                                       +                             +                            +             |  
|                   | PSD/detsum                            |                             | sum of detector counts     |             |
|                   +---------------------------------------+-----------------------------+----------------------------+-------------+
|                   | PSD/mmpx                              | PSD/mmpy                    | pixel width                | millimeter  |
|                   +---------------------------------------+-----------------------------+----------------------------+-------------+
|                   | PSD/time_of_flight_0                  |                             | channel width              |             |
|                   +                                       +                             +                            +             |  
|                   | PSD/time_of_flight_1                  |                             | number of channels         |             |
|                   +                                       +                             +                            +             |  
|                   | PSD/time_of_flight_2                  |                             | time-of-flight delay       | microseconds|
|                   +---------------------------------------+-----------------------------+----------------------------+-------------+
|                   |                                       | Theta/                      |                            |             |
|                   +---------------------------------------+-----------------------------+----------------------------+-------------+
|                   | dan/value                             | VirtualAxis/DAN_actual_angle| detector angle             | degree      |
|                   +---------------------------------------+-----------------------------+----------------------------+-------------+ 
|                   | san/value                             | SAN/value                   | sample angle               | degree      |
+-------------------+---------------------------------------+-----------------------------+----------------------------+-------------+
| monitor1          | data                                  |                             |                            |             |
+-------------------+---------------------------------------+-----------------------------+----------------------------+-------------+
| monitor2          | data                                  |                             |                            |             |
+-------------------+---------------------------------------+-----------------------------+----------------------------+-------------+
| wavelength (D17)                                                                                                                   |
+------------------------------------------------------------------------------------------------------------------------------------+
| theta (Figaro)                                                                                                                     |
+------------------------------------------------------------------------------------------------------------------------------------+

Example
-------

Usage
-----

**Example - Load ILL D17 NeXus file:**

.. testcode:: LoadILLReflectometry

   # Load ILL D17 data file (TOF mode) into a workspace 2D using default input options:
   ws1 = Load('ILLD17_111686.nxs')

   print("Workspace {} has {} dimensions (spectra) and {} histograms.").format(ws1.name(), ws1.getNumDims(), ws1.getNumberHistograms())

   # Load ILL d17 data file (TOF mode) into a workspace 2D using a user-defined angle of 15 degrees:
   ws2 = Load('ILLD17_111686.nxs', "InputAngle"="user defined", BraggAngle = "30")

   # The SampleLog entry stheta will be the user defined angle of 30 degrees:
   angleBragg = ws2.getRun().getProperty("stheta").value * 180. / np.pi

   print("The detector of workspace {} was rotated by {} degrees. The offset angle is 1.272018 degrees.").format(ws2.name(), 2. * angleBragg)

   import numpy as np
   ws3 = mtd['out']

   instrument = ws3.getInstrument()
   samplePos = ws3.getInstrument().getSample().getPos()
   sourcePos = ws3.getInstrument().getSource().getPos()

   # The scattering angle 
   scatteringAngleLeft = ws3.getDetector(174).getTwoTheta(samplePos, (samplePos - sourcePos))*180.0/np.pi
   scatteringAngleRight = ws3.getDetector(175).getTwoTheta(samplePos, (samplePos - sourcePos))*180.0/np.pi
   print(The scattering angle is {} and {} for detectors {} and {}, respectively.).format(scatteringAngleLeft, scatteringAngleRight, 174, 175)

Output:

.. testoutput:: LoadILLReflectometry	
	Workspace ILLD17_111686 has 2 dimensions (spectra) and has 258 histograms.

        The detector of workspace ILLD17_111686 was rotated by 61.0 degrees. The offset angle is 1.272018 degrees.
 
        The scattering angle is 61.098289073677 and 61.00498229043 for detectors 174 and 175. respectively. 


.. testcleanup:: LoadILLReflectometry

   DeleteWorkspace(ws)
   import os,mantid
   filename=mantid.config.getString("defaultsave.directory")+"test.map"
   os.remove(filename)

.. categories::

.. sourcelink::
