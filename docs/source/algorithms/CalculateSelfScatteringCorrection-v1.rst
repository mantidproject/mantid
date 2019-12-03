.. algorithm::

.. summary::

.. relatedalgorithms::

.. properties::

Description
-----------

This is a workflow algorithm that calculates the placzek self scattering
factor focused by into detector banks. This is done by executing several
sub-algorithms as listed below.

#. :ref:`algm-SetSample` Sets sample data for the run that is to be corrected to the raw workspace.
#. :ref:`algm-ExtractSpectra` Extracts the monitor spectrum closest to the sample (incident spectrum).
#. :ref:`algm-ConvertUnits` Converts incident spectrum to wavelength.
#. :ref:`algm-FitIncidentSpectrum` Fit a curve to the incident spectrum.
#. :ref:`algm-CalculatePlaczekSelfScattering` Calculate the Placzek self scattering factor for each pixel.
#. :ref:`algm-LoadCalFile` Loads the detector calibration.
#. :ref:`algm-DiffractionFocussing` Focus the Placzek self scattering factor into detector banks.
#. :ref:`algm-CreateWorkspace` Create a workspace containing the number of pixels in each detector bank.
#. :ref:`algm-Divide` Normalize the Placzek correction by pixel number in bank
#. :ref:`algm-ConvertToDistribution` Change the workspace into a format that can be subtracted.
#. :ref:`algm-ConvertUnits` Converts correction into MomentumTransfer.

Workflow
########

.. diagram:: CalculateSelfScatteringCorrection-v1_wkflw.dot
