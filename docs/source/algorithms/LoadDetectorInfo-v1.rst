.. algorithm::

.. summary::

.. relatedalgorithms::

.. properties::

.. |AlphaX| replace:: :math:`\alpha_{x}`

.. |AlphaY| replace:: :math:`\alpha_{y}`

.. |AlphaZ| replace:: :math:`\alpha_{z}`

.. |WX| replace:: :math:`W_{x}`

.. |WY| replace:: :math:`W_{y}`

.. |WZ| replace:: :math:`W_{z}`


Description
------------

The algorithm loads specific detector properties and, if *RelocateDets* property is true, the detector positions into the instrument,
attached to the target workspace and modifies these properties as described below.

The detection time delay for each detector is subtracted from the time of flight bin boundaries in the spectrum associated with that detector.
It is required that all the monitors have the same time delay and if this is non-zero the delay time is added to all TOF values.
It is important that the units of the input workspace are TOF in microseconds, that :ref:`algm-GroupDetectors`
has not been run and this algorithm is only applied once to a workspace.

Values for the partial pressure of **3He** and **wall thickness** are added into the parameter map for each detector
in a form that can be read by :ref:`algm-DetectorEfficiencyCor`. That is, the values are assumed to be in atmospheres
and meters, respectively, and the properties are stored internally in the workspace parameter map as **3He(atm)** and **wallT(m)**.
The values are likely to be read from the same **RAW** file that the workspace was loaded from or a
**DAT** file that corresponds to the same run or series of experimental runs.

Spectra whose associated detector data are not found in the input **DAT** or **RAW** file will not have their
time of flight bin boundaries adjusted. Similarly nothing will be written to the parameter map for those detectors,
the algorithm will continue to process data as normal but a warning will be written to the log.
Detectors that are listed in the input file but do not exist in the instrument definition file will be ignored and details will be written to the log.

If all the time offsets are the same and the file **appears** to contain enough data for all detectors
all detector spectra will use same bin boundaries, where possible.
This will make the algorithm run much more quickly and use less memory.

When using a **RAW** file the time offset for each detector is read from the "hold off" table in the file's
header while pressure and wall thickness data must be present in the user table array.
The format for **.DAT** files is specified in the document **DETECTOR.DAT format** written by Prof G Toby Perring and briefly described below.

If the RelocateDets option is set to true, (it is false by default)
then the detectors are moved to the corresponding positions specified in the data file provided.

Related or similar algorithms
##############################

See :ref:`algm-UpdateInstrumentFromFile` which can do similar job in modifying the detector parameters and positions and  accepting arbitrary ASCII files.

**Detectors.dat** data format
#############################

The detector data can be stored as ASCII or `NeXus <http://download.nexusformat.org/>`_ data file. It contains roughly corresponds
to the detector.dat file, present on the instrument and written to the run files. The contents of these data can be overwritten by the file,
provided as input for this algorithm

The description of the ASCII DETECTOR.dat file is provided in the table below. Nexus file format can come in two flavors.
The first one is completely equivalent to the ASCII 19 column data format and introduced to increase the speed of accessing these data in binary format,
where the second one left for the compatibility with Libisis.
It has meaningful data corresponding to the columns 1-6, 17&18 below, but does not support multiple tube pressures and wall thickness.

The :ref:`algm-LoadDetectorInfo` algorithm currently reads and interprets rows 1-6,17&18 of the table,
provided below (or columns of det.dat file) or correspondent data blocks from the NeXus file.
It also does not understand the short (15 columns) MARI detector.dat ASCII file format (see **ISIS detector.dat files** below).

Co-ordinate frames
##################

For the purposes of the detector table we choose a right handed set of axes fixed in the spectrometer:

- x-axis  -- horizontal
- y-axis  -- vertical
- z-axis  -- parallel to :math:`k_{i}`

Centers of each detector element are defined in spherical polar co-ordinates as seen from the sample position:

- **THETA** --  Polar angle measured from the z-axis (what we would normally call the scattering angle PHI). Note that  0< **THETA** <180
- **PHI**   --  Azimuthal angle measured from the x-axis in a right handed sense (what TGP, CDF	and RD01 call -BETA).
                  For example, the West Bank of HET has PHI=0, the North Bank has PHI=90, the South Bank has PHI=270.

To specify the orientation of a detector, define axes x', y', z' as follows:

-  x'-axis -- increasing THETA
-  y'-axis -- increasing PHI
-  z'-axis -- parallel to the line joining sample and detector

The natural coordinate frame for the detector, xd, yd, zd, may not coincide with x', y', z'.
For example, the natural frame for a gas tube is with zd along the axis of the tube, and the direction of xd chosen to be perpendicular to the line joining the detector with the sample.
The characteristic dimensions of the detector, W_x, W_y, W_z, are given in the frame xd, yd, zd.
The detector coordinate axes xd, yd, zd are related to x', y', z' by a rotation.
The transformation is be expressed by a three-component vector  :math:`\alpha_{x},\alpha_{y},\alpha_{z}`,
where the magnitude of  the vector gives the angle of rotation in a right-hand sense,
and the normalized elements give the components along x', y', z' of the unit vector about which the rotation takes place.
The magnitude of the vector is in degrees.

- e.g. non-PSD gas tube on the Debye-Scherrer cone:
       :math:`\alpha_{x} = -90^{o};\alpha_{y} = \alpha_{z} = 0^{o}; W_{x}=W_{y}= 0.0254, W_{z}=0.300`

- e.g. Davidson bead monitor filling the HET beam at the monitor_2 position:
       :math:`\alpha_{x} =\alpha_{y}= \alpha_{z} = 0^{o}; W_{x}=W_{y}=0.045, W_{z}=0.00025`

Note that for PSD detectors the angles and dimensions refer to the pixel, not the whole tube. For HET, :math:`W_{z}= 0.914/64 = 0.01428`.

File format
###########

The file consists of number of ASCII columns, separated by spaces. All distances are in meters, and all angles in degrees.

+---------------+-------------+-------------+-------------------------------------------------------------------------------------------------------+
| Column Number | Column Name | Column Type |                                Column Description                                                     |
+===============+=============+=============+=======================================================================================================+
| 1             | DET_NO      | integer     | Detector index number as in SPECTRA.DAT                                                               |
+---------------+-------------+-------------+-------------------------------------------------------------------------------------------------------+
| 2             | DELTA       | real        | Electronics delay time ( :math:`\mu s` ). The origin is up to you. HOMER uses the peak in monitor_2   |
|               |             |             | as the origin of time, so the only thing that really matters is the difference in the delay           |
|               |             |             | time between the detectors and the monitors.                                                          |
+---------------+-------------+-------------+-------------------------------------------------------------------------------------------------------+
| 3             | L2          | real        |Sample - detector distance (m)                                                                         |
+---------------+-------------+-------------+-------------------------------------------------------------------------------------------------------+
| 4             | CODE        | integer     | Code number that describes the detector type.  Up to now this column has                              |
|               |             |             | been redundant so the old files can contain unity for all detectors.                                  |
|               |             |             | Proper detectors should now follow the scheme:                                                        |
|               |             |             |                                                                                                       |
|               |             |             | 0.  Dummy detector entry (see later)                                                                  |
|               |             |             | 1.  Davidson scintillator bead monitor (or just monitor)                                              |
|               |             |             | 2.  non-PSD gas tube                                                                                  |
|               |             |             | 3.  PSD gas tube                                                                                      |
+---------------+-------------+-------------+-------------------------------------------------------------------------------------------------------+
| 5             | THETA       | real        |Scattering angle (deg)                                                                                 |
+---------------+-------------+-------------+-------------------------------------------------------------------------------------------------------+
| 6             | PHI         | real        |Azimuthal angle (deg). Origin and rotation sense defined above                                         |
+---------------+-------------+-------------+-------------------------------------------------------------------------------------------------------+
| 7             | |WX|        | real        |True detector dimensions (m) in the frame xd'                                                          |
+---------------+-------------+-------------+-------------------------------------------------------------------------------------------------------+
| 8             | |WY|        | real        |True detector dimensions (m) in the frame yd'                                                          |
+---------------+-------------+-------------+-------------------------------------------------------------------------------------------------------+
| 9             | |WZ|        | real        |True detector dimensions (m) in the frame zd'                                                          |
+---------------+-------------+-------------+-------------------------------------------------------------------------------------------------------+
| 10            | F_x         | real        | False detector dimensions (m) in the frame xd' to avoid gaps between detectors                        |
+---------------+-------------+-------------+-------------------------------------------------------------------------------------------------------+
| 11            | F_y         | real        |False detector dimensions (m) in the frame yd' to avoid gaps between detectors                         |
+---------------+-------------+-------------+-------------------------------------------------------------------------------------------------------+
| 12            | F_z         | real        |False detector dimensions (m) in the frame zd' to avoid gaps between detectors                         |
+---------------+-------------+-------------+-------------------------------------------------------------------------------------------------------+
| 13            | |AlphaX|    | real        | x-coordinate of the vector describing orientation of detector in the co-ordinate frame defined above. |
+---------------+-------------+-------------+-------------------------------------------------------------------------------------------------------+
| 14            | |AlphaY|    | real        | y-coordinate of the vector describing orientation of detector in the co-ordinate frame defined above. |
+---------------+-------------+-------------+-------------------------------------------------------------------------------------------------------+
| 15            | |AlphaZ|    | real        | z-coordinate of the vector describing orientation of detector in the co-ordinate frame defined above. |
+---------------+-------------+-------------+-------------------------------------------------------------------------------------------------------+
| The columns with numbers higher then those described above contain information about the detectors that is dependent on the detector type:        |
+---------------+-------------+-------------+-------------------------------------------------------------------------------------------------------+
|    **CODE = 0 (Dummy detector entry)** :                                                                                                          |
+---------------+-------------+-------------+-------------------------------------------------------------------------------------------------------+
| 16            | det_1       | real        | Frequently, some of the inputs to the data acquisition electronics do not have any detectors          |
|               |             |             | plugged into them. To ensure that any noise on these inputs is safely directed to a 'dust-bin'        |
|               |             |             | spectrum, they are given detector numbers which are associated with spectrum 0 in SPECTRA.DAT.        |
|               |             |             | Dummy entries in DETECTOR.DAT are required for each of these dummy detectors.                         |
|               |             |             | These entries should be given detector CODE = 0, which will be used to indicate that the other        |
|               |             |             | entries in DETECTOR.DAT can be ignored. For the sake of clarity, set all DELTA, L2...DET_4 to         |
|               |             |             | zero for dummy detectors.                                                                             |
+---------------+-------------+-------------+-------------------------------------------------------------------------------------------------------+
| 17            | det_2       | real        | The same as 16                                                                                        |
+---------------+-------------+-------------+-------------------------------------------------------------------------------------------------------+
| 18            | det_2       | real        | The same as 16                                                                                        |
+---------------+-------------+-------------+-------------------------------------------------------------------------------------------------------+
| 19            | det_2       | real        | The same as 16                                                                                        |
+---------------+-------------+-------------+-------------------------------------------------------------------------------------------------------+
|     **CODE = 1 (monitor)** :                                                                                                                      |
+---------------+-------------+-------------+-------------------------------------------------------------------------------------------------------+
| 16            | det_1       | real        | Dead time  ( :math:`\mu s` ). Important for old detectors and high counting rate.                     |
+---------------+-------------+-------------+-------------------------------------------------------------------------------------------------------+
| 17            | det_2       | real        | Macroscopic absorption cross-section :math:`\Sigma ;(m^{-1}meV^{-0.5})`.                              |
|               |             |             | For our monitors this is for Li scintillator glass. (I think I know what :math:`\Sigma`;              |
|               |             |             | is approximately, but we don't at present use it anywhere, so set to zero)                            |
+---------------+-------------+-------------+-------------------------------------------------------------------------------------------------------+
| 18            | det_3       | real        | Ignored. Set to zero                                                                                  |
+---------------+-------------+-------------+-------------------------------------------------------------------------------------------------------+
| 19            | det_4       | real        | Ignored. Set to zero                                                                                  |
+---------------+-------------+-------------+-------------------------------------------------------------------------------------------------------+
|   **CODE = 2 (non-PSD gas tube)** :                                                                                                               |
+---------------+-------------+-------------+-------------------------------------------------------------------------------------------------------+
| 16            | det_1       | real        | Dead time ( :math:`\mu s` ). Important for old detectors and high counting rate.                      |
+---------------+-------------+-------------+-------------------------------------------------------------------------------------------------------+
| 17            | det_2       | real        | Gas tube detector **3He** partial pressure (atms)                                                     |
+---------------+-------------+-------------+-------------------------------------------------------------------------------------------------------+
| 18            | det_3       | real        | Gas tube wall thickness (m) ( 0.00080 )                                                               |
+---------------+-------------+-------------+-------------------------------------------------------------------------------------------------------+
| 19            | det_4       | real        | Ignored. Set to zero                                                                                  |
+---------------+-------------+-------------+-------------------------------------------------------------------------------------------------------+
|   **CODE = 3  (PSD gas tube)** :                                                                                                                  |
+---------------+-------------+-------------+-------------------------------------------------------------------------------------------------------+
| 16            | det_1       | real        | Dead time ( :math:`\mu s` ). Important for old detectors and high counting rate.                      |
+---------------+-------------+-------------+-------------------------------------------------------------------------------------------------------+
| 17            | det_2       | real        | Gas tube detector **3He** partial pressure (atms) (10.0 or 6.4)                                       |
+---------------+-------------+-------------+-------------------------------------------------------------------------------------------------------+
| 18            | det_3       | real        | Gas tube wall thickness (m) ( 0.00080 )                                                               |
+---------------+-------------+-------------+-------------------------------------------------------------------------------------------------------+
| 19            | det_4       | real        | Index of tube to which the pixel belongs. Each PSD gas tube must be given a unique identifier.        |
|               |             |             | This enables programs that use DETECTOR.DAT to recognize that pixels have come from the same PSD tube.|
+---------------+-------------+-------------+-------------------------------------------------------------------------------------------------------+


ISIS DETECTOR.DAT raw files
###########################


The ISIS raw files seem to have two possible entries - MARI is non-standard for some reason. The table below describes correspondence between the fields
in ASCII file above and the data containing in DETECTOR.DAT file present on data acquisition machine  and the data written to the RAW file on different ISIS instruments.

+----+--------------+------------------+---------------+
|    | Field        |  Field name in RAW file          |
+----+--------------+------------------+---------------+
|    | Name in      | All instruments: | MARI fields:  |
| N  | ASCII file,  | Fields in use:14 | In use:10.    |
|    | table above  | ASCII ncol:19    | ASCII ncol:15 |
+====+==============+==================+===============+
| 1  |  det_no      |     spec         |    spec       |
+----+--------------+------------------+---------------+
| 2  |  delta       |     delt         |    delt       |
+----+--------------+------------------+---------------+
| 3  |  l2          |    len2          |   len2        |
+----+--------------+------------------+---------------+
| 4  |  code        |    code          |     code      |
+----+--------------+------------------+---------------+
| 5  |  theta       |    tthe          |     tthe      |
+----+--------------+------------------+---------------+
| 6  |  phi         |     ut1          |     ut1       |
+----+--------------+------------------+---------------+
| 7  |  |WX|        |     ut2          |     ut2       |
+----+--------------+------------------+---------------+
| 8  |  |WY|        |     ut3          |     ut3       |
+----+--------------+------------------+---------------+
| 9  |  |WZ|        |     ut4          |     ut4       |
+----+--------------+------------------+---------------+
| 10 |  F_x         |     ut5          |     ---       |
+----+--------------+------------------+---------------+
| 11 |  F_y         |     ut6          |     ---       |
+----+--------------+------------------+---------------+
| 12 |  F_z         |     ut7          |     ---       |
+----+--------------+------------------+---------------+
| 13 | |AlphaX|     |     ut8          |     ut5       |
+----+--------------+------------------+---------------+
| 14 | |AlphaY|     |     ut9          |     ut6       |
+----+--------------+------------------+---------------+
| 15 | |AlphaZ|     |     ut10         |     ut7       |
+----+--------------+------------------+---------------+
| 16 |  det_1       |     ut11         |     ---       |
+----+--------------+------------------+---------------+
| 17 |  det_2       |     ut12         |     ut8       |
+----+--------------+------------------+---------------+
| 18 |  det_3       |     ut13         |    ut9        |
+----+--------------+------------------+---------------+
| 19 |  det_4       |      ut14        |    ut10       |
+----+--------------+------------------+---------------+


Usage
------------

**Example - LoadDetectorInfo:**

Calibrate **MARI** using full format detector calibration file.
Note, that :ref:`algm-LoadDetectorInfo` algorithm does not modify the monitors. Test test below provides calibration file for six detectors and test output shows,
that the first three detectors (monitors) were not touched and the next three were completely modified.

.. testcode:: exLoadDetectorInfo

   import math
   import os

   # printing procedure
   def write_detectors(instr_type,instr,ndet):
       ''' print first ndet detectors from given instrument '''

       print("{0} {1} instrument".format(instr_type, instr.getName()))
       print('det ID | monitor? | polar angle| position X | position Y | position Z |  Pressure  | Wall thick |')

       # get first nder detectors using detector ID
       for i in range(0,ndet):
         if i<3:
             detBase = 1
         else:
             detBase = 1101-3
         detID = detBase+i
         det1 = instr.getDetector(detID);
         pos = det1.getPos();
         pressure = det1.getNumberParameter('TubePressure');
         thickness = det1.getNumberParameter('TubeThickness');
         print(' {0:5} | {1:8} | {2:10.3f} | {3:>10.3f} | {4:>10.3f} | {5:>10.3f} | {6:10} | {7:10} |'.format(\
                detID,det1.isMonitor(),(det1.getPhi()*(180/math.pi)),pos.X(),pos.Y(),pos.Z(),pressure[0],thickness[0]))
       print('*********************************************************************************')
   #
   def prepare_test_detector(ind):
        """  prepare modified detector with random test values  """
        if ind<3:
            detBase = 1
            offset=0;
            l2 = -10  # incorrect  L2 for testing changes
            code=1
            theta = 180 # incorrect theta for testing changes
            phi = 1     # incorrect phi for testing changes
            w_xyz = 0.5 # incorrect w_x,w_y, w_z for testing changes
            f_xyz = 5   # incorrect for testing changes
            a_xyz = 10  # incorrect for testing changes
            det1  = 1   # incorrect for testing changes
            det2  = 5   # incorrect tube pressure for testing changes
            det3  = 0.09 # incorrect wall thickness for testing changes
            det4  = 1   # incorrect for testing changes
        else:
            detBase = 1101-3
            offset=5.5
            l2 =  10  # incorrect  L2 for testing changes
            code = 3
            theta = -180 #  incorrect theta for testing changes
            phi = 90     # incorrect phi for testing changes
            w_xyz = 1.5  # incorrect w_x,w_y, w_z for testing changes
            f_xyz = 50   # incorrect for testing changes
            a_xyz = 20   # incorrect for testing changes
            det1  = 3    # incorrect for testing changes
            det2  = 15   # incorrect tube pressure for testing changes
            det3  = 0.9  # incorrect wall thickness for testing changes
            det4  = 666  # incorrect for testing changes

        return (detBase,offset,l2,code,theta,phi,w_xyz,f_xyz,a_xyz,det1,det2,det2,det4)


   def write_test_cal_file(filename,instrument,ndet):
      """ writes partial detector.dat file  modified for testing purposes
            filename   -- the string, describing the name of the file to write:
            instrument -- the pointer to instrument to modify
            ndet       -- number of detectors to modify using this calibration file for testing purposes
      """

      f = open(filename,'w');
      f.write("Full format DETECTOR.DAT generated by CREATE_DETECTOR_FILE (part of it written for testing purposes)");
      f.write("{0}    14\n".format(ndet))
      f.write("det no.  offset    l2     code     theta        phi         w_x         w_y         w_z         f_x         f_y         f_z         a_x         a_y         a_z        det_1       det_2       det_3       det4\n");

      for i in range(0,ndet):
         detBase,offset,l2,code,theta,phi,w_xyz,f_xyz,a_xyz,det1,det2,det3,det4=prepare_test_detector(i);
         detID = detBase+i
         f.write("{0:>9} {1:>7} {2: >8f} {3:>5} {4:>11f} {5:>11f} {6:>11f} {7:>11f} {8:>11f} {9:>11f} {10:>11f} {11:>11f} {12:>11f} {13:>11f} {14:>11f} {15:>11f} {16:>11f} {17:>11f} {18:>11f} {19:>11f}\n".format(\
             detID,offset,l2,code,theta,phi,w_xyz,w_xyz,w_xyz,f_xyz,f_xyz,f_xyz,a_xyz,a_xyz,a_xyz,a_xyz,det1,det2,det3,det4))
      #  end write loop
      f.close()

   #-------------------- ------------------------------------------------------------------
   #  TEST  THE ALGORITHM
   #--------------------------------------------------------------------------------------
   # create sample workspace
   ws=CreateSampleWorkspace();
   #--------------------------------------------------------------------------------------
   # load MARI
   det=LoadInstrument(ws,InstrumentName='MARI', RewriteSpectraMap=True)
   inst1=ws.getInstrument();
   #
   write_detectors('unCalibrated',inst1,10);
   #--------------------------------------------------------------------------------------
   # Prepare calibration file changing first 6 detectors & monitors
   file_name = 'mari_det.dat'
   write_test_cal_file(file_name ,inst1,6);
   #--------------------------------------------------------------------------------------
   # CALIBRATE mari using full det.dat calibration file
   LoadDetectorInfo(ws,DataFilename=file_name,RelocateDets=True);
   inst1=ws.getInstrument();
   #--------------------------------------------------------------------------------------
   # look at the result:
   write_detectors('Calibrated',inst1,10);


.. testcleanup:: exLoadDetectorInfo

   os.remove(file_name)

**Output:**

.. testoutput:: exLoadDetectorInfo

   unCalibrated MARI instrument
   det ID | monitor? | polar angle| position X | position Y | position Z |  Pressure  | Wall thick |
        1 |        1 |      0.000 |      0.000 |      0.000 |     -4.739 |       10.0 |     0.0008 |
        2 |        1 |      0.000 |      0.000 |      0.000 |     -1.442 |       10.0 |     0.0008 |
        3 |        1 |      0.000 |      0.000 |      0.000 |      5.820 |       10.0 |     0.0008 |
     1101 |        0 |    -68.640 |      0.347 |     -0.888 |      3.907 |       10.0 |     0.0008 |
     1102 |        0 |    -69.300 |      0.347 |     -0.919 |      3.900 |       10.0 |     0.0008 |
     1103 |        0 |    -69.920 |      0.347 |     -0.950 |      3.893 |       10.0 |     0.0008 |
     1104 |        0 |    -70.510 |      0.347 |     -0.981 |      3.885 |       10.0 |     0.0008 |
     1105 |        0 |    -71.060 |      0.347 |     -1.012 |      3.877 |       10.0 |     0.0008 |
     1106 |        0 |    -71.570 |      0.347 |     -1.043 |      3.869 |       10.0 |     0.0008 |
     1107 |        0 |    -72.060 |      0.347 |     -1.073 |      3.861 |       10.0 |     0.0008 |
   *********************************************************************************
   Calibrated MARI instrument
   det ID | monitor? | polar angle| position X | position Y | position Z |  Pressure  | Wall thick |
        1 |        1 |      0.000 |      0.000 |      0.000 |     -4.739 |       10.0 |     0.0008 |
        2 |        1 |      0.000 |      0.000 |      0.000 |     -1.442 |       10.0 |     0.0008 |
        3 |        1 |      0.000 |      0.000 |      0.000 |      5.820 |       10.0 |     0.0008 |
     1101 |        0 |      0.000 |      0.000 |      0.000 |    -10.000 |        3.0 |       15.0 |
     1102 |        0 |      0.000 |      0.000 |      0.000 |    -10.000 |        3.0 |       15.0 |
     1103 |        0 |      0.000 |      0.000 |      0.000 |    -10.000 |        3.0 |       15.0 |
     1104 |        0 |    -70.510 |      0.347 |     -0.981 |      3.885 |       10.0 |     0.0008 |
     1105 |        0 |    -71.060 |      0.347 |     -1.012 |      3.877 |       10.0 |     0.0008 |
     1106 |        0 |    -71.570 |      0.347 |     -1.043 |      3.869 |       10.0 |     0.0008 |
     1107 |        0 |    -72.060 |      0.347 |     -1.073 |      3.861 |       10.0 |     0.0008 |
   *********************************************************************************




.. categories::

.. sourcelink::
