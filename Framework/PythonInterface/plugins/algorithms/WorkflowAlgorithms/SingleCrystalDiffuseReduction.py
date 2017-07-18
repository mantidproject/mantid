from __future__ import (absolute_import, division, print_function)

from mantid.api import (DataProcessorAlgorithm, mtd, AlgorithmFactory,
                        FileProperty, FileAction,
                        MultipleFileProperty, WorkspaceProperty,
                        PropertyMode, Progress)
from mantid.simpleapi import (LoadIsawUB, MaskDetectors, ConvertUnits,
                              CropWorkspace, LoadInstrument,
                              SetGoniometer, SetUB, ConvertToMD,
                              MDNormSCD, DivideMD, MinusMD, Load,
                              DeleteWorkspace,
                              CreateSingleValuedWorkspace, LoadNexus,
                              MultiplyMD, LoadIsawDetCal, MaskBTP)
from mantid.geometry import SpaceGroupFactory, SymmetryOperationFactory
from mantid.kernel import VisibleWhenProperty, PropertyCriterion, FloatArrayLengthValidator, FloatArrayProperty, Direction
from mantid import logger
import numpy as np


class SingleCrystalDiffuseReduction(DataProcessorAlgorithm):
    temp_workspace_list = ['__sa', '__flux', '__run', '__md', '__data', '__norm',
                           '__bkg', '__bkg_md', '__bkg_data', '__bkg_norm', '__scaled_background',
                           'PreprocessedDetectorsWS']

    def category(self):
        return "Diffraction\\Reduction"

    def name(self):
        return "SingleCrystalDiffuseReduction"

    def summary(self):
        return "Single Crystal Diffuse Scattering Reduction, normalisation, symmetry and background substraction"

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

        # Vanadium SA and flux
        self.declareProperty(FileProperty(name="SolidAngle",defaultValue="",action=FileAction.Load,
                                          extensions=[".nxs"]),
                             doc="An input workspace containing momentum integrated vanadium (a measure"
                             "of the solid angle). See :ref:`MDnormSCD <algm-MDnormSCD>` for details")
        self.declareProperty(FileProperty(name="Flux",defaultValue="",action=FileAction.Load,
                                          extensions=[".nxs"]),
                             "An input workspace containing momentum dependent flux. See :ref:`MDnormSCD <algm-MDnormSCD>` for details")

        # UBMatrix
        self.declareProperty(FileProperty(name="UBMatrix",defaultValue="",action=FileAction.Load,
                                          extensions=[".mat", ".ub", ".txt"]),
                             doc="Path to an ISAW-style UB matrix text file. See :ref:`LoadIsawUB <algm-LoadIsawUB>`")
        # Goniometer
        self.declareProperty('SetGoniometer', False, "Set which Goniometer to use. See :ref:`SetGoniometer <algm-SetGoniometer>`")
        condition = VisibleWhenProperty("SetGoniometer", PropertyCriterion.IsNotDefault)
        self.copyProperties('SetGoniometer', ['Goniometers', 'Axis0', 'Axis1', 'Axis2'])
        self.setPropertySettings("Goniometers", condition)
        self.setPropertySettings('Axis0', condition)
        self.setPropertySettings('Axis1', condition)
        self.setPropertySettings('Axis2', condition)

        # Corrections
        self.declareProperty(FileProperty(name="LoadInstrument",defaultValue="",action=FileAction.OptionalLoad,
                                          extensions=[".xml"]),
                             "Load a different instrument IDF onto the data from a file. See :ref:`LoadInstrument <algm-LoadInstrument>`")
        self.declareProperty(FileProperty(name="DetCal",defaultValue="",action=FileAction.OptionalLoad,
                                          extensions=[".detcal"]),
                             "Load an ISAW DetCal calibration onto the data from a file. See :ref:`LoadIsawDetCal <algm-LoadIsawDetCal>`")

        self.copyProperties('MaskBTP', ['Bank', 'Tube', 'Pixel'])

        # SymmetryOps, name, group unmber or list symmetries
        self.declareProperty("SymmetryOps", "",
                             "If specified the symmetry will be applied, can be space group name or number, or list individual symmetries.")

        # Binning output
        self.copyProperties('ConvertToMD', ['Uproj', 'Vproj', 'Wproj'])
        self.declareProperty(FloatArrayProperty("BinningDim0", [-5.05,5.05,101], FloatArrayLengthValidator(3), direction=Direction.Input),
                             "Binning parameters for the 0th dimension. Enter it as a"
                             "comma-separated list of values with the"
                             "format: 'minimum,maximum,number_of_bins'.")
        self.declareProperty(FloatArrayProperty("BinningDim1", [-5.05,5.05,101], FloatArrayLengthValidator(3), direction=Direction.Input),
                             "Binning parameters for the 1st dimension. Enter it as a"
                             "comma-separated list of values with the"
                             "format: 'minimum,maximum,number_of_bins'.")
        self.declareProperty(FloatArrayProperty("BinningDim2", [-5.05,5.05,101], FloatArrayLengthValidator(3), direction=Direction.Input),
                             "Binning parameters for the 2nd dimension. Enter it as a"
                             "comma-separated list of values with the"
                             "format: 'minimum,maximum,number_of_bins'.")

        self.declareProperty(WorkspaceProperty("OutputWorkspace", "",
                                               optional=PropertyMode.Mandatory,
                                               direction=Direction.Output),
                             "Output Workspace. If background is subtracted _data and _background workspaces will also be made.")

        # Background
        self.setPropertyGroup("Background","Background")
        self.setPropertyGroup("BackgroundScale","Background")

        # Vanadium
        self.setPropertyGroup("SolidAngle","Vanadium")
        self.setPropertyGroup("Flux","Vanadium")

        # Goniometer
        self.setPropertyGroup("SetGoniometer","Goniometer")
        self.setPropertyGroup("Goniometers","Goniometer")
        self.setPropertyGroup("Axis0","Goniometer")
        self.setPropertyGroup("Axis1","Goniometer")
        self.setPropertyGroup("Axis2","Goniometer")

        # Corrections
        self.setPropertyGroup("LoadInstrument","Corrections")
        self.setPropertyGroup("DetCal","Corrections")
        self.setPropertyGroup("Bank","Corrections")
        self.setPropertyGroup("Tube","Corrections")
        self.setPropertyGroup("Pixel","Corrections")

        # Projection and binning
        self.setPropertyGroup("Uproj","Projection and binning")
        self.setPropertyGroup("Vproj","Projection and binning")
        self.setPropertyGroup("Wproj","Projection and binning")
        self.setPropertyGroup("BinningDim0","Projection and binning")
        self.setPropertyGroup("BinningDim1","Projection and binning")
        self.setPropertyGroup("BinningDim2","Projection and binning")

    def validateInputs(self):
        issues = dict()

        if self.getProperty("SymmetryOps").value:
            syms=self.getProperty("SymmetryOps").value
            try:
                if not SpaceGroupFactory.isSubscribedNumber(int(syms)):
                    issues["SymmetryOps"] = 'Space group number '+syms+' is not valid'
            except ValueError:
                if not SpaceGroupFactory.isSubscribedSymbol(syms):
                    for sym in syms.split(';'):
                        if not SymmetryOperationFactory.exists(sym):
                            issues["SymmetryOps"] = sym+' is not valid symmetry or space group name'

        return issues

    def PyExec(self):
        # remove possible old temp workspaces
        [DeleteWorkspace(ws) for ws in self.temp_workspace_list if mtd.doesExist(ws)]

        _background = bool(self.getProperty("Background").value)
        _load_inst = bool(self.getProperty("LoadInstrument").value)
        _detcal = bool(self.getProperty("DetCal").value)
        _outWS_name = self.getPropertyValue("OutputWorkspace")
        _masking = bool(self.getProperty("Bank").value) or bool(self.getProperty("Tube").value) or bool(self.getProperty("Pixel").value)

        UBList = self._generate_UBList()

        progress = Progress(self, 0.0, 1.0, len(UBList)*len(self.getProperty("Filename").value))

        dim0_min, dim0_max, dim0_bins = self.getProperty('BinningDim0').value
        dim1_min, dim1_max, dim1_bins = self.getProperty('BinningDim1').value
        dim2_min, dim2_max, dim2_bins = self.getProperty('BinningDim2').value
        MinValues="{},{},{}".format(dim0_min,dim1_min,dim2_min)
        MaxValues="{},{},{}".format(dim0_max,dim1_max,dim2_max)
        AlignedDim0=",{},{},{}".format(dim0_min, dim0_max, int(dim0_bins))
        AlignedDim1=",{},{},{}".format(dim1_min, dim1_max, int(dim1_bins))
        AlignedDim2=",{},{},{}".format(dim2_min, dim2_max, int(dim2_bins))

        LoadNexus(Filename=self.getProperty("SolidAngle").value, OutputWorkspace='__sa')
        LoadNexus(Filename=self.getProperty("Flux").value, OutputWorkspace='__flux')

        if _masking:
            MaskBTP(Workspace='__sa',
                    Bank=self.getProperty("Bank").value,
                    Tube=self.getProperty("Tube").value,
                    Pixel=self.getProperty("Pixel").value)

        XMin = mtd['__sa'].getXDimension().getMinimum()
        XMax = mtd['__sa'].getXDimension().getMaximum()

        if _background:
            Load(Filename=self.getProperty("Background").value,
                 OutputWorkspace='__bkg',
                 FilterByTofMin=self.getProperty("FilterByTofMin").value,
                 FilterByTofMax=self.getProperty("FilterByTofMax").value)
            if _load_inst:
                LoadInstrument(Workspace='__bkg', Filename=self.getProperty("LoadInstrument").value, RewriteSpectraMap=False)
            if _detcal:
                LoadIsawDetCal(InputWorkspace='__bkg', Filename=self.getProperty("DetCal").value)
            MaskDetectors(Workspace='__bkg',MaskedWorkspace='__sa')
            ConvertUnits(InputWorkspace='__bkg',OutputWorkspace='__bkg',Target='Momentum')
            CropWorkspace(InputWorkspace='__bkg',OutputWorkspace='__bkg',XMin=XMin,XMax=XMax)

        for run in self.getProperty("Filename").value:
            logger.notice("Working on " + run)

            Load(Filename=run,
                 OutputWorkspace='__run',
                 FilterByTofMin=self.getProperty("FilterByTofMin").value,
                 FilterByTofMax=self.getProperty("FilterByTofMax").value)
            if _load_inst:
                LoadInstrument(Workspace='__run', Filename=self.getProperty("LoadInstrument").value, RewriteSpectraMap=False)
            if _detcal:
                LoadIsawDetCal(InputWorkspace='__run', Filename=self.getProperty("DetCal").value)
            MaskDetectors(Workspace='__run',MaskedWorkspace='__sa')
            ConvertUnits(InputWorkspace='__run',OutputWorkspace='__run',Target='Momentum')
            CropWorkspace(InputWorkspace='__run',OutputWorkspace='__run',XMin=XMin,XMax=XMax)

            if self.getProperty('SetGoniometer').value:
                SetGoniometer(Workspace='__run',
                              Goniometers=self.getProperty('Goniometers').value,
                              Axis0=self.getProperty('Axis0').value,
                              Axis1=self.getProperty('Axis1').value,
                              Axis2=self.getProperty('Axis2').value)

            # Set background Goniometer to be the same as data
            if _background:
                mtd['__bkg'].run().getGoniometer().setR(mtd['__run'].run().getGoniometer().getR())

            for ub in UBList:
                progress.report()
                SetUB(Workspace='__run', UB=ub)
                ConvertToMD(InputWorkspace='__run',
                            OutputWorkspace='__md',
                            QDimensions='Q3D',
                            dEAnalysisMode='Elastic',
                            Q3DFrames='HKL',
                            QConversionScales='HKL',
                            Uproj=self.getProperty('Uproj').value,
                            Vproj=self.getProperty('Vproj').value,
                            Wproj=self.getProperty('wproj').value,
                            MinValues=MinValues,
                            MaxValues=MaxValues)
                MDNormSCD(InputWorkspace=mtd['__md'],
                          FluxWorkspace='__flux',
                          SolidAngleWorkspace='__sa',
                          OutputWorkspace='__data',
                          SkipSafetyCheck=True,
                          TemporaryDataWorkspace='__data' if mtd.doesExist('__data') else None,
                          OutputNormalizationWorkspace='__norm',
                          TemporaryNormalizationWorkspace='__norm' if mtd.doesExist('__norm') else None,
                          AlignedDim0=mtd['__md'].getDimension(0).name+AlignedDim0,
                          AlignedDim1=mtd['__md'].getDimension(1).name+AlignedDim1,
                          AlignedDim2=mtd['__md'].getDimension(2).name+AlignedDim2)
                DeleteWorkspace('__md')

                if _background:
                    SetUB(Workspace='__bkg', UB=ub)
                    ConvertToMD(InputWorkspace='__bkg',
                                OutputWorkspace='__bkg_md',
                                QDimensions='Q3D',
                                dEAnalysisMode='Elastic',
                                Q3DFrames='HKL',
                                QConversionScales='HKL',
                                Uproj=self.getProperty('Uproj').value,
                                Vproj=self.getProperty('Vproj').value,
                                Wproj=self.getProperty('Wproj').value,
                                MinValues=MinValues,
                                MaxValues=MaxValues)
                    MDNormSCD(InputWorkspace='__bkg_md',
                              FluxWorkspace='__flux',
                              SolidAngleWorkspace='__sa',
                              SkipSafetyCheck=True,
                              OutputWorkspace='__bkg_data',
                              TemporaryDataWorkspace='__bkg_data' if mtd.doesExist('__bkg_data') else None,
                              OutputNormalizationWorkspace='__bkg_norm',
                              TemporaryNormalizationWorkspace='__bkg_norm' if mtd.doesExist('__bkg_norm') else None,
                              AlignedDim0=mtd['__bkg_md'].getDimension(0).name+AlignedDim0,
                              AlignedDim1=mtd['__bkg_md'].getDimension(1).name+AlignedDim1,
                              AlignedDim2=mtd['__bkg_md'].getDimension(2).name+AlignedDim2)
                    DeleteWorkspace('__bkg_md')
            DeleteWorkspace('__run')

        if _background:
            # outWS = data / norm - bkg_data / bkg_norm * BackgroundScale
            DivideMD(LHSWorkspace='__data',RHSWorkspace='__norm',OutputWorkspace=_outWS_name+'_data')
            DivideMD(LHSWorkspace='__bkg_data',RHSWorkspace='__bkg_norm',OutputWorkspace=_outWS_name+'_background')
            CreateSingleValuedWorkspace(OutputWorkspace='__scale', DataValue=self.getProperty('BackgroundScale').value)
            MultiplyMD(LHSWorkspace=_outWS_name+'_background',
                       RHSWorkspace='__scale',
                       OutputWorkspace='__scaled_background')
            DeleteWorkspace('__scale')
            MinusMD(LHSWorkspace=_outWS_name+'_data',RHSWorkspace='__scaled_background',OutputWorkspace=_outWS_name)
        else:
            # outWS = data / norm
            DivideMD(LHSWorkspace='__data',RHSWorkspace='__norm',OutputWorkspace=_outWS_name)

        self.setProperty("OutputWorkspace", mtd[_outWS_name])

        # remove temp workspaces
        [DeleteWorkspace(ws) for ws in self.temp_workspace_list if mtd.doesExist(ws)]

    def _generate_UBList(self):
        CreateSingleValuedWorkspace(OutputWorkspace='__ub')
        LoadIsawUB('__ub',self.getProperty("UBMatrix").value)
        ub=mtd['__ub'].sample().getOrientedLattice().getUB().copy()
        DeleteWorkspace(Workspace='__ub')

        symOps = self.getProperty("SymmetryOps").value
        if symOps:
            try:
                symOps = SpaceGroupFactory.subscribedSpaceGroupSymbols(int(symOps))[0]
            except ValueError:
                pass
            if SpaceGroupFactory.isSubscribedSymbol(symOps):
                symOps = SpaceGroupFactory.createSpaceGroup(symOps).getSymmetryOperations()
            else:
                symOps = SymmetryOperationFactory.createSymOps(symOps)
            logger.information('Using symmetries: '+str([sym.getIdentifier() for sym in symOps]))

            ub_list=[]
            for sym in symOps:
                UBtrans = np.zeros((3,3))
                UBtrans[0] = sym.transformHKL([1,0,0])
                UBtrans[1] = sym.transformHKL([0,1,0])
                UBtrans[2] = sym.transformHKL([0,0,1])
                UBtrans=np.matrix(UBtrans.T)
                ub_list.append(ub*UBtrans)
            return ub_list
        else:
            return [ub]


AlgorithmFactory.subscribe(SingleCrystalDiffuseReduction)
