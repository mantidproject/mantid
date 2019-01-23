.. algorithm::

.. summary::

.. relatedalgorithms::

.. properties::

Description
-----------

This algorithm saves the first spectrum of a workspace in ASCII format in order to be processed by software, e.g. Motofit.
It is possible to provide a group of workspaces as input.
In this case, each filename contains the corresponding workspace name.
The choice of the file extension (`.mft`, `.txt`, `.dat`) defines the file format and appends the `Filename` with the corresponding extension except for the `custom` field, which represents a custom format where the `Filename` can be chosen freely.
In case of histogrammed input data, the resulting file will contain the bin centre for the x-values representing the quantity `q`.
It is especially useful for saving reflectometry reduction data.
A file can be loaded back into Mantid by :ref:`algm-LoadAscii`, which will not have an instrument defined and `Sample Logs` are missing.
This algorithm writes data in scientific exponential notation (E-notation) of double-precision.
Please note that `SampleLog` entries are not case sensitive, editing fields `Title` and `title` both edit the field `title`.

Computation of resolution values
--------------------------------

For the `Custom File Format (Empty Field)`_ and the `TXT File Format`_ file format, the option `WriteResolution` enables the computation of the resolution values from existing x-values (points):

:math:`x_i \cdot \frac{x_1 - x_0}{x_1}`,

where the bin centre :math:`x_i` will be multiplied by the qutient of first and second bin centre :math:`x_{0}` and :math:`x_{1}`, respectively.

MFT File Format
---------------

The file contains minimum 21 header lines each separating its name and value by a colon, followed by an empty line and a line describing the quantities of each column which are `q`, `refl`, `refl_error` and eventually `q_res (FWHM)`, if present, and the data.
The header lines contain the following information: `Instrument`, `User-local contact`, `Title`, `Subtitle`, `Start date + time`, `End date + time`, `Theta 1 + dir + ref numbers`, `Theta 2 + dir + ref numbers`, `Theta 3 + dir + ref numbers`, (foreseen potentially added angle(s),) followed by 9 user defined parameter lines, followed by potentially added user defined parameter lines, `Number of file format`, `Number of data points`.
The version of the file format is set randomly to the high value 40 in order to exceed all ILL Cosmos versions.
For the ILL instruments D17 and FIGARO, obligatory header lines will be automatically filled by using the workspaces `Sample Logs` information which can be modified as shown in the `Usage`_.
The algorithm seeks to add the values for the following SampleLog entries: `instrument.name`, `user.namelocalcontact`, `title`, `start_time` and `end_time` which correspond to the header lines of names `Instrument`, `User-local contact`, `Title`, `Start date + time`, `End date + time`, respectively.
The options `WriteHeader`, `WriteResolution` and `Separator` do not have any effect and are not visible in the user interface.

TXT File Format
---------------

This file format (ANSTO Ascii format) writes four columns of data without any additional information.
If the resolution values are not present will be optionally computed, see `Computation of resolution values`_.
The options `LogList`, `WriteHeader`, `WriteResolution` and `Separator` do not have any effect and are not visible in the user interface.

DAT File Format
---------------

Stores first the number of lines followed by three columns of data.
The option `LogList`, `WriteResolution` and `Separator` do not have any effect and are not visible in the user interface.

Custom File Format (Empty Field)
--------------------------------

Enables a selection to write header lines via the option `WriteHeader` and the separator via the option `Separator` as well as the number of columns to write, i.e. three or four via the option `WriteResolution`.
The header follows the specification of the `MFT File format`_, if enabled.
Please consider to directly provide the desired file extension via the input `Filename`.
All options are taken into account and are visible in the user interface.

Usage
-----

**Example - Save a workspace in reflectometry ASCII format**

.. testcode:: SaveReflectometryAscii_general_usage

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
    file = os.path.join(os.path.expanduser("~"), "ws")

    # Add Sample Log entries
    # Add a Title entry:
    AddSampleLog(Workspace=ws, LogName='title', LogText='MyTest', LogType='String')
    # Add an entry called d as a Parameter (then, only eight not defined parameter lines remain):
    AddSampleLog(Workspace=ws, LogName='d', LogText='0.3', LogType='Number', LogUnit='mm', NumberType='Double')

    # Save with mft extension and using the option LogList: title will be added to a required header line and d will be additionally added
    # to the first parameter field.
    SaveReflectometryAscii(InputWorkspace=ws, Filename=file, LogList=['title', 'd'])

    if os.path.exists(file + ".mft"):
      myFile = open((file + ".mft"), 'r')
      print(myFile.read())

.. testoutput:: SaveReflectometryAscii_general_usage
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
   d : 0.29999999999999999 mm
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

.. testcleanup:: SaveReflectometryAscii_general_usage

   if os.path.exists(file + ".mft"):
     # Delete file
     os.remove(file + ".mft")

.. categories::

.. sourcelink::
