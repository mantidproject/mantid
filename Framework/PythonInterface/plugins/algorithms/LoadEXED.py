# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
from mantid.kernel import Direction
from mantid.api import (
    PythonAlgorithm,
    AlgorithmFactory,
    FileAction,
    FileProperty,
    WorkspaceProperty,
    MatrixWorkspaceProperty,
    PropertyMode,
    WorkspaceFactory,
    AnalysisDataService,
)
from mantid.simpleapi import LoadInstrument, AddSampleLogMultiple, SetGoniometer
from mantid.simpleapi import ExtractSpectra, MaskDetectors, RemoveMaskedSpectra
from mantid.simpleapi import RotateInstrumentComponent
import struct
import numpy as np
import copy


class LoadEXED(PythonAlgorithm):
    __doc__ = """This algorithm reads Exed raw files and creates two workspaces.
    One for the detectors and another for the monitor.
    """

    def category(self):
        return "Inelastic;Diffraction"

    def name(self):
        return "LoadEXED"

    def version(self):
        return 1

    def summary(self):
        return """This algorithm reads Exed raw files and creates two workspaces.
        One for the detectors and another for the monitor."""

    def PyInit(self):
        self.declareProperty(
            FileProperty(name="Filename", defaultValue="", action=FileAction.Load, extensions=["raw"]), doc="Data file produced by egraph."
        )

        self.declareProperty(
            FileProperty(name="InstrumentXML", defaultValue="", action=FileAction.OptionalLoad, extensions=["xml"]),
            doc="Instrument definition file. If no file is specified, the default idf is used.",
        )

        self.declareProperty(
            "AngleOverride",
            -255.0,
            doc="Rotation angle (degrees) of the EXED magnet."
            "\nThis should be read from the data file!\n"
            "Only change the value if the file has an incomplete header!",
        )

        self.declareProperty(
            MatrixWorkspaceProperty(name="OutputWorkspace", defaultValue="", direction=Direction.Output),
            doc="Mantid workspace containing the measured data.",
        )

        self.declareProperty(
            WorkspaceProperty(name="OutputMonitorWorkspace", defaultValue="", direction=Direction.Output, optional=PropertyMode.Optional),
            doc="Workspace containing the measured monitor spectra.",
        )

    def PyExec(self):
        fn = self.getPropertyValue("Filename")
        wsn = self.getPropertyValue("OutputWorkspace")
        monitor_workspace_name = self.getPropertyValue("OutputMonitorWorkspace")
        if monitor_workspace_name == "":
            self.setPropertyValue("OutputMonitorWorkspace", wsn + "_Monitors")
        # print (fn, wsn)
        self.override_angle = self.getPropertyValue("AngleOverride")
        self.fxml = self.getPropertyValue("InstrumentXML")

        # load data

        parms_dict, det_udet, det_count, det_tbc, data = self.read_file(fn)
        nrows = int(parms_dict["NDET"])
        # nbins=int(parms_dict['NTC'])
        xdata = np.array(det_tbc)
        xdata_mon = np.linspace(xdata[0], xdata[-1], len(xdata))
        ydata = data.astype(float)
        ydata = ydata.reshape(nrows, -1)
        edata = np.sqrt(ydata)
        # CreateWorkspace(OutputWorkspace=wsn,DataX=xdata,DataY=ydata,DataE=edata,
        #                NSpec=nrows,UnitX='TOF',WorkspaceTitle='Data',YUnitLabel='Counts')
        nr, nc = ydata.shape
        ws = WorkspaceFactory.create("Workspace2D", NVectors=nr, XLength=nc + 1, YLength=nc)
        for i in range(nrows):
            ws.setX(i, xdata)
            ws.setY(i, ydata[i])
            ws.setE(i, edata[i])
        ws.getAxis(0).setUnit("tof")
        AnalysisDataService.addOrReplace(wsn, ws)

        # self.setProperty("OutputWorkspace", wsn)
        # print ("ws:", wsn)
        # ws=mtd[wsn]

        # fix the x values for the monitor
        for i in range(nrows - 2, nrows):
            ws.setX(i, xdata_mon)
        self.log().information("set detector IDs")
        # set detetector IDs
        for i in range(nrows):
            ws.getSpectrum(i).setDetectorID(det_udet[i])
        # Sample_logs the header values are written into the sample logs
        log_names = [str(sl.encode("ascii", "ignore").decode()) for sl in parms_dict.keys()]
        log_values = [str(sl.encode("ascii", "ignore").decode()) if isinstance(sl, str) else str(sl) for sl in parms_dict.values()]
        for i in range(len(log_values)):
            if ("nan" in log_values[i]) or ("NaN" in log_values[i]):
                log_values[i] = "-1.0"
        AddSampleLogMultiple(Workspace=wsn, LogNames=log_names, LogValues=log_values)
        SetGoniometer(Workspace=wsn, Goniometers="Universal")
        if self.fxml == "":
            LoadInstrument(Workspace=wsn, InstrumentName="Exed", RewriteSpectraMap=True)
        else:
            LoadInstrument(Workspace=wsn, Filename=self.fxml, RewriteSpectraMap=True)
        try:
            RotateInstrumentComponent(
                Workspace=wsn, ComponentName="Tank", Y=1, Angle=-float(parms_dict["phi"].encode("ascii", "ignore")), RelativeRotation=False
            )
        except:
            self.log().warning(
                "The instrument does not contain a 'Tank' component. "
                "This means that you are using a custom XML instrument definition. "
                "OMEGA_MAG will be ignored."
            )
            self.log().warning("Please make sure that the detector positions in the instrument definition are correct.")
        # Separate monitors into seperate workspace
        __temp_monitors = ExtractSpectra(
            InputWorkspace=wsn,
            WorkspaceIndexList=",".join([str(s) for s in range(nrows - 2, nrows)]),
            OutputWorkspace=self.getPropertyValue("OutputMonitorWorkspace"),
        )
        # ExtractSpectra(InputWorkspace = wsn, WorkspaceIndexList = ','.join([str(s) for s in range(nrows-2, nrows)]),
        # OutputWorkspace = wsn + '_Monitors')
        MaskDetectors(Workspace=wsn, WorkspaceIndexList=",".join([str(s) for s in range(nrows - 2, nrows)]))
        RemoveMaskedSpectra(InputWorkspace=wsn, OutputWorkspace=wsn)

        self.setProperty("OutputWorkspace", wsn)
        self.setProperty("OutputMonitorWorkspace", __temp_monitors)

    def read_file(self, fn):
        """
        function to read header and return a dictionary of the parameters parms_dict
        the read the data and return it in det_udet, det_count, det_tbc, and data
        """
        fin = open(fn, "rb")
        header = True
        parms_dict = {}
        parms_dict["omega"] = "0.0"
        parms_dict["chi"] = "0.0"
        # parms_dict['ALL_OMEGA_MAG'] = []
        while header:
            b_line = fin.readline()
            line = b_line.decode("ascii")
            if "=" in line:
                line_lst = line.split("=")
                if "omment" in line_lst[0]:
                    parms_dict["Comment"] = line.strip("Comment =")
                # if "OMEGA_MAG" in line_lst[0]:
                #    parms_dict['ALL_OMEGA_MAG'].append(line_lst[1].strip())
                #    parms_dict[line_lst[0].strip()]=parm_val
                elif len(line_lst) > 1:
                    parm_val = line_lst[1].strip()
                    if (parm_val.isdigit()) & (line_lst[0].find("Run_Number") < 0):
                        parm_val = eval(parm_val)
                    parms_dict[line_lst[0].strip()] = parm_val
            if line.find("Following are binary data") > 0:
                header = False
                while line.find("DATA=1") < 0:
                    b_line = fin.readline()
                    line = b_line.decode("ascii")
                    # print line
        nrows = int(parms_dict["NDET"])
        nbins = int(parms_dict["NTC"])
        self.log().information("read UDET")
        # print ("read UDET")
        det_udet = self.struct_data_read(fin, nrows)

        self.log().information("read Counter")
        det_count = self.struct_data_read(fin, nrows)

        self.log().information("read TimeBinBoundaries")
        det_tbc = self.struct_data_read(fin, nbins + 1, "f")

        self.log().information("read Data")
        data = np.fromfile(fin, np.uint32, nrows * nbins, "")

        fin.close()
        # print(parms_dict)
        # parms_dict['ALL_OMEGA_MAG'] = ','.join(parms_dict['ALL_OMEGA_MAG'])
        # print('DEBUG: CAR_OMEGA_MAG is ' + parms_dict['ALL_OMEGA_MAG'])
        if float(self.override_angle) < -254.0:
            try:
                parms_dict["phi"] = str(copy.deepcopy(parms_dict["CAR_OMEGA_MAG"]))
            except KeyError:
                try:
                    parms_dict["phi"] = str(copy.deepcopy(parms_dict["OMEGA_MAG"]))
                except KeyError:
                    self.log().warning("OMEGA_MAG value not found! The detector position will be incorrect!")
            else:
                self.log().information("OMEGA_MAG read as " + str(parms_dict["phi"]))
        else:
            parms_dict["phi"] = str(self.override_angle)
            self.log().warning("OMEGA_MAG taken from user input, not from the file. Current value: " + str(parms_dict["phi"]))
        return parms_dict, det_udet, det_count, det_tbc, data

    def struct_data_read(self, fin, nrows, data_type="i", byte_size=4):
        """
        helper function to read binary data_type
        requires the file handle, number of rows and data_type
        """
        self.log().debug("nrows %d" % nrows)
        tmp_lst = [struct.unpack(data_type, fin.read(byte_size))[0] for i in range(nrows)]
        # for i in range(nrows):
        #    data = struct.unpack(data_type,fin.read(byte_size))[0]
        #    tmp_lst.append(data)
        # print(tmp_lst)
        return tmp_lst


# Register algorthm with Mantid.
AlgorithmFactory.subscribe(LoadEXED)
