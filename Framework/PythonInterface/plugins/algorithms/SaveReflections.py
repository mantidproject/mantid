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
    """Writes a PeaksWorkspace to an ASCII file in the format required
    by the Fullprof crystallographic refinement program.

    This is a 7 columns file format consisting of H, K, L, instensity,
    sigma, crystal domain, and wavelength.
    """

    def __call__(self, file_name, workspace):
        """Write a PeaksWorkspace to an ASCII file using this formatter.

        :param file_name: the file name to output data to.
        :param workspace: the PeaksWorkspace to write to file.
        """
        with open(file_name, 'w') as f_handle:
            self.write_header(f_handle, workspace)
            self.write_peaks(f_handle, workspace)

    def write_header(self, f_handle, workspace):
        """Write the header of the Fullprof file format

        :param f_handle: handle to the file to write to.
        :param workspace: the PeaksWorkspace to save to file.
        """
        f_handle.write(workspace.getTitle())
        f_handle.write("(3i4,2f12.2,i5,4f10.4)\n")
        f_handle.write("  0 0 0\n")
        f_handle.write("#  h   k   l      Fsqr       s(Fsqr)   Cod   Lambda\n")

    def write_peaks(self, f_handle, workspace):
        """Write all the peaks in the workspace to file.

        :param f_handle: handle to the file to write to.
        :param workspace: the PeaksWorkspace to save to file.
        """
        for i, peak in enumerate(workspace):
            data = (peak['h'],peak['k'],peak['l'],peak['Intens'],peak['SigInt'],i+1,peak['Wavelength'])
            line = "%4i%4i%4i%12.2f%12.2f%5i%10.4f\n" % data
            f_handle.write(line)

# ------------------------------------------------------------------------------------------------------


class JanaFormat(object):
    """Writes a PeaksWorkspace to an ASCII file in the format required
    by the Jana2006 crystallographic refinement program.

    This is an 11 column file format consisting of H, K, L, intensity, sigma, 
    crystal domain, wavelength, 2*theta, transmission, absorption weighted path length (Tbar),
    and thermal diffuse scattering correction (TDS).

    Currently the last three columns are shard coded to 1.0, 0.0, and 0.0 respectively.
    """

    def __call__(self, file_name, workspace):
        """Write a PeaksWorkspace to an ASCII file using this formatter.

        :param file_name: the file name to output data to.
        :param workspace: the PeaksWorkspace to write to file.
        """
        self._cache_instrument_params(workspace)
        with open(file_name, 'w') as f_handle:
            self.write_header(f_handle, workspace)
            self.write_peaks(f_handle, workspace)

    def write_header(self, f_handle, workspace):
        """Write the header of the Fullprof file format

        :param f_handle: handle to the file to write to.
        :param workspace: the PeaksWorkspace to save to file.
        """
        sample = workspace.sample()
        lattice = sample.getOrientedLattice()
        lattice_params = [lattice.a(), lattice.b(), lattice.c(), lattice.alpha(), lattice.beta(), lattice.gamma()]
        lattice_params = "".join(["{: >10.4f}".format(value) for value in lattice_params])
        f_handle.write("# Lattice parameters   {}\n".format(lattice_params))
        f_handle.write("(3i5,2f12.2,i5,4f10.4)\n")

    def write_peaks(self, f_handle, workspace):
        """Write all the peaks in the workspace to file.

        :param f_handle: handle to the file to write to.
        :param workspace: the PeaksWorkspace to save to file.
        """
        column_names = ["h", "k", "l", "Fsqr", "s(Fsqr)", "Cod", "Lambda", "Twotheta", "Transm.", "Tbar", "TDS"]
        column_format = "#{:>4}{:>4}{:>4}{:>12}{:>12}{:>5}{:>10}{:>10}{:>10}{:>10}{:>10}\n"
        f_handle.write(column_format.format(*column_names))
        for row in workspace:
            self.write_peak(f_handle, row)

    def write_peak(self, f_handle, peak):
        """Write a single Peak from the peaks workspace to file.

        :param f_handle: handle to the file to write to.
        :param peak: the peak object to write to the file.
        """
        f_handle.write("{h: >5.0f}{k: >5.0f}{l: >5.0f}".format(**peak))
        f_handle.write("{Intens: >12.2f}".format(**peak))
        f_handle.write("{SigInt: >12.2f}".format(**peak))
        f_handle.write("{: >5.0f}".format(1))
        f_handle.write("{Wavelength: >10.4f}".format(**peak))
        f_handle.write("{: >10.4f}".format(self._get_two_theta(peak['DetID'])))
        f_handle.write("{: >10.4f}{: >10.4f}{: >10.4f}".format(1.0, 0.0, 0.0))
        f_handle.write("\n")

    def _get_two_theta(self, det_id):
        """Get the two theta value for this peak
        :param det_id: the detector ID of this Peak
        """
        det = self._instrument.getDetector(det_id)
        return np.degrees(det.getTwoTheta(self._instrument.getPos(), self._z_axis))

    def _cache_instrument_params(self, workspace):
        """Cache some parameters about the instrument

        This stores some of the instrument parameters at the start
        of execution so we can quickly access them later.

        :param workspace: the PeaksWorkspace to cache instrument data from.
        """
        self._instrument = workspace.getInstrument()
        frame = self._instrument.getReferenceFrame()
        self._z_axis = frame.vecPointingAlongBeam()


# ------------------------------------------------------------------------------------------------------


class SaveHKLFormat(object):
    """Writes a PeaksWorkspace to an ASCII file in the format output from
    the SaveHKL algorithm.

    The SaveHKL algorithm currently supports both the GSAS and SHELX formats. For
    more information see the SaveHKL algorithm documentation.
    """

    def __call__(self, file_name, workspace):
        """Write a PeaksWorkspace to an ASCII file using this formatter.

        :param file_name: the file name to output data to.
        :param workspace: the PeaksWorkspace to write to file.
        """
        SaveHKL(Filename=file_name, InputWorkspace=workspace, OutputWorkspace=workspace.name())


AlgorithmFactory.subscribe(SaveReflections)
