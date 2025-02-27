.. algorithm::

.. summary::

.. relatedalgorithms::

.. properties::

Description
-----------


POLDI is a pulse overlap diffractometer and therefore the arrival time of a neutron does not uniquely determine
the time-of-flight of the neutron (and hence the d-spacing in a given detector). Neutrons of different wavelength can
arrive at the same time having passed through the chopper in a different pulse/slit.
Indeed, neutrons corresponding to the same d-spacing are observed at many different arrival times in a given detector
due to the overlapping pulses measured.

.. figure:: /images/POLDI1p2_silicon_raw2D.jpeg
   :figwidth: 10 cm
   :align: right
   :alt: Raw POLDI 1.2 data (i.e. post 2D detector upgrade) for silicon powder standard.

   Raw POLDI 1.2 data (i.e. post 2D detector upgrade) for silicon powder standard.

PoldiAutoCorrelation is an algorithm that performs a type of average of the intensity over different arrival times from
each pulse corresponding to a given d-spacing for each detector. It produces a correlation spectrum, similar to the
the spectrum of a conventional TOF diffractometer.
Bragg reflections will appear in this correlation spectrum as peaks surrounded by a dip in the background.
The positions of the Bragg peaks can be determined to a high accuracy in the correlation spectrum and indexed
accordingly.

Note by convention the correlation spectrum is converted from d-spacing into momentum transfer.

Further details of the POLDI instrument and the reduction can be found in [1]_.

.. figure:: /images/PoldiAutoCorrelation_silicon.jpeg
   :figwidth: 10 cm
   :align: right
   :alt: Correlation spectrum of silicon powder standard.

    Correlation spectrum of silicon powder standard.

Usage
-----

Note POLDI 1.2 is still under development, the data are currently saved in ASCII format with no meta-data required
for the reduction. Currently there is a helper function ``load_poldi`` that reads the ASCII file, loads an instrument
definition and adds the appropriate meta-data to the workspace.

.. testcode:: POLDI_silicon_autocorr

    from mantid.simpleapi import *
    from mantid.api import FileFinder
    from plugins.algorithms.poldi_utils import load_poldi

    fpath_data = FileFinder.getFullPath("poldi_448x500_chopper5k_silicon.txt")
    fpath_idf = FileFinder.getFullPath("POLDI_Definition_448_calibrated.xml")

    # load the raw data
    ws = load_poldi(fpath_data, fpath_idf, chopper_speed=5000, t0=5.855e-02, t0_const=-9.00)
    print(f"The workspace contains {ws.getNumberHistograms()} spectra.")

    ws_corr = PoldiAutoCorrelation(InputWorkspace=ws, OutputWorkspace='ws_corr')
    print(f"The correlation spectrum has {ws_corr.blocksize()} bins.")

Output:

.. testoutput:: POLDI_silicon_autocorr

    The workspace contains 448 spectra.
    The correlation spectrum has 2460 bins.

References
----------

.. [1] Stuhr, U. (2005). Nuclear Instruments and Methods in Physics Research Section A, 545(1-2), 319-329.

.. categories::

.. sourcelink::
