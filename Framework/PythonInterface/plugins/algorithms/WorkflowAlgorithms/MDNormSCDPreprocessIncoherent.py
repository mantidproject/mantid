from __future__ import (absolute_import, division, print_function)

from mantid.api import (DataProcessorAlgorithm, mtd, AlgorithmFactory,
                        FileProperty, FileAction,
                        MultipleFileProperty, WorkspaceProperty,
                        PropertyMode)
from mantid.simpleapi import (ConvertUnits, CropWorkspace,
                              LoadInstrument, Minus, Load,
                              DeleteWorkspace, LoadIsawDetCal,
                              LoadMask, GroupDetectors, Rebin,
                              MaskDetectors, SumSpectra, SortEvents,
                              IntegrateFlux, AnvredCorrection,
                              NormaliseByCurrent)
from mantid.kernel import Direction, Property, FloatMandatoryValidator, VisibleWhenProperty, PropertyCriterion


class MDNormSCDPreprocessIncoherent(DataProcessorAlgorithm):

    def category(self):
        return "MDAlgorithms\\Normalisation"

    def seeAlso(self):
        return [ "MDNormSCD","MDNormDirectSC" ]

    def name(self):
        return "MDNormSCDPreprocessIncoherent"

    def summary(self):
        return "Creates the Solid Angle and Flux workspace from an incoherent scatterer for MDNormSCD"

    def PyInit(self):
        # files to reduce
        self.declareProperty(MultipleFileProperty(name="Filename",
                                                  extensions=["_event.nxs", ".nxs.h5", ".nxs"]),
                             "Files to combine in reduction")

        # background
        self.declareProperty(FileProperty(name="Background",defaultValue="",action=FileAction.OptionalLoad,
                                          extensions=["_event.nxs", ".nxs.h5", ".nxs"]),
                             "Background run")
        self.declareProperty("BackgroundScale", 1.0,
                             doc="The background will be scaled by this number before being subtracted.")

        # Filter by TOF
        self.copyProperties('LoadEventNexus', ['FilterByTofMin', 'FilterByTofMax'])

        # Cropping
        self.declareProperty('MomentumMin', Property.EMPTY_DBL, doc="Minimum value in momentum.", validator=FloatMandatoryValidator())
        self.declareProperty('MomentumMax', Property.EMPTY_DBL, doc="Maximum value in momentum.", validator=FloatMandatoryValidator())

        # Grouping
        self.declareProperty(FileProperty(name="GroupingFile",defaultValue="",action=FileAction.OptionalLoad,
                                          extensions=[".xml",".map"]),
                             "A file that consists of lists of spectra numbers to group. See :ref:`GroupDetectors <algm-GroupDetectors>`")

        # Corrections
        self.declareProperty('NormaliseByCurrent', True, "Normalise the Solid Angle workspace by the proton charge.")
        self.declareProperty(FileProperty(name="LoadInstrument",defaultValue="",action=FileAction.OptionalLoad,
                                          extensions=[".xml"]),
                             "Load a different instrument IDF onto the data from a file. See :ref:`LoadInstrument <algm-LoadInstrument>`")
        self.declareProperty(FileProperty(name="DetCal",defaultValue="",action=FileAction.OptionalLoad,
                                          extensions=[".detcal"]),
                             "Load an ISAW DetCal calibration onto the data from a file. See :ref:`LoadIsawDetCal <algm-LoadIsawDetCal>`")
        self.declareProperty(FileProperty(name="MaskFile",defaultValue="",action=FileAction.OptionalLoad,
                                          extensions=[".xml",".msk"]),
                             "Masking file for masking. Supported file format is XML and ISIS ASCII. See :ref:`LoadMask <algm-LoadMask>`")
        # Anvred
        self.declareProperty('SphericalAbsorptionCorrection', False,
                             "Apply Spherical Absorption correction using :ref:`AnvredCorrection <algm-AnvredCorrection>`")
        condition = VisibleWhenProperty("SphericalAbsorptionCorrection", PropertyCriterion.IsNotDefault)
        self.copyProperties('AnvredCorrection', ['LinearScatteringCoef','LinearAbsorptionCoef','Radius'])
        self.setPropertySettings("LinearScatteringCoef", condition)
        self.setPropertySettings("LinearAbsorptionCoef", condition)
        self.setPropertySettings("Radius", condition)

        # Output
        self.declareProperty(WorkspaceProperty("SolidAngleOutputWorkspace", "",
                                               optional=PropertyMode.Mandatory,
                                               direction=Direction.Output),
                             "Output Workspace for Solid Angle")

        self.declareProperty(WorkspaceProperty("FluxOutputWorkspace", "",
                                               optional=PropertyMode.Mandatory,
                                               direction=Direction.Output),
                             "Output Workspace for Flux")

        # Background
        self.setPropertyGroup("Background","Background")
        self.setPropertyGroup("BackgroundScale","Background")

        # Corrections
        self.setPropertyGroup("NormaliseByCurrent","Corrections")
        self.setPropertyGroup("LoadInstrument","Corrections")
        self.setPropertyGroup("DetCal","Corrections")
        self.setPropertyGroup("MaskFile","Corrections")
        self.setPropertyGroup("SphericalAbsorptionCorrection","Corrections")
        self.setPropertyGroup("LinearScatteringCoef","Corrections")
        self.setPropertyGroup("LinearAbsorptionCoef","Corrections")
        self.setPropertyGroup("Radius","Corrections")

    def PyExec(self):
        _background = bool(self.getProperty("Background").value)
        _load_inst = bool(self.getProperty("LoadInstrument").value)
        _norm_current = bool(self.getProperty("NormaliseByCurrent").value)
        _detcal = bool(self.getProperty("DetCal").value)
        _masking = bool(self.getProperty("MaskFile").value)
        _grouping = bool(self.getProperty("GroupingFile").value)
        _anvred = bool(self.getProperty("SphericalAbsorptionCorrection").value)
        _SA_name = self.getPropertyValue("SolidAngleOutputWorkspace")
        _Flux_name = self.getPropertyValue("FluxOutputWorkspace")

        XMin = self.getProperty("MomentumMin").value
        XMax = self.getProperty("MomentumMax").value
        rebin_param = ','.join([str(XMin),str(XMax),str(XMax)])

        Load(Filename=self.getPropertyValue("Filename"),
             OutputWorkspace='__van',
             FilterByTofMin=self.getProperty("FilterByTofMin").value,
             FilterByTofMax=self.getProperty("FilterByTofMax").value)

        if _norm_current:
            NormaliseByCurrent(InputWorkspace='__van',
                               OutputWorkspace='__van')

        if _background:
            Load(Filename=self.getProperty("Background").value,
                 OutputWorkspace='__bkg',
                 FilterByTofMin=self.getProperty("FilterByTofMin").value,
                 FilterByTofMax=self.getProperty("FilterByTofMax").value)
            if _norm_current:
                NormaliseByCurrent(InputWorkspace='__bkg',
                                   OutputWorkspace='__bkg')
            else:
                pc_van = mtd['__van'].run().getProtonCharge()
                pc_bkg = mtd['__bkg'].run().getProtonCharge()
                mtd['__bkg'] *= pc_van/pc_bkg
            mtd['__bkg'] *= self.getProperty('BackgroundScale').value
            Minus(LHSWorkspace='__van', RHSWorkspace='__bkg', OutputWorkspace='__van')
            DeleteWorkspace('__bkg')

        if _load_inst:
            LoadInstrument(Workspace='__van', Filename=self.getProperty("LoadInstrument").value, RewriteSpectraMap=False)
        if _detcal:
            LoadIsawDetCal(InputWorkspace='__van', Filename=self.getProperty("DetCal").value)

        if _masking:
            LoadMask(Instrument=mtd['__van'].getInstrument().getName(),
                     InputFile=self.getProperty("MaskFile").value,
                     OutputWorkspace='__mask')
            MaskDetectors(Workspace='__van',MaskedWorkspace='__mask')
            DeleteWorkspace('__mask')

        ConvertUnits(InputWorkspace='__van',OutputWorkspace='__van',Target='Momentum')
        Rebin(InputWorkspace='__van',OutputWorkspace='__van',Params=rebin_param)
        CropWorkspace(InputWorkspace='__van',OutputWorkspace='__van',XMin=XMin,XMax=XMax)

        if _anvred:
            AnvredCorrection(InputWorkspace='__van', OutputWorkspace='__van',
                             LinearScatteringCoef=self.getProperty("LinearScatteringCoef").value,
                             LinearAbsorptionCoef=self.getProperty("LinearAbsorptionCoef").value,
                             Radius=self.getProperty("Radius").value,
                             OnlySphericalAbsorption='1', PowerLambda ='0')

        # Create solid angle
        Rebin(InputWorkspace='__van',
              OutputWorkspace=_SA_name,
              Params=rebin_param,PreserveEvents=False)

        # Create flux
        if _grouping:
            GroupDetectors(InputWorkspace='__van', OutputWorkspace='__van', MapFile=self.getProperty("GroupingFile").value)
        else:
            SumSpectra(InputWorkspace='__van', OutputWorkspace='__van')

        Rebin(InputWorkspace='__van',OutputWorkspace='__van',Params=rebin_param)
        flux = mtd['__van']
        for i in range(flux.getNumberHistograms()):
            el=flux.getSpectrum(i)
            if flux.readY(i)[0] > 0:
                el.divide(flux.readY(i)[0],flux.readE(i)[0])
        SortEvents(InputWorkspace='__van', SortBy="X Value")
        IntegrateFlux(InputWorkspace='__van', OutputWorkspace=_Flux_name, NPoints=10000)
        DeleteWorkspace('__van')

        self.setProperty("SolidAngleOutputWorkspace", mtd[_SA_name])
        self.setProperty("FluxOutputWorkspace", mtd[_Flux_name])


AlgorithmFactory.subscribe(MDNormSCDPreprocessIncoherent)
