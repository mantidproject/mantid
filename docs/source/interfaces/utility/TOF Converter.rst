.. _tof_converter:

Time of Flight Converter
========================

Description
-----------

Converts the units of single value which can be either entered by the user or by using a :ref:`workspace <Workspace>` with a single value in it.
ToF converter takes multiple inputs depending on the units you wish to convert between.

Input
-----

ToF Converter will take an Input value i.e the value you wish to convert.
If you wish to convert to/from Momentum Transfer or d-spacing to/from another unit, then a Scattering angle :math:`\theta` will need to be specified.
If you wish to convert Time of Flight then you will need to specify a Total Flight path in meters.

Output
------
The output of the program will be the converted value produced by the input values specified.

Available Conversion Units
--------------------------

Some of the units available are those registered with see: :ref:`Units <Unit Factory>`.
The units that are not specified in :ref:`Units <Unit Factory>` but are used in ToFConverter
are:

Nu (:math:`\nu`): :math:`\frac{h}{m_{N}\lambda^2}`

Velocity: :math:`\frac{h}{m_{N}\lambda}` and

Temperature: :math:`\frac{m_{N} v_{p}^2}{2k_{b}}`.

.. categories:: Interfaces
