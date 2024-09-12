# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
# pylint: disable=no-init,invalid-name
import numpy as np
import mantid
import mantid.simpleapi as api
from mantid.kernel import StringListValidator

try:
    import h5py  # http://www.h5py.org/
except ImportError:
    pass


class SaveNexusPD(mantid.api.PythonAlgorithm):
    NX_CLASS = "NX_class"
    AXES_DICT = {"tof": "time-of-flight", "dspacing": "d-spacing", "Q": "momentum transfer"}

    _dtype = None
    _compressArgs = {}
    _tempNames = []
    _sample = None
    _sourcePos = None

    def category(self):
        return "DataHandling\\Nexus"

    def seeAlso(self):
        return ["SaveNexus"]

    def name(self):
        return "SaveNexusPD"

    def summary(self):
        return "Save powder diffraction data to a NeXus file"

    def _determineDtype(self):
        datatype = self.getProperty("DataType").value

        if datatype == "float32":
            self._dtype = np.float32
        elif datatype == "float64":
            self._dtype = np.float64
        else:
            raise RuntimeError("Do not understand '%s'" % datatype)

    def _determineCompression(self):
        compression = self.getProperty("Compression").value

        if str(compression) != "None":
            self._compressArgs["compression"] = compression

    def PyInit(self):
        self.declareProperty(mantid.api.WorkspaceProperty("InputWorkspace", "", mantid.kernel.Direction.Input), "Workspace to save")
        self.declareProperty(
            mantid.api.FileProperty("OutputFilename", "", action=mantid.api.FileAction.Save, extensions=[".h5"]), doc="Name of the savefile"
        )

        group = "Options"

        self.declareProperty("NXentry", "", "Overrides the NXentry name from the workspace name")
        self.declareProperty(
            "DataType", "float32", StringListValidator(["float32", "float64"]), doc="All data saved will be converted to this type"
        )
        self.declareProperty("Compression", "gzip", StringListValidator(["gzip", "lzf", "None"]), doc="Algorithm for compressing data")
        self.declareProperty("WriteMomentumTransfer", True, doc="Add the momentum transfer (Q) axis to the file")
        self.declareProperty("ProtonChargeUnits", "uA.hour", StringListValidator(["uA.hour", "C", "pC"]))
        self.declareProperty("Append", False)

        self.setPropertyGroup("NXentry", group)
        self.setPropertyGroup("DataType", group)
        self.setPropertyGroup("Compression", group)
        self.setPropertyGroup("WriteMomentumTransfer", group)
        self.setPropertyGroup("ProtonChargeUnits", group)
        self.setPropertyGroup("Append", group)

    def validateInputs(self):
        issues = dict()

        wkspParamName = "InputWorkspace"
        allowedUnits = ["TOF", "dSpacing", "MomentumTransfer"]

        # check for units
        wksp = self.getProperty(wkspParamName).value
        units = wksp.getAxis(0).getUnit().unitID()
        if units not in allowedUnits:
            allowedUnits = ["'%s'" % unit for unit in allowedUnits]
            allowedUnits = ", ".join(allowedUnits)
            issues[wkspParamName] = "Only support units %s" % allowedUnits

        return issues

    def _createInstrument(self, nxentry):
        nxinstrument = nxentry.create_group("instrument")
        nxinstrument.attrs[self.NX_CLASS] = "NXinstrument"

        nxmoderator = nxinstrument.create_group("moderator")
        nxmoderator.attrs[self.NX_CLASS] = "NXmoderator"

        if self._sourcePos is not None:
            L1 = self._sourcePos.distance(self._sample.getPos())
            L1 = -1.0 * abs(L1)  # nexus likes the distance negative
            temp = nxmoderator.create_dataset("distance", data=[L1], dtype=self._dtype)
            temp.attrs["units"] = "metre"

        return nxinstrument

    def _writeY(self, nxdata, wksp, index):
        temp = nxdata.create_dataset(name="data", data=wksp.readY(index), dtype=self._dtype, **self._compressArgs)
        temp.attrs["uncertainties"] = "errors"
        temp.attrs["axes"] = "dspacing"
        temp.attrs["signal"] = 1
        temp.attrs["units"] = str(wksp.YUnit())
        nxdata.create_dataset(name="errors", data=wksp.readE(index), dtype=self._dtype, **self._compressArgs)

    # pylint: disable=too-many-arguments
    def _writeX(self, nxdata, name, wksp, index, writeDx):
        units = wksp.getAxis(0).getUnit().symbol().ascii()
        reverse = name == "Q"  # reverse the array
        arr = wksp.readX(index)
        if reverse:
            arr = arr[::-1]  # reverse the array
        temp = nxdata.create_dataset(name=name, data=arr, dtype=self._dtype, **self._compressArgs)
        temp.attrs["units"] = units
        temp.attrs["longname"] = self.AXES_DICT[name]

        if writeDx:  # optional
            arr = wksp.readDx(index)
            if reverse:  # reverse the array
                arr = arr[::-1]

            temp = nxdata.create_dataset(name=name + "_errors", data=arr, dtype=self._dtype, **self._compressArgs)
            temp.attrs["units"] = units

    def _writeProtonCharge(self, nxentry, wksp):
        if "gd_prtn_chrg" not in wksp.run():
            return  # nothing to do

        pcharge = wksp.run()["gd_prtn_chrg"]

        value = pcharge.value
        units = str(pcharge.units)  # will be in units of uA.hour

        unitsDesired = self.getProperty("ProtonChargeUnits").value
        if unitsDesired == "uA.hour":
            pass  # nothing to do
        elif unitsDesired == "C":
            value = value * 3600.0 / 1.0e6
        elif unitsDesired == "pC":
            value = value * 3600.0 * 1.0e6
        units = unitsDesired

        field = nxentry.create_dataset("proton_charge", data=[value], dtype=self._dtype)
        field.attrs["units"] = units

    def _writeDetectorPos(self, nxinstrument, name, detector):
        nxdetector = nxinstrument.create_group(name)
        nxdetector.attrs[self.NX_CLASS] = "NXdetector"

        if self._sample is None or detector is None:
            return nxdetector

        # only continue if there is position information
        L2 = detector.getDistance(self._sample)
        polar = detector.getTwoTheta(self._sample.getPos(), self._sourcePos)  # radians
        azi = detector.getPhi()  # radians

        temp = nxdetector.create_dataset("distance", data=[abs(L2)], dtype=self._dtype)
        temp.attrs["units"] = "metre"

        temp = nxdetector.create_dataset("polar_angle", data=[polar], dtype=self._dtype)
        temp.attrs["units"] = "radian"

        temp = nxdetector.create_dataset("azimuthal_angle", data=[azi], dtype=self._dtype)
        temp.attrs["units"] = "radian"

        return nxdetector

    def _deleteTmpWksp(self):
        for wsname in self._tempNames:
            if mantid.mtd.doesExist(wsname):
                mantid.mtd.remove(wsname)

    def _getOtherUnits(self, wksp):
        """Returns workspaces in units [TOF, dSpacing, MomentumTransfer]"""

        xUnit = wksp.getAxis(0).getUnit()
        x_id = xUnit.unitID()

        result = [None, None, None]
        for i, target in enumerate(["TOF", "dSpacing", "MomentumTransfer"]):
            if str(x_id) == target:
                result[i] = wksp
            elif self._sourcePos is not None:
                wsname = "__SaveNexusPD_%s" % target
                self._tempNames.append(wsname)
                temp = api.ConvertUnits(InputWorkspace=wksp, OutputWorkspace=wsname, Target=target, EMode="Elastic")
                result[i] = temp
            else:
                pass  # can't ConvertUnits
        return result

    def _determineSourceSample(self, wksp):
        self._sample = wksp.getInstrument().getSample()
        source = wksp.getInstrument().getSource()

        # set all of the information to None if the
        # instrument doesn't supply it
        if (source is not None) and (self._sample is not None):
            self._sourcePos = self._sample.getPos() - source.getPos()

    # pylint: disable=too-many-branches
    def PyExec(self):
        self._determineDtype()
        self._determineCompression()

        wksp = self.getProperty("InputWorkspace").value

        self._determineSourceSample(wksp)

        (tof, dspacing, momentumtransfer) = self._getOtherUnits(wksp)

        xAxesToWrite = []
        if tof is not None:
            xAxesToWrite.append("tof")
        if dspacing is not None:
            xAxesToWrite.append("dspacing")
        if self.getProperty("WriteMomentumTransfer").value and momentumtransfer is not None:
            xAxesToWrite.append("Q")

        append = self.getProperty("Append").value
        if append:
            filemode = "a"  # append
        else:
            filemode = "w"  # write, will silently overwrite

        filename = self.getProperty("OutputFilename").value
        with h5py.File(filename, filemode) as handle:
            # default name for entry is the workspace name
            wkspname = self.getProperty("NXentry").value.strip()
            if len(wkspname) == 0:
                wkspname = str(wksp)

            # check for the entry alread existing in append mode
            if append and wkspname in handle.keys():
                raise IOError("NXentry named '%s' already exists in '%s'" % (wkspname, filename))

            # create the entry
            nxentry = handle.create_group(wkspname)
            nxentry.attrs[self.NX_CLASS] = "NXentry"

            self._writeProtonCharge(nxentry, wksp)

            nxinstrument = self._createInstrument(nxentry)

            for i in range(wksp.getNumberHistograms()):
                writeDx = not np.all(wksp.readDx(i) == 0)

                dataname = "spectrum_%d" % wksp.getSpectrum(i).getSpectrumNo()

                nxdata = nxentry.create_group(dataname)
                nxdata.attrs[self.NX_CLASS] = "NXdata"

                if self._sourcePos is None:
                    detector = None
                else:
                    try:
                        detector = wksp.getDetector(i)
                    except RuntimeError:
                        detector = None
                nxdetector = self._writeDetectorPos(nxinstrument, dataname, detector)

                self._writeY(nxdetector, tof, i)

                # write out axes in the nxdetector
                for field, wkspIter in zip(("tof", "dspacing", "Q"), (tof, dspacing, momentumtransfer)):
                    if field in xAxesToWrite:
                        self._writeX(nxdetector, field, wkspIter, i, writeDx)

                # create hard links into nxdata
                for field in ["data", "errors"] + xAxesToWrite:
                    nxdata[field] = nxdetector[field]

        self._deleteTmpWksp()


if "h5py" in locals():
    mantid.api.AlgorithmFactory.subscribe(SaveNexusPD)
