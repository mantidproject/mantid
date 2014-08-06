.. algorithm::

.. warning::

    This algorithm is currently under review and may change or appear with a different name in future releases. The documentation may be outdated.

.. summary::

.. alias::

.. properties::

Description
-----------

PoldiProjectRun algorithm is used to analyze a bunch of POLDI raw data
files, following a standard POLDI analysis process. This algorithm take
as parameter a tableMatrix with a list of the sample to analyze, and for
each sample a bunch of setup information for the different algorithms
(such as the data file path, etc...).

This tableWorkspace can be built easily using the two algorithms
:ref:`algm-PoldiProjectAddFile` and
:ref:`algm-PoldiProjectAddDir`, which will create and/or
fill properly a targeted tableWorkspace. The needed columns and there
content are describe in the following `Data
Manager <PoldiProjectRun#Data_Manager>`__ paragraph.

The algorithm is used the classical way. Only one parameter is
compulsory.

.. code-block:: python

    OutputWorkspace = PoldiProjectRun(InputWorkspace=sample_manager_ws)

Data are processed alone, or grouped together. For each acquisition
file, setup information have to be loaded. During the data treatment
process, transitional workspace are created.

In a close future, it will possible to share different workspace between
data-file: for example when one knows that some acquisitions should be
strictly the same, auto-correlation and peak detection could be done
only one for all the data.

Data manager
############

A MatrixWorkspace is created to store all the information about
data-files and the future workspace needed during the analysis. The
stored information are:

-  spl Name - name of the sample, extract from the sample file name, without the extension
-  year - year of the acquisition
-  number - id number of the acquisition
-  data file - full path of the data file
-  spl log - name of the MatrixWorkspace where the data log are loaded
-  spl corr - name of the MatrixWorkspace where the correlated spectra is loaded
-  spl dead wires - name of the MatrixWorkspace where the dead wires are loaded
-  spl peak - name of the MatrixWorkspace where the detected peak information are stored

POLDI setup manager
###################

For each acquisition file, the IDF are loaded:

-  Instrument Definition files - The POLDI instrument geometry.
-  Instrument Parameters files - The setup parameters for the data, at t he time of the acquisition.

The POLDI setup informations can be shared between acquisition obtained
during the same beam-time. While loading each instrument files, the
different POLDI configurations used are stored in a MatrixWorkspace
(most often, there is only one per year), with an example of data. The
needed POLDI setup informations will then be extracted from the IDF of
each of these example sample.

Therefore each POLDI setup are loaded only once and shared between the
different data files.

Analysis steps
##############

Loading the data
################

Each data-file is loaded on a 2DWorkspace. The associated log and setup
information are loaded in dedicated workspace as specified in the
sample-manager TableWorkspace.

:ref:`algm-LoadSINQFile`

The raw data are loaded in a 2DWorkspace, using the generic file-loader
for SINQ data, given the instrument name *POLDI* as parameter.

.. code-block:: python

    LoadSINQFile(Instrument      = "POLDI",
                 Filename        = sample_file_path,
                 OutputWorkspace = sample_name)

:ref:`algm-PoldiLoadLog`

The associated *logs* informations are extracted from the *hdf* raw data
file, an store in a dedicated MatrixWorkspace. A dictionary file
contains the set of key/path to extract and store all the needed
information. More specifically, the acquisition starting time is
extracted and store in the sample WS to initialize the *run\_start*
variable.

.. code-block:: python

    PoldiLoadLog(InputWorkspace = sample_output_ws,
                 Filename       = sample_file_path,
                 Dictionary     = poldi_dictionnary_file_path,
                 PoldiLog       = sample_log_ws)

:ref:`algm-LoadInstrument`

For each raw data WS, the corresponding IDF is loaded, based on the
acquisition starting time.

.. code-block:: python

   LoadInstrument(Workspace         = sample_output_ws,
                  InstrumentName    = "Poldi",
                  RewriteSpectraMap = True)

:ref:`algm-PoldiRemoveDeadWires`

Some wires are permanently dead and should not be taken into account.
They are listed in the IDF of a given setup (IPP). Some others wires
should not be used, because they seem untrustable (dead wires, hot
wires, random behavior,...). These wires are detected by successive
comparison with there neighbors: intensity from two successive wires
should not differ more than *BadWiresThreshold*\ (\*100)%. One by one,
the most deviant wires are checks and removed until they all fit the
condition.

.. code-block:: python

   PoldiRemoveDeadWires(InputWorkspace      = sample_output_ws,
                        RemoveExcludedWires = True,
                        AutoRemoveBadWires  = True,
                        BadWiresThreshold   = BadWiresThreshold,
                        PoldiDeadWires      = sample_dead_wires_ws)

Loading POLDI parameters
########################

While loading the data, the different needed setup have been store in a
dedicated workspace.

they are now all extracted, using an example sample for each of them.

:ref:`algm-PoldiLoadChopperSlits`

The chopper configuration is loaded in a dedicated Workspace, one per
*Poldi IPP* setup detected.

.. code-block:: python

   PoldiLoadChopperSlits(InputWorkspace    = ex_of_sample_ws,
                         PoldiChopperSlits = ipp_chopper_slits)

:ref:`algm-PoldiLoadSpectra`

The characteristic Poldi spectra (*Intensity=f(wavelength)*) is
extracted from each IDF.

.. code-block:: python

   PoldiLoadSpectra(InputWorkspace = ex_of_sample_ws,
                    PoldiSpectra   = ipp_Poldi_spectra)

:ref:`algm-PoldiLoadIPP`

Local setup information (such as the detector position, chopper offset,
etc...) are extracted and stores in a dedicated workspace.

.. code-block:: python

   PoldiLoadIPP(InputWorkspace = ex_of_sample_ws,
                PoldiIPP       = ipp_ipp_data)

Pre-analyzing data
##################

In order to setup the 2D fit to analyze the data, some information need
to be extracted from the file, such as an idea of the peaks position.
This is done using an autocorrelation function, following by a peak
detection algorithm.

The process has been cut in different algorithm in order to give the
possibility to change/improve/modify each steps. For example, the peak
detection process can be based on some previous results to not start
from scratch, or given the sample crystal structure/symetries/space
group...

:ref:`algm-PoldiAutoCorrelation`

Almost all the previous loaded workspace are used by this algorithm.
From the sample manager workspace, and the Poldi setup workspace, all
the targeted workspace can be found and given as parameters to the
algorithm. The auto-correlated graph is store in a dedicated workspace,
on row (0).

:ref:`algm-PoldiPeakDetection`

The previous autocorrelation function is analyzed to detected possible
peaks. The found peak are stored in a dedicated workspace, and added to
the previously created *sample\_correlated\_ws*: on row (1) the detected
peak, on row (2) the fitted peak.


.. code-block:: python

    PoldiPeakDetection(InputWorkspace         = sample_correlated_ws,
                       PeakDetectionThreshold = PeakDetectionThreshold,
                       OutputWorkspace        = sample_peak_ws)

.. categories::
