from mantid.simpleapi import (
    LoadMD,
    CreatePeaksWorkspace,
    CreateSingleValuedWorkspace,
    LoadIsawUB,
    SetUB,
    ConvertWANDSCDtoQ,
    ConvertHFIRSCDtoMDE,
    ConvertQtoHKLMDHisto,
    mtd,
)
import systemtesting
import numpy as np

from mantid import config


class ConvertQtoHKLMDHistoTest_match_ConvertWANDSCDtoQ_test(
    systemtesting.MantidSystemTest
):
    def requiredMemoryMB(self):
        return 8000

    def runTest(self):
        config["Q.convention"] = "Inelastic"

        # wavelength (angstrom) -----
        wavelength = 1.486

        # minimum and maximum values for Q sample -----
        min_values = [-7.5, -0.65, -4.4]
        max_values = [6.8, 0.65, 7.5]

        # ---

        data = LoadMD(
            Filename="/HFIR/HB2C/shared/WANDscripts/test_data/data.nxs"
        )

        peaks = CreatePeaksWorkspace(
            NumberOfPeaks=0,
            OutputType="LeanElasticPeak"
        )

        ws = CreateSingleValuedWorkspace()
        LoadIsawUB(
            InputWorkspace=ws,
            Filename="/HFIR/HB2C/shared/WANDscripts/test_data/data.mat",
        )
        UB = ws.sample().getOrientedLattice().getUB()

        SetUB(peaks, UB=UB)

        Q = ConvertWANDSCDtoQ(
            InputWorkspace="data",
            UBWorkspace="peaks",
            Wavelength=wavelength,
            Frame="HKL",
            Uproj="1,1,0",
            Vproj="-1,1,0",
            BinningDim0="-6.02,6.02,301",
            BinningDim1="-6.02,6.02,301",
            BinningDim2="-6.02,6.02,301",
        )

        data_norm = ConvertHFIRSCDtoMDE(
            data,
            Wavelength=wavelength,
            MinValues="{},{},{}".format(*min_values),
            MaxValues="{},{},{}".format(*max_values),
        )

        HKL = ConvertQtoHKLMDHisto(
            data_norm,
            PeaksWorkspace="peaks",
            Uproj="1,1,0",
            Vproj="-1,1,0",
            Extents="-6.02,6.02,-6.02,6.02,-6.02,6.02",
            Bins="301,301,301",
        )

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
            decimal=2
        )
