.. algorithm::

.. summary::

.. alias::

.. properties::

Description
-----------

Some instrument definition file (:ref:`IDF <InstrumentDefinitionFile>`)
positions are only approximately correct and the true positions are
located within data files. This algorithm reads the detector positioning
from the supplied file and updates the instrument accordingly. It
currently supports ISIS Raw, ISIS NeXus files and ASCII files.

It is assumed that the positions specified in the file are all with
respect to the a coordinate system defined with its origin at the sample
position. Note that this algorithm moves the detectors without
subsequent rotation, hence this means that detectors may not for example
face the sample perfectly after this algorithm has been applied.

Related or similar algorithms
##############################

See :ref:`algm-LoadDetectorInfo` which can do similar job modifying the detector parameters and positions and working with special format ASCII files or ISIS raw or special nexus files. 


Additional Detector Parameters Using ASCII File
###############################################

The ASCII format allows a multi-column text file to provide new
positions along with additional parameters for each detector. If a text
file is used then the ``AsciiHeader`` parameter is required as it
identifies each column in the file as header information in the file is
always ignored. There is a minor restriction in that the first column is
expected to specify either a detector ID or a spectrum number and will
never be interpreted as anything else.

The keywords recognised by the algorithm to pick out detector position
values & spectrum/ID values are: spectrum, ID, R,theta, phi. The
spectrum/ID keywords can only be used in the first column. A dash (-) is
used to ignore a column.

As an example the following header:

::

    spectrum,theta,-,-,R

and the following text file:

::

        1   0.0000  -4.2508  11.0550  -2.4594
        2   0.0000   0.0000  11.0550   2.3800
        3 130.4653  -0.4157  11.0050   0.6708
        4 131.9319  -0.5338  11.0050   0.6545
        5 133.0559  -0.3362  11.0050   0.6345

would tell the algorithm to interpret the columns as:

#. Spectrum number
#. Theta position value
#. Ignored: Detector's electronics delay time -- the difference of pulse synchronization signal time and the time DAE starts measuring frame time of given spectra.
#. Ignored: Sample to source distance for the particular detector
#. Sample-detector distance R.


Usage
-----

**Example - Update Instrument:**

.. testcode:: exUpdateInstrumentFromFile

   import math
   import os
   # priting procedure
   def print_10_detectors(instr_type,instr):
       ''' print first 10 detectors from given instrument '''
        # get first 10 detectors using detector ID 

       print "{0} {1} instrument".format(instr_type, instr.getName())
       for i in xrange(0,10):
         if i<3:
             detBase = 1
         else:
             detBase = 1101-3
         detID = detBase+i
         det1 = instr.getDetector(detID);
         pos = det1.getPos();
         print 'det with ID: {0:5} is monitor? {1:5}, polar angle: {2:10.3f}, position: | {3:<10.3f} | {4:<10.3f} | {5:<10.3f}|\n'.format(\
                detID,det1.isMonitor(),(det1.getPhi()*(180/math.pi)),pos.X(),pos.Y(),pos.Z()),
       print '*********************************************************************************'
        
   #--------------------------------------------------------------------------------------      
   # create sample workspace
   ws=CreateSampleWorkspace();  
   #--------------------------------------------------------------------------------------      
   # load MARI
   det=LoadInstrument(ws,InstrumentName='MARI')   
   inst1=ws.getInstrument();   
   #   
   print_10_detectors('unCalibrated',inst1);
   #--------------------------------------------------------------------------------------   
   # Prepare calibration file changing first 6 detectors & monitors
   file_name = os.path.join(config["defaultsave.directory"], "TestCalibration.dat")    
   f = open(file_name,'w');
   # prepare through each spectra in the test workspace and change its detector calibration parameters
   f.write(' Test calibration file \n')   
   f.write(' detID  theta  delay source_dist detector_dist\n')
   for i in xrange(0,6):
      f.write('{0}  {1}  {2}  {3}  {4}  {5}\n'.format(i+1,(i+1)*3.1415926/200,0.5,100,(i+1)*3.1415926/5,10))
    
   f.close();
   #--------------------------------------------------------------------------------------      
   # CALIBRATE:
   UpdateInstrumentFromFile(ws,Filename=file_name,AsciiHeader='spectrum,theta,-,-,phi,R',MoveMonitors=True,SkipFirstNLines=2)
   inst1=ws.getInstrument();
   #--------------------------------------------------------------------------------------      
   # look at the result:
   print_10_detectors('Calibrated',inst1);

    

.. testcleanup:: exUpdateInstrumentFromFile

   os.remove(file_name)   
   

**Output:**

.. testoutput:: exUpdateInstrumentFromFile

   unCalibrated MARI instrument
   det with ID:     1 is monitor?     1, polar angle:      0.000, position: | 0.000      | 0.000      | -4.739    |
   det with ID:     2 is monitor?     1, polar angle:      0.000, position: | 0.000      | 0.000      | -1.442    |
   det with ID:     3 is monitor?     1, polar angle:      0.000, position: | 0.000      | 0.000      | 5.820     |
   det with ID:  1101 is monitor?     0, polar angle:    -68.640, position: | 0.347      | -0.888     | 3.907     |
   det with ID:  1102 is monitor?     0, polar angle:    -69.300, position: | 0.347      | -0.919     | 3.900     |
   det with ID:  1103 is monitor?     0, polar angle:    -69.920, position: | 0.347      | -0.950     | 3.893     |
   det with ID:  1104 is monitor?     0, polar angle:    -70.510, position: | 0.347      | -0.981     | 3.885     |
   det with ID:  1105 is monitor?     0, polar angle:    -71.060, position: | 0.347      | -1.012     | 3.877     |
   det with ID:  1106 is monitor?     0, polar angle:    -71.570, position: | 0.347      | -1.043     | 3.869     |
   det with ID:  1107 is monitor?     0, polar angle:    -72.060, position: | 0.347      | -1.073     | 3.861     |
   *********************************************************************************
   Calibrated MARI instrument
   det with ID:     1 is monitor?     1, polar angle:      0.628, position: | 0.003      | 0.000      | 10.000    |
   det with ID:     2 is monitor?     1, polar angle:      1.257, position: | 0.005      | 0.000      | 10.000    |
   det with ID:     3 is monitor?     1, polar angle:      1.885, position: | 0.008      | 0.000      | 10.000    |
   det with ID:  1101 is monitor?     0, polar angle:      2.513, position: | 0.011      | 0.000      | 10.000    |
   det with ID:  1102 is monitor?     0, polar angle:      3.142, position: | 0.014      | 0.001      | 10.000    |
   det with ID:  1103 is monitor?     0, polar angle:      3.770, position: | 0.016      | 0.001      | 10.000    |
   det with ID:  1104 is monitor?     0, polar angle:    -70.510, position: | 0.347      | -0.981     | 3.885     |
   det with ID:  1105 is monitor?     0, polar angle:    -71.060, position: | 0.347      | -1.012     | 3.877     |
   det with ID:  1106 is monitor?     0, polar angle:    -71.570, position: | 0.347      | -1.043     | 3.869     |
   det with ID:  1107 is monitor?     0, polar angle:    -72.060, position: | 0.347      | -1.073     | 3.861     |
   *********************************************************************************
  


.. categories::
