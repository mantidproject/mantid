import numpy as np
from mantid.api import AlgorithmFactory, FileProperty, FileAction, PythonAlgorithm, IPeaksWorkspaceProperty
from mantid.kernel import StringListValidator, Direction
from mantid.simpleapi import SaveHKL

# List of file format names supported by this algorithm
SUPPORTED_FORMATS = ["Fullprof", "GSAS", "Jana", "SHELX"]


class SaveReflections(PythonAlgorithm):

    def category(self):
        return "DataHandling"

    def summary(self):
        return "Saves single crystal reflections to a variety of formats"

    def PyInit(self):
        """Initilize the algorithms properties"""

        self.declareProperty(FileProperty("Filename", "",
                             action=FileAction.Save,
                             direction=Direction.Input),
                             doc="File with the data from a phonon calculation.")

        self.declareProperty(IPeaksWorkspaceProperty("InputWorkspace", '', Direction.Input),
                             doc="The name of the peaks worksapce to save")

        self.declareProperty(name="Format",
                             direction=Direction.Input,
                             defaultValue="Fullprof",
                             validator=StringListValidator(SUPPORTED_FORMATS),
                             doc="The output format to export reflections to")

    def PyExec(self):
        """Execute the algorithm"""
        workspace = self.getProperty("InputWorkspace").value
        output_format = self.getPropertyValue("Format")
        file_name = self.getPropertyValue("Filename")

        file_writer  = self.choose_format(output_format)
        file_writer(file_name, workspace)

    def choose_format(self, output_format):
        """Choose the function to use to write out data for this format

        :param output_format: the format to use to output refelctions as. Options are
            "Fullprof", "GSAS", "Jana", and "SHELX".

        :returns: file format to use for saving reflections to an ASCII file.
        """
        if output_format == "Fullprof":
            return FullprofFormat()
        elif output_format == "Jana":
            return JanaFormat()
        elif output_format == "GSAS" or output_format == "SHELX":
            return SaveHKLFormat()
        else:
            raise RuntimeError("Unexpected file format {}. Format should be one of {}."
                               .format(output_format, ", ".join(SUPPORTED_FORMATS)))

# ------------------------------------------------------------------------------------------------------


class FullprofFormat(object):

    def __call__(self, file_name, workspace):
        with open(file_name, 'w') as f_handle:
            self.write_header(f_handle, workspace)
            self.write_peaks(f_handle, workspace)

    def write_header(self, f_handle, workspace):
        f_handle.write(workspace.getTitle())
        f_handle.write("(3i4,2f12.2,i5,4f10.4)\n")
        f_handle.write("  0 0 0\n")
        f_handle.write("#  h   k   l      Fsqr       s(Fsqr)   Cod   Lambda\n")

    def write_peaks(self, f_handle, workspace):
        for i, peak in enumerate(workspace):
            data = (peak['h'],peak['k'],peak['l'],peak['Intens'],peak['SigInt'],i+1,peak['Wavelength'])
            line = "%4i%4i%4i%12.2f%12.2f%5i%10.4f\n" % data
            f_handle.write(line)

# ------------------------------------------------------------------------------------------------------


class JanaFormat(object):

    def __call__(self, file_name, workspace):
        with open(file_name, 'w') as f_handle:
            self.write_header(f_handle, workspace)
            self.write_peaks(f_handle, workspace)

    def write_header(self, f_handle, ws):
        sample = ws.sample()
        lattice = sample.getOrientedLattice()
        lattice_params = [lattice.a(), lattice.b(), lattice.c(), lattice.alpha(), lattice.beta(), lattice.gamma()]
        lattice_params = "".join(["{: >10.4f}".format(value) for value in lattice_params])
        f_handle.write("# Lattice parameters   {}\n".format(lattice_params))
        f_handle.write("(3i5,2f12.2,i5,4f10.4)\n")

    def write_peaks(self, f_handle, ws):
        column_names = ["h", "k", "l", "Fsqr", "s(Fsqr)", "Cod", "Lambda", "Twotheta", "Transm.", "Tbar", "TDS"]
        column_format = "#{:>4}{:>4}{:>4}{:>12}{:>12}{:>5}{:>10}{:>10}{:>10}{:>10}{:>10}\n"
        f_handle.write(column_format.format(*column_names))
        for row in ws:
            self.write_peak(f_handle, ws, row)

    def write_peak(self, f_handle, ws, peak):
        f_handle.write("{h: >5.0f}{k: >5.0f}{l: >5.0f}".format(**peak))
        f_handle.write("{Intens: >12.2f}".format(**peak))
        f_handle.write("{SigInt: >12.2f}".format(**peak))
        f_handle.write("{: >5.0f}".format(1))
        f_handle.write("{Wavelength: >10.4f}".format(**peak))
        f_handle.write("{: >10.4f}".format(self._get_two_theta(ws, peak['DetID'])))
        f_handle.write("{: >10.4f}{: >10.4f}{: >10.4f}".format(1.0, 0.0, 0.0))
        f_handle.write("\n")

    def _get_two_theta(self, ws, det_id):
        instrument = ws.getInstrument()
        det = instrument.getDetector(det_id)
        frame = instrument.getReferenceFrame()
        z_axis = frame.vecPointingAlongBeam()
        return np.degrees(det.getTwoTheta(instrument.getPos(), z_axis))


# ------------------------------------------------------------------------------------------------------


class SaveHKLFormat(object):

    def __call__(self, file_name, workspace):
        SaveHKL(Filename=file_name, InputWorkspace=workspace, OutputWorkspace=workspace.name())


AlgorithmFactory.subscribe(SaveReflections)
