.. _TubeCalibFitParams:

TubeCalibFitParams
==================

An object of the python class (defined in tube_calib_fit_param.py) holds the parameters needed for fitting the positions of the peaks formed by the slits or edges. The constructor has the following arguments: 

+-----+-----------------+------------+----------+--------------------------------------------------+
|Order|Name             |Type        |Default   |Description                                       |
+=====+=================+============+==========+==================================================+
|1    |Peaks            |Array of    |Mandatory |The expected positions of the peaks in pixels     |
|     |                 |real numbers|          |                                                  |
+-----+-----------------+------------+----------+--------------------------------------------------+
|2    |Height           |real number |1000.0    |Expected height of peak                           |
+-----+-----------------+------------+----------+--------------------------------------------------+
|3    |Width            |real number |30.0      |Expected width of peak in pixels                  |
+-----+-----------------+------------+----------+--------------------------------------------------+
|4    |ThreePointMethod |boolean     |False     |If true, then the Three Point Method for MERLIN   |
|     |                 |            |          |is used, else it's peak fitting of slits          |
+-----+-----------------+------------+----------+--------------------------------------------------+

After creation it just needs to be passed to getCalibration of :ref:`Tube_calib`. 

If ThreePointMethod is True, then the other arguments are ignored and Peaks can be an empty array. 
