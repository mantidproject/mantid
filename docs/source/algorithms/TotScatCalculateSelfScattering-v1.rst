.. algorithm::

.. summary::

.. relatedalgorithms::

.. properties::

Description
-----------

This is a workflow algorithm that calculates the placzek self scattering
factor focused into detector banks. This is done by executing several
sub-algorithms as listed below.

#. :ref:`algm-SetSample` Sets sample data for the run that is to be corrected to the raw workspace.
#. :ref:`algm-ExtractSpectra` Extracts the monitor spectrum closest to the sample (incident spectrum).
#. :ref:`algm-ConvertUnits` Converts incident spectrum to wavelength.
#. :ref:`algm-FitIncidentSpectrum` Fit a curve to the incident spectrum.
#. :ref:`algm-CalculatePlaczek` Calculate the Placzek self scattering factor for each pixel.
#. :ref:`algm-ConvertUnits` Convert the Placzek correction into MomentumTransfer
#. :ref:`algm-Rebin` Rebin correction before GroupDetectors.
#. :ref:`algm-LoadCalFile` Loads the detector calibration.
#. :ref:`algm-GroupDetectors` Group the Placzek self scattering factor into detector banks.
#. :ref:`algm-CreateWorkspace` Create a workspace containing the number of pixels in each detector bank.
#. :ref:`algm-Divide` Normalize the Placzek correction by pixel number in bank

Workflow
########

.. diagram:: TotScatCalculateSelfScattering-v1_wkflw.dot

.. categories::

.. sourcelink::
