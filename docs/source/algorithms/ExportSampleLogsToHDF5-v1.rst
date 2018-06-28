.. algorithm::

.. summary::

.. relatedalgorithms::

.. properties::

Description
-----------

Exports a workspace's sample logs to a HDF5 file, with an option to
exclude certain logs from being saved.

HDF5 File Format
################

The algorithm makes no attempt to sort the sample logs into a
hierarchy - each log is saved to its own dataset, in a group called
**Sample Logs**. Time-series logs are averaged, and the first element
of ``FloatArrayProperty`` time series logs is used. Where logs have
units, the units are saved in the dataset's metadata.

Usage
-----

**Example - Export sample logs to a new HDF5 file:**

.. testcode:: ExportSampleLogsToHDF5

   import h5py
   import os
   import sys

   # The following is needed as strings read from h5py are formatted differently between Python 2 and 3
   run_number_output = "Run number (read from file): {}"
   if sys.version_info[0] < 3:
       format_run_number = run_number_output.format
   else:
       format_run_number = lambda run_number: run_number_output.format(run_number.tostring().decode())

   input_ws = Load(Filename="ENGINX00213855.nxs")

   output_filename = os.path.join(config["defaultsave.directory"],
 	                          "ExportSampleLogsToHDF5DocTest.hdf5")

   ExportSampleLogsToHDF5(InputWorkspace=input_ws,
                          Filename=output_filename,
                          Blacklist="nspectra")

   with h5py.File(output_filename, "r") as f:
       sample_logs_group = f["Sample Logs"]
       
       print("Proton charge saved to file: {}".format("tot_prtn_chrg" in sample_logs_group))
       print(format_run_number(sample_logs_group["run_number"][0]))
       print("nspectra (blacklisted) saved to file: {}".format("nspectra" in sample_logs_group))

.. testcleanup:: ExportSampleLogsToHDF5

   os.remove(output_filename)

Output:

.. testoutput:: ExportSampleLogsToHDF5

   Proton charge saved to file: True
   Run number (read from file): 213855
   nspectra (blacklisted) saved to file: False

.. categories::

.. sourcelink::
