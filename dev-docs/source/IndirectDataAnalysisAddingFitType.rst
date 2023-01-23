.. _IDA-AddingFitType-ref:

Adding a new fitting function to IDA
====================================

The tabs in Indirect Data Analysis use each fit spectra to a predefined list of functions, adding new functions to the
list of available ones can seem daunting at first but it can be done by following some steps.

FqFit
-----

In the `FQFitConstants.h` file there are sets of maps of {"Function name", "FunctionString"}, e.g.
`{std::string("TeixeiraWater"), std::string("name=TeixeiraWater, Tau=1, L=1.5, constraints=(Tau>0, L>0)")}`. To add
a new function to FqFit you simply need to add it to this list of available functions. Currently we divide up the
functions into Width, EISF, and All with the intent that the list of functions would change depending on what data is
loaded into the tab, however as it is not yet implemented for the workspace input it uses the list for All.

MSDFit
------

The fit function strings in MSDFit are stored in the `IndirectDataAnalysisMSDFitTab.cpp` file in `msdFunctionStrings`,
To add a function you just need to add it to this map and add the function to the others in `createParameterEstimation`
e.e. `parameterEstimation.addParameterEstimationFunction(MSDGAUSSFUNC, estimateMsd);`

IqtFit
------

The fitting functions in IqtFit are hard coded into the template browser and is not expected for other functions to be
added to it.

ConvFit
-------

ConvFit has the most involved process of adding a function to it's library and it needs to be added in its entirety.

Firstly, In `ConvTypes.h` add the function name to the enumerated list FitType e.g. `TeixeiraWater`, and in the paramID
enum add an entry for each parameter in the function along with a uniqure prefix denoting which function they are for.
e.g. `TW_HEIGHT, TW_DIFFCOEFF, TW_TAU, TW_CENTRE`. Make sure to preserve the ordering of the current enum and
add the parameters of the function along each other.

Secondly, in ConvTypes.cpp add QDependency for the function as either true or false in the `FitTypeQDepends` map based on if
the function is Q dependant.

.. code-block:: cpp

   {FitType::TeixeiraWater, true},

Then, in the FitTypeEnumToString and FitTypeStringToEnum add the mapping of FitType from the header onto the function name

.. code-block:: cpp

    {FitType::TeixeiraWater, "TeixeiraWaterSQE"}

    {"TeixeiraWaterSQE", FitType::TeixeiraWater}

After that, In the g_paramName add ParamID from the header and parameter names for each parameter in the function. The order of these
is important so add them in the same order as they appear in the header.

.. code-block:: cpp

    {ParamID::TW_HEIGHT, "Height"},
    {ParamID::TW_DIFFCOEFF, "DiffCoeff"},
    {ParamID::TW_TAU, "Tau"},
    {ParamID::TW_CENTRE, "Centre"},

Moreover, to tie it all together add param ranges to g_typeMap in the form
`{FitType, {"Function name displayed in tab", "FunctionName", {ParamID::first, ParamID::last}}}` this allows the template
to cnostruct a function out of the related parameters. There are several places where this can be added, those being FitType,
LorentzianType, and BackgroundType. ConvFit can run fits with one of each Fit, Lorentzian and Background but only one of each.

.. code-block:: cpp

    {FitType::TeixeiraWater, {"Teixeira Water", "TeixeiraWaterSQE", {ParamID::TW_HEIGHT, ParamID::TW_CENTRE}}},

Finally, In IndirectDataAnalysisConvFitTab in setupFitTab add to m_fitStrings with fit function name and shorterned key,
    this key will be used in the output workspace from the fit.

.. code-block:: cpp

   m_fitStrings["TeixeiraWaterSQE"] = "TxWater";

In ConvFunctionModel add the build function string function

.. code-block:: cpp

    std::string ConvFunctionModel::buildTeixeiraFunctionString() const {
      return "name=TeixeiraWaterSQE, Height=1, DiffCoeff=2.3, Tau=1.25, Centre=0, "
      "constraints=(Height>0, DiffCoeff>0, Tau>0)";
    }

and then add else if to buildPeaksFunctionString and buildLorentzianPeaksString/buildFitTypeString

.. code-block:: cpp

  else if (m_fitType == FitType::TeixeiraWater) {
    functions.append(buildTeixeiraFunctionString());
  }