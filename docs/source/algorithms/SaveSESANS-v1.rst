.. algorithm::

.. summary::

.. relatedalgorithms::

.. properties::

Description
-----------

Saves the given workspace to a file which will be formatted in the
SESANS data format. A workspace with a single spectrum is expected,
where the X values are wavelength and the Y values are polarisation.

The sample thickness from the run metadata is used to normalise the polarisation data.
The :literal:`OverrideSampleThickness` parameter can be used to specify a different value for this if needed.

Measurement metadata
####################

To help identify the measurement, the following entries are added to the file header after the
mandatory SESANS headers:

============================== ========================================================================================
Header                         Value
============================== ========================================================================================
Facility                       The :literal:`Facility` property if set, otherwise the facility that the instrument
                               belongs to, as defined in the Mantid facilities configuration
Instrument                     The :literal:`Instrument` property if set, otherwise the instrument name of the input
                               workspace
Run_number                     The :literal:`RunNumber` property if set, otherwise the :literal:`run_number` sample log
                               of the input workspace
Sample_position                The :literal:`SamplePosition` property if set, otherwise the sample changer positions
                               (e.g. :literal:`AT` or :literal:`1CT`) recorded in the :literal:`SamplePos` sample log
                               during the selected time range, in order and separated by commas
Blank_run_number               The :literal:`BlankRunNumber` property
Sample_transmission_run_number The :literal:`SampleTransmissionRunNumber` property
Blank_transmission_run_number  The :literal:`BlankTransmissionRunNumber` property
Reduction_software             Always :literal:`Mantid`
Software_version               The version of Mantid used to save the file
Reduction_date                 The date and time (UTC, ISO 8601) at which the file was saved
============================== ========================================================================================

The :literal:`Facility`, :literal:`Instrument`, :literal:`Run_number`, :literal:`Sample_position` and
blank and transmission run number entries are omitted if no value can be determined.

The time range used for :literal:`Sample_position` is from the start to the end of the run, unless the workspace has
been filtered by time, for example using :ref:`FilterByTime <algm-FilterByTime>`. The position at the start of the time
range is included, but positions logged earlier are not. Repeated positions are only listed once, so a sample that
stayed in one position gives a single value. For older data, the :literal:`SamplePos` log may contain a number rather
than the name of the position.

Usage
-----

**Example - Loading a file**

.. testcode:: LoadSESANSRoundTrip

   import os

   # Create dummy workspace with sample thickness metadata
   dataX = [1,2,3,4,5]
   dataY = [6,1,9,14]
   dataE = [1,1,4,5]
   out_ws = CreateWorkspace(dataX, dataY, dataE)
   out_ws.setTitle("Dummy workspace")
   out_ws.sample().setThickness(10)

   file_path = os.path.join(config["defaultsave.directory"], "example.ses")

   # Do a 'roundtrip' of the data
   SaveSESANS(InputWorkspace=out_ws, Filename=file_path, ThetaZMax=1,ThetaYMax=1, EchoConstant=1, Sample="Sample")
   LoadSESANS(Filename=file_path, OutputWorkspace="in_ws")

   # Retrieve loaded workspace from ADS
   in_ws = mtd["in_ws"]
   print("Y values of loaded workspace = " + str(in_ws.y(0)))

.. testcleanup:: LoadSESANSRoundTrip

   os.remove(file_path)

Output:

.. testoutput:: LoadSESANSRoundTrip

   Y values of loaded workspace = [0.796338 0.       0.179365 0.130324]

.. categories::

.. sourcelink::
