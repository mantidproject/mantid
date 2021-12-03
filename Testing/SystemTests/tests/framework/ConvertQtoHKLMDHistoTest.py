from mantid.simpleapi import (
    CopySample,
    CreateMDHistoWorkspace,
    CreatePeaksWorkspace,
    CreateSingleValuedWorkspace,
    LoadInstrument,
    SetGoniometer,
    SetUB,
    ConvertWANDSCDtoQ,
    ConvertHFIRSCDtoMDE,
    ConvertQtoHKLMDHisto,
    mtd,
)
import systemtesting
import numpy as np

from mantid.kernel import FloatTimeSeriesProperty


class ConvertQtoHKLMDHistoTest_match_ConvertWANDSCDtoQ_test(
    systemtesting.MantidSystemTest
):
    def requiredMemoryMB(self):
        return 8000

    def runTest(self):
        S = np.random.random(32*240*100)

        ConvertWANDSCDtoQTest_data = CreateMDHistoWorkspace(Dimensionality=3, Extents='0.5,32.5,0.5,240.5,0.5,100.5',
                                                            SignalInput=S.ravel('F'), ErrorInput=np.sqrt(S.ravel('F')),
                                                            NumberOfBins='32,240,100', Names='y,x,scanIndex',
                                                            Units='bin,bin,number')

        ConvertWANDSCDtoQTest_dummy = CreateSingleValuedWorkspace()
        LoadInstrument(ConvertWANDSCDtoQTest_dummy, InstrumentName='WAND', RewriteSpectraMap=False)

        ConvertWANDSCDtoQTest_data.addExperimentInfo(ConvertWANDSCDtoQTest_dummy)

        log = FloatTimeSeriesProperty('s1')
        for t, v in zip(range(100), np.arange(0, 50, 0.5)):
            log.addValue(t, v)
        ConvertWANDSCDtoQTest_data.getExperimentInfo(0).run()['s1'] = log
        ConvertWANDSCDtoQTest_data.getExperimentInfo(0).run().addProperty('duration', [60.] * 100, True)
        ConvertWANDSCDtoQTest_data.getExperimentInfo(0).run().addProperty('monitor_count', [120000.] * 100, True)
        ConvertWANDSCDtoQTest_data.getExperimentInfo(0).run().addProperty('twotheta', list(
            np.linspace(np.pi * 2 / 3, 0, 240).repeat(32)), True)
        ConvertWANDSCDtoQTest_data.getExperimentInfo(0).run().addProperty('azimuthal', list(
            np.tile(np.linspace(-0.15, 0.15, 32), 240)), True)

        peaks = CreatePeaksWorkspace(NumberOfPeaks=0, OutputType='LeanElasticPeak')

        SetUB(ConvertWANDSCDtoQTest_data, 5, 5, 7, 90, 90, 120, u=[-1, 0, 1], v=[1, 0, 1])
        SetGoniometer(ConvertWANDSCDtoQTest_data, Axis0='s1,0,1,0,1', Average=False)

        CopySample(InputWorkspace=ConvertWANDSCDtoQTest_data,
                   OutputWorkspace=peaks,
                   CopyName=False,
                   CopyMaterial=False,
                   CopyEnvironment=False,
                   CopyShape=False,
                   CopyLattice=True)

        Q = ConvertWANDSCDtoQ(InputWorkspace=ConvertWANDSCDtoQTest_data,
                              UBWorkspace=peaks,
                              Wavelength=1.486,
                              Frame='HKL',
                              Uproj='1,1,0',
                              Vproj='-1,1,0',
                              BinningDim0='-6.04,6.04,151',
                              BinningDim1='-6.04,6.04,151',
                              BinningDim2='-6.04,6.04,151')

        data_norm = ConvertHFIRSCDtoMDE(ConvertWANDSCDtoQTest_data,
                                        Wavelength=1.486,
                                        MinValues='-6.04,-6.04,-6.04',
                                        MaxValues='6.04,6.04,6.04')

        HKL = ConvertQtoHKLMDHisto(data_norm,
                                   PeaksWorkspace=peaks,
                                   Uproj='1,1,0',
                                   Vproj='-1,1,0',
                                   Extents='-6.04,6.04,-6.04,6.04,-6.04,6.04',
                                   Bins='151,151,151')

        for i in range(HKL.getNumDims()):
            print(HKL.getDimension(i).getUnits(), Q.getDimension(i).getUnits())
            np.testing.assert_equal(
                HKL.getDimension(i).getUnits(), Q.getDimension(i).getUnits()
            )

        hkl_data = mtd["HKL"].getSignalArray()
        Q_data = mtd["Q"].getSignalArray()

        print(np.isnan(Q_data).sum())
        print(np.isclose(hkl_data, 0).sum())

        xaxis = mtd["HKL"].getXDimension()
        yaxis = mtd["HKL"].getYDimension()
        zaxis = mtd["HKL"].getZDimension()

        x, y, z = np.meshgrid(
            np.linspace(xaxis.getMinimum(),
                        xaxis.getMaximum(),
                        xaxis.getNBins()),
            np.linspace(yaxis.getMinimum(),
                        yaxis.getMaximum(),
                        yaxis.getNBins()),
            np.linspace(zaxis.getMinimum(),
                        zaxis.getMaximum(),
                        zaxis.getNBins()),
            indexing="ij",
            copy=False,
        )

        print(
            x[~np.isnan(Q_data)].mean(),
            y[~np.isnan(Q_data)].mean(),
            z[~np.isnan(Q_data)].mean(),
        )
        print(
            x[~np.isclose(hkl_data, 0)].mean(),
            y[~np.isclose(hkl_data, 0)].mean(),
            z[~np.isclose(hkl_data, 0)].mean(),
        )
        np.testing.assert_almost_equal(
            x[~np.isnan(Q_data)].mean(),
            x[~np.isclose(hkl_data, 0)].mean(),
            decimal=2
        )
        np.testing.assert_almost_equal(
            y[~np.isnan(Q_data)].mean(),
            y[~np.isclose(hkl_data, 0)].mean(),
            decimal=2
        )
        np.testing.assert_almost_equal(
            z[~np.isnan(Q_data)].mean(),
            z[~np.isclose(hkl_data, 0)].mean(),
            decimal=1
        )
