.. algorithm::

.. summary::

.. relatedalgorithms::

.. properties::

Description
-----------

Only for ILL usage. In the future, ILL plans to load data in raw format (no units used). The X-axis represent the time *channel number*.
This algorithm converts the *channel number* to time of flight using the distances source-sample-detectors defined in the IDF. In addition, the user has to input the fields:

- **ListOfSpectraIndices** - Spectrum or list of spectra to look for the elastic peak. Note that the spectra chosen must be at the same distance.
- **ListOfChannelIndices** - Elastic peak channels - For the selected spetra above, the list/range of channels to look for the elastic peak (e.g. range).

Note that the Input Workspace must have the following properties:

-  ``wavelength``
-  ``channel_width``

Instead of using the fields above (ListOfSpectraIndices and ListOfChannelIndices), the user can specify the Elastic Peak Position (channel number) and the respective spectrum, using the following fields:

- **ElasticPeakPosition**
- **ElasticPeakPositionSpectrum**


.. categories::

.. sourcelink::
