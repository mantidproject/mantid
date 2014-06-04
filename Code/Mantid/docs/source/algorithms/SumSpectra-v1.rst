.. algorithm::

.. summary::

.. alias::

.. properties::

Description
-----------

Takes a workspace as input and sums all of the spectra within it
maintaining the existing bin structure and units. Any masked spectra are
ignored. The result is stored as a new workspace containing a single
spectra.

The algorithm adds to the **OutputWorkspace** three additional
properties (Log values). The properties (Log) names are:
**"NumAllSpectra"**, **"NumMaskSpectra"** and **"NumZeroSpectra"**,
where:

| ``  NumAllSpectra  -- is the number of spectra contributed to the sum``
| ``  NumMaskSpectra -- the spectra dropped from the summations because they are masked. ``
| ``                    If monitors (``\ **``IncludeMonitors``**\ ``=false) are not included in the summation,``
| ``                    they are not counted here. ``
| ``  NumZeroSpectra -- number of zero bins in histogram workspace or empty spectra for event workspace. ``
| ``                    These spectra are dropped from the summation of histogram workspace ``
| ``                    when ``\ **``WeightedSum``**\ `` property is set to True.``

Assuming **pWS** is the output workspace handle, from Python these
properties can be accessed by the code:

| ``   nSpectra       = pWS.getRun().getLogData("NumAllSpectra").value``
| ``   nMaskedSpectra = pWS.getRun().getLogData("NumMaskSpectra").value ``
| ``   nZeroSpectra   = pWS.getRun().getLogData("NumZeroSpectra").value``

It is also available in stats property obtained by qtiGenie function
avrg\_spectra

| ``  (avrg,stats) = avrg_spectra(Input_workspace)``
| ``   stats==[nSpectra,nMaskedSpectra,nZeroSpectra]``

From C++ they can be reached as strings by the code:

| ``     std::string rez=pWS->run().getLogData("NumAllSpectra")->value();``
| ``     std::string rez=pWS->run().getLogData("NumMaskSpectra")->value();``
| ``     std::string rez=pWS->run().getLogData("NumZeroSpectra")->value();``

.. categories::
