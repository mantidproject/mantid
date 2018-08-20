.. algorithm::

.. summary::

.. relatedalgorithms::

.. properties::

Description
-----------

This algorithm saves the first spectrum of a workspace in ASCII format with .mft file name extension and can be used by software like Motofit.
In case of histogrammed input data, the resulting file will contain the bin centre for the quantity `q`.
It is especially useful for saving reflectometry reduction data.
A file can be loaded back into Mantid by :ref:`algm-LoadAscii`, which will not have an instrument defined and `Sample Logs` are missing.

File Format
-----------

The file contains minimum 21 header lines each separating its name and value by a colon, followed by an empty line and a line describing the quantities of each column which are `q`, `refl`, `refl_error` and eventually `q_res (FWHM)`, if present, and the data.
The header lines contain the following information: `Instrument`, `User-local contact`, `Title`, `Subtitle`, `Start date + time`, `End date + time`, `Theta 1 + dir + ref numbers`, `Theta 2 + dir + ref numbers`, `Theta 3 + dir + ref numbers`, (foreseen potentially added angle(s),) followed by 9 user defined parameter lines, followed by potentially added user defined parameter lines, `Number of file format`, `Number of data points`.
The version of the file format is set randomly to a higher value (40) in order to exceed all ILL Cosmos versions.
For the ILL instruments D17 and FIGARO, obligatory header lines will be automatically filled by using the workspaces `Sample Logs` information which can be modified as shown in the example.
No header lines will be written when disabling the input option `WriteHeader`.
This algorithm writes data in scientific exponential notation (E-notation) of double-precision.

Usage
-----

**Example - Save a workspace in MFT ASCII**

.. testcode:: SaveMFT_general_usage

    # import the os path libraries for directory functions
    import os

    # create histogram workspace
    x = range(0, 4)
    y = range(0, 3)
    e = [1]*3
    dx = [9.5]*3

    ws = CreateWorkspace(DataX=x, DataY=y, DataE=e, Dx=dx)

    # Create an absolute path by joining the proposed filename to a directory
    # os.path.expanduser("~") used in this case returns the home directory of the current user
    file = os.path.join(os.path.expanduser("~"), "ws.mft")

    # Add Sample Log entries
    # Add a Title entry which will be automatically used
    AddSampleLog(Workspace=ws, LogName='Title', LogText='MyTest', LogType='String')
    # Add an entry called d as a Parameter (then, only eight not defined parameter lines remain):
    AddSampleLog(Workspace=ws, LogName='d', LogText='0.3', LogType='Number', LogUnit='mm', NumberType='Double')

    # Save with mft extension and using the option LogList: Title will be added to a required header line and d will be additionally added
    # to the first parameter field.
    SaveMFT(InputWorkspace=ws, Filename=file, LogList=['Title', 'd'])

    if os.path.exists(file):
      myFile = open(file, 'r')
      print(myFile.read())

.. testoutput:: SaveMFT_general_usage
   :options: +NORMALIZE_WHITESPACE

   MFT
   Instrument : Not defined
   User-local contact : Not defined
   Title : MyTest
   Subtitle : Not defined
   Start date + time : Not defined
   End date + time : Not defined
   Theta 1 + dir + ref numbers : Not defined
   Theta 2 + dir + ref numbers : Not defined
   Theta 3 + dir + ref numbers : Not defined
   d : 0.29999999999999999
   Parameter  : Not defined
   Parameter  : Not defined
   Parameter  : Not defined
   Parameter  : Not defined
   Parameter  : Not defined
   Parameter  : Not defined
   Parameter  : Not defined
   Parameter  : Not defined
   Number of file format : 40
   Number of data points : 3

                              q                        refl                    refl_err                q_res (FWHM)
          5.000000000000000e-01       0.000000000000000e+00       1.000000000000000e+00       9.500000000000000e+00
          1.500000000000000e+00       1.000000000000000e+00       1.000000000000000e+00       9.500000000000000e+00
          2.500000000000000e+00       2.000000000000000e+00       1.000000000000000e+00       9.500000000000000e+00

.. testcleanup:: SaveMFT_general_usage

   if os.path.exists(file):
     # Delete file
     os.remove(file)

.. categories::

.. sourcelink::
