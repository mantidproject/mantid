.. algorithm::

.. summary::

.. relatedalgorithms::

.. properties::

Description
-----------

This algorithms performs full treatment of :ref:`ILL's time-of-flight <DirectILL>` data recorded with the ILL instruments IN4, IN5, IN6, PANTHER, and SHARP.
This high level algorithm steers the reduction for each sample type and performs the full set of corrections for a given sample run, or set thereof;
measured at one initial energy and one or more temperatures (for powder) and one or more sample angles (for single crystal).

The sample measurement will be corrected for all the effects the user selects and the input is provided for, such as flat background, empty container subtraction, and vanadium normalisation.
The output is transformed to :math:`S(q,\omega)` space, and :math:`S(2\theta,\omega)`, and can be stored as .nxs and .nxspe files, if requested.

The algorithm is intended to be run multiple times, for each of the available processes (`Cadmium`, `Empty`, `Vanadium`, `Sample`) and each change of initial energy,
and sample geometry and material. Multiple temperatures can be reduced together.

After each execution, a report is printed at the notice level. It contains the numor that was reduced, or the first and last in case of a list, which input workspaces
(from `Cadmium`, `Empty`, `Vanadium`, `MaskWorkspace`) were used, if any, the incident energy, and the sample temperature(s).

ProcessAs
---------
Different input properties can be specified depending on the value of **ProcessAs**, as summarized in the table:

+------------------+------------------------------+------------------------------+
| ProcessAs        | Input Workspace Properties   | Other Input Properties       |
+==================+==============================+==============================+
| Cadmium          |                              |                              |
+------------------+------------------------------+------------------------------+
| Empty            |                              | * FlatBackgroundSource       |
|                  |                              | * FlatBackgroundScaling      |
|                  |                              | * FlatBkgAveragingWindow     |
|                  |                              | * GroupDetHorizontallyBy     |
|                  |                              | * GroupDetVerticallyBy       |
|                  |                              | * DetectorGrouping           |
|                  |                              | * GroupDetBy                 |
|                  |                              | * IncidentEnergyCalibration  |
|                  |                              | * ElasticChannel             |
|                  |                              | * IncidentEnergy             |
|                  |                              | * ElasticChannel             |
|                  |                              | * EPPCreationMethod          |
|                  |                              | * ElasticChannelIndex        |
+------------------+------------------------------+------------------------------+
| Vanadium         | * CadmiumWorkspace           | * all from Empty, and:       |
|                  | * EmptyContainerWorkspace    | * AbsorptionCorrection       |
|                  | * MaskWorkspace              | * SelfAttenuationMethod      |
|                  |                              | * SampleMaterial             |
|                  |                              | * SampleGeometry             |
|                  |                              | * ContainerMaterial          |
|                  |                              | * ContainerGeometry          |
|                  |                              | * EnergyExchangeBinning      |
|                  |                              | * MomentumTransferBinning    |
|                  |                              | * GroupingAngleStep          |
|                  |                              | * GroupingBehaviour          |
+------------------+------------------------------+------------------------------+
| Sample           | * CadmiumWorkspace           | * all from Vanadium, and     |
|                  | * EmptyContainerWorkspace    | * SampleAngleOffset          |
|                  | * VanadiumWorkspace          |                              |
|                  | * MaskWorkspace              |                              |
+------------------+------------------------------+------------------------------+

All the input workspace properties above are optional, unless bolded.
For example, if processing as sample, if an empty container and cadmium absorber inputs are specified, subtraction of these workspaces will be performed,
while if not, this step will be skipped.

On top of the input properties, there are also switches that control the workflow and which corrections are to be performed. For example, the sample
is going to be normalised to absolute units with vanadium if `AbsoluteUnitsNormalisation` is set to "Absolute Units ON". There is also a number of parameters
that allow creating bespoke masking, these include:

- `MaskWorkspace` - custom mask workspace
- `MaskedTubes` - list of tubes to be masked
- `MaskThresholdMin`, `MaskThresholdMax` - minimum and maximum threshold values of normalised counts to be masked
- `MaskedAngles` - range of 2theta angles to be masked
- `MaskWithVanadium` - whether to use Vanadium-derived diagnostics to mask data


ReductionType
-------------

There are two supported reduction types available: `Powder` and `SingleCrystal`. The choice impacts the reduction workflow of the `Sample` process, as can
be seen in the diagrams below. The `SingleCrystal` reduction exits the reduction earlier and saves the output to be processed externally to Mantid, while
`Powder` continues to the call to :ref:`DirectILLReduction <algm-DirectILLReduction>`, and then saves its output.


Caching with ADS
----------------

This algorithm cleans-up the intermediate workspaces after execution if `ClearCache` property is checked (`True` by default). It is recommended to keep it checked due
to large memory consumption coming from keeping rawdata.

Default naming schemes are imposed to ensure smooth communication of workspace contents. While user can specify the name for the output :ref:`WorkspaceGroup <WorkspaceGroup>`,
the names of contents will consist of the name of the group as a prefix, the numor of the rawdata (or first rawdata in case of merging), initial energy, and temperature (when
`ReductionType` is `Powder`).


Saving output
-------------

When `SaveOutput` property is checked, the output workspaces are saved in the default save directory. Depending on the `ReductionType`, and contents of the workspace saved,
the output is either a .nxs or a .nxspe file. For `SingleCrystal` reduction type, the output of the rebinning is saved as .nxspe files with the `Psi` parameter coming from
a sum of the relevant sample log and user-defined `SampleAngleOffset` property. For `Powder`, :math:`S (2\theta, \omega)` output is saved as .nxspe while the rest is saved
as regular .nxs.


Workflows
---------

Empty container
###############

.. diagram:: DirectILLAutoProcess-v1_empty_wkflw.dot

Vanadium
########

.. diagram:: DirectILLAutoProcess-v1_vanadium_wkflw.dot

Sample, powder
##############

.. diagram:: DirectILLAutoProcess-v1_sample_powder_wkflw.dot

Sample, single crystal
######################

.. diagram:: DirectILLAutoProcess-v1_sample_sx_wkflw.dot

.. include:: ../usagedata-note.txt

**Example - full treatment of a sample at 2 different temperatures in IN4**

.. testsetup:: ExDirectILLAutoProcessPowder

    config.setFacility('ILL')
    config.appendDataSearchSubDir('ILL/IN4/')

.. testcode:: ExDirectILLAutoProcessPowder

    vanadium_runs = 'ILL/IN4/085801-085802'
    sample_runs = 'ILL/IN4/087294+087295.nxs,ILL/IN4/087283-087290.nxs'
    container_runs = 'ILL/IN4/087306-087309.nxs,ILL/IN4/087311-087314.nxs'

    vanadium_ws = 'vanadium_auto'
    container_ws = 'container'
    sample_ws = 'sample'

    # Sample self-shielding and container subtraction.
    geometry = {
        'Shape': 'HollowCylinder',
        'Height': 4.0,
        'InnerRadius': 1.9,
        'OuterRadius': 2.0,
        'Center': [0.0, 0.0, 0.0]
    }
    material = {
        'ChemicalFormula': 'Cd S',
        'SampleNumberDensity': 0.01
    }
    Ei = 8.804337831263577

    DirectILLAutoProcess(
        Runs=vanadium_runs,
        OutputWorkspace=vanadium_ws,
        ProcessAs='Vanadium',
        ReductionType='Powder',
        FlatBkg = 'Flat Bkg ON',
        ElasticChannel='Elastic Channel AUTO',
        EPPCreationMethod='Fit EPP'
    )

    DirectILLAutoProcess(
        Runs=container_runs,
	OutputWorkspace=container_ws,
        ProcessAs='Empty',
        ReductionType='Powder',
        IncidentEnergyCalibration="Energy Calibration ON",
        IncidentEnergy=Ei
    )

    # Need to interpolate container to 50K
    T0 = 1.5
    T1 = 100.0
    DT = T1 - T0
    Ts = 50.0 # Target T
    RebinToWorkspace(
        WorkspaceToRebin='container_087311_Ei9meV_T100.0K',
        WorkspaceToMatch='container_087306_Ei9meV_T1.5K',
        OutputWorkspace='container_087311_Ei9meV_T100.0K'
    )
    container_Ei9meV_50K = (T1 - Ts) / DT * mtd['container_087306_Ei9meV_T1.5K'] + (Ts - T0) / DT * mtd['container_087311_Ei9meV_T100.0K']
    mtd[container_ws].add('container_Ei9meV_50K')

    DirectILLAutoProcess(
        Runs=sample_runs,
        OutputWorkspace=sample_ws,
        ProcessAs='Sample',
        ReductionType='Powder',
        VanadiumWorkspace=vanadium_ws,
        EmptyContainerWorkspace=container_ws,
        IncidentEnergyCalibration="Energy Calibration ON",
        IncidentEnergy=Ei,
        SampleMaterial=material,
        SampleGeometry=geometry,
        SaveOutput=False,
        ClearCache=True,
    )

    outputs = ['sample_SofQW_087294_Ei9meV_T1.5K', 'sample_SofQW_087283_Ei9meV_T50.0K']
    for output in outputs:
        SofQW = mtd[output]
        qAxis = SofQW.readX(0)  # Vertical axis
        eAxis = SofQW.getAxis(1)  # Horizontal axis
        print('{}: Q range: {:.3}...{:.3}A; W range {:.3}...{:.3}meV'.format(
            output, qAxis[0], qAxis[-1], eAxis.getMin(), eAxis.getMax()))

Output:

.. testoutput:: ExDirectILLAutoProcessPowder

    sample_SofQW_087294_Ei9meV_T1.5K: Q range: 0.0...9.21A; W range -97.0...7.62meV
    sample_SofQW_087283_Ei9meV_T50.0K: Q range: 0.0...9.19A; W range -96.6...7.62meV

.. testcleanup:: ExDirectILLAutoProcessPowder

    mtd.clear()
    import os
    to_remove = ["vanadium_auto_SofQW_085801_Ei9meV_T20.0K.nxs",
                 "vanadium_auto_SofTW_085801_Ei9meV_T20.0K.nxspe",
                 "vanadium_auto_diag_085801_Ei9meV_T20.0K",
                 "vanadium_auto_integral_085801_Ei9meV_T20.0K.nxs",
                 "container_087306_Ei9meV_T1.5K.nxs",
                 "container_087311_Ei9meV_T100.0K.nxs"]
    for ftr in to_remove:
        os.remove(ftr)

.. categories::

.. sourcelink::
