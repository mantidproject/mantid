# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
import os.path as osp
import numpy as np
from mantid.api import AlgorithmFactory, FileProperty, FileAction, IPeaksWorkspaceProperty, PythonAlgorithm
from mantid.kernel import StringListValidator, Direction, logger, EnabledWhenProperty, PropertyCriterion
from mantid.simpleapi import FilterPeaks
from enum import Enum


def num_modulation_vectors(workspace):
    """Check if the workspace has any modulation vectors stored and
    return the number if any

    :params: workspace :: the workspace to check
    :returns: The number of stored modulation vectors
    """
    sample = workspace.sample()
    if sample.hasOrientedLattice():
        lattice = workspace.sample().getOrientedLattice()
        count = 0
        for i in range(3):
            vec = lattice.getModVec(i)
            if abs(vec.X()) > 0 or abs(vec.Y()) or abs(vec.Z()):
                count += 1
        return count
    else:
        return 0


def get_two_theta(dspacing, wavelength):
    """Get the two theta value for this peak.

    This is just Bragg's law relating wavelength to scattering angle.
    :param dspacing: DSpacing of a peak
    :param wavelength: Wavelength of a peak
    :returns: the scattering angle for the peak.
    """
    theta = 2.0 * np.arcsin(0.5 * (wavelength / dspacing))
    return np.rad2deg(theta)


def has_modulated_indexing(workspace):
    """Check if this workspace has more than 3 indices

    :params: workspace :: the workspace to check
    :returns: True if the workspace > 3 indices else False
    """
    return num_modulation_vectors(workspace) > 0


def get_intHKLM(peak, workspace):
    """Get the HKL to write to file (of parent peak if a satellite) and mnp (empty if no modulation)

    :params: peak :: peak object from workspace
    :params: workspace :: the peak workspace
    :returns: list containing hkl to write (hkl of the parent if a satellite peak)
    :returns: list of mnp (empty if not a satellite peak)
    """
    hkl = peak.getHKL()
    mnp = []
    if has_modulated_indexing(workspace):
        lattice = workspace.sample().getOrientedLattice()
        num_mod_vec = num_modulation_vectors(workspace)
        mnp = list(peak.getIntMNP())[:num_mod_vec]  # +/- 1 for one of up to 3 mod vecs (0 otherwise)
        for ivec in range(num_mod_vec):
            if abs(mnp[ivec]) > 1e-10:
                # undo modulation to get integer HKL (can't round if cell non-primitive and mod component > 0.5)
                mod_vec = lattice.getModVec(ivec)
                hkl -= mod_vec * mnp[ivec]
                break
    return list(hkl), mnp


class SaveReflections(PythonAlgorithm):
    def category(self):
        return "DataHandling\\Text;Crystal\\DataHandling"

    def summary(self):
        return "Saves single crystal reflections to a variety of formats"

    def PyInit(self):
        """Initilize the algorithms properties"""

        self.declareProperty(IPeaksWorkspaceProperty("InputWorkspace", "", Direction.Input), doc="The name of the peaks worksapce to save")

        self.declareProperty(
            FileProperty("Filename", "", action=FileAction.Save, direction=Direction.Input),
            doc="File with the data from a phonon calculation.",
        )

        self.declareProperty(
            name="Format",
            direction=Direction.Input,
            defaultValue="Fullprof",
            validator=StringListValidator([fmt.name for fmt in ReflectionFormat]),
            doc="The output format to export reflections to",
        )

        self.declareProperty(
            name="SplitFiles",
            defaultValue=False,
            direction=Direction.Input,
            doc="If True save separate files with only the peaks associated"
            "with a single modulation vector in a single file. Only "
            "applies to JANA format.",
        )

        self.declareProperty(
            name="MinIntensOverSigma",
            defaultValue=0.0,
            direction=Direction.Input,
            doc="Will only save peaks with intensity/sigma ratio greater than e or equal to MinIntensOverSigma.",
        )

        self.declareProperty(
            name="SeparateBatchNumbers",
            defaultValue=False,
            direction=Direction.Input,
            doc="If True all peaks from all runs will be labelled with the same batch number (in SHELX) or COD in"
            "Jana/Fullprof - i.e. same scale factor will be used for all runs. If False then a different scale "
            "factor will be used for each run number in the peak table. This option does not apply to GSAS format.",
        )
        not_gsas = EnabledWhenProperty("Format", PropertyCriterion.IsNotEqualTo, ReflectionFormat.GSAS.name)
        self.setPropertySettings("SeparateBatchNumbers", not_gsas)

    def PyExec(self):
        """Execute the algorithm"""
        workspace = self.getProperty("InputWorkspace").value
        output_format = ReflectionFormat[self.getPropertyValue("Format")]
        file_name = self.getPropertyValue("Filename")
        split_files = self.getProperty("SplitFiles").value
        min_i_over_sig = self.getProperty("MinIntensOverSigma").value

        # apply I/sigma filter
        filtered_workspace = FilterPeaks(
            InputWorkspace=workspace,
            FilterVariable="Signal/Noise",
            FilterValue=min_i_over_sig,
            Operator=">=",
            EnableLogging=False,
            StoreInADS=False,
        )
        # set title of workspace (written to header in fullprof format)
        filtered_workspace.setTitle(workspace.getTitle() if workspace.getTitle() else workspace.name())

        # find the max intensity so fits in column with format 12.2f in Fullprof and Jana, 8.2f in SaveHKL (SHELX, GSAS)
        scale = 1
        if filtered_workspace.getNumberPeaks() > 0:
            max_intens = max(filtered_workspace.column("Intens"))
            max_exponent = 8 if output_format in [ReflectionFormat.Fullprof, ReflectionFormat.Jana] else 4
            min_exponent = -2  # 2 decimal points in SHELX, FullProf, Jana2006 and GSAS-II
            if max_intens >= 10**max_exponent:
                # find scale factor to scale intensity to largest value in the available width (e.g. 9999.99 for SHELX)
                scale = ((10**max_exponent) - 10**min_exponent) / max_intens
        else:
            logger.warning(
                f"There are no peaks with Intens/Sigma >= {min_i_over_sig} in peak workspace {workspace.name()}. "
                f"An empty file will be produced."
            )
        # get batch numebrs using run numbers
        if output_format != ReflectionFormat.GSAS:
            if self.getProperty("SeparateBatchNumbers").value:
                _, batch_nums = np.unique(filtered_workspace.column("RunNumber"), return_inverse=True)
            else:
                batch_nums = np.ones(filtered_workspace.getNumberPeaks(), dtype=int)

        # scale intensity, sigma and batch num
        for ipk, peak in enumerate(filtered_workspace):
            peak.setIntensity(peak.getIntensity() * scale)
            peak.setSigmaIntensity(peak.getSigmaIntensity() * scale)
            if output_format != ReflectionFormat.GSAS:
                peak.setRunNumber(int(batch_nums[ipk]))

        FORMAT_MAP[output_format]()(file_name, filtered_workspace, split_files)


# ------------------------------------------------------------------------------------------------------


class FullprofFormat(object):
    """Writes a PeaksWorkspace to an ASCII file in the format required
    by the Fullprof crystallographic refinement program.

    This is a 7 columns file format consisting of H, K, L, intensity,
    sigma, crystal domain, and wavelength.
    """

    def __call__(self, file_name, workspace, split_files):
        """Write a PeaksWorkspace to an ASCII file using this formatter.

        :param file_name: the file name to output data to.
        :param workspace: the PeaksWorkspace to write to file.
        :param _: Ignored parameter for compatibility with other savers
        """
        with open(file_name, "w") as f_handle:
            self.write_header(f_handle, workspace)
            self.write_peaks(f_handle, workspace)

    def write_header(self, f_handle, workspace):
        """Write the header of the Fullprof file format

        :param f_handle: handle to the file to write to.
        :param workspace: the PeaksWorkspace to save to file.
        """
        num_hkl = 3 + has_modulated_indexing(workspace)  # add a column if mod vectors
        title = workspace.getTitle() if workspace.getTitle() else workspace.name()
        f_handle.write(title + "\n")
        f_handle.write("({}i4,2f12.2,i5,4f10.4)\n".format(num_hkl))
        wavelength = "0"  # if TOF Laue this is ignored
        if np.std([pk.getWavelength() for pk in workspace]) < 0.01:
            # check for constant wavelength (same as in SaveHKLCW)
            wavelength = f"{workspace.getPeak(0).getWavelength():.5f}"
        f_handle.write("  {} 0 0\n".format(wavelength))
        mod_colname = ""
        if has_modulated_indexing(workspace):
            # num_rows = 2*num_vecs (separate rows for +/- q)
            f_handle.write("   {:>4.0f}\n".format(2 * num_modulation_vectors(workspace)))
            # now write out mod vectors
            lattice = workspace.sample().getOrientedLattice()
            row_num = 1
            for ivec in range(num_modulation_vectors(workspace)):
                vec = lattice.getModVec(ivec)
                x, y, z = vec.X(), vec.Y(), vec.Z()
                if abs(x) > 0 or abs(y) > 0 or abs(z) > 0:
                    f_handle.write("   {}{: >13.6f}{: >13.6f}{: >13.6f}\n".format(row_num, x, y, z))
                    f_handle.write("   {}{: >13.6f}{: >13.6f}{: >13.6f}\n".format(row_num + 1, -x, -y, -z))
                    row_num += 2
            mod_colname = "   m"
        f_handle.write("#  h   k   l{}      Fsqr       s(Fsqr)   Cod   Lambda\n".format(mod_colname))

    def write_peaks(self, f_handle, workspace):
        """Write all the peaks in the workspace to file.

        :param f_handle: handle to the file to write to.
        :param workspace: the PeaksWorkspace to save to file.
        """
        for i, peak in enumerate(workspace):
            hkl, mnp = get_intHKLM(peak, workspace)
            if mnp:
                if all([abs(m) < 1e-10 for m in mnp]):
                    # modulations present in ws but this is not a satellite peak
                    iq = [0]
                else:
                    # find num of mod vector as written in header
                    iq = [2 * im + 1 + (m < 0) for im, m in enumerate(mnp) if (abs(m) > 1e-10)]
                hkl.extend(iq)
            hkls = "".join(["{:>4.0f}".format(item) for item in hkl])

            data = (peak.getIntensity(), peak.getSigmaIntensity(), peak.getRunNumber(), peak.getWavelength())
            line = "{:>12.2f}{:>12.2f}{:>5.0f}{:>10.4f}\n".format(*data)
            line = "".join([hkls, line])

            f_handle.write(line)


# ------------------------------------------------------------------------------------------------------


class JanaFormat(object):
    """Writes a PeaksWorkspace to an ASCII file in the format required
    by the Jana2006 crystallographic refinement program.

    This is an 11 column file format consisting of H, K, L, intensity, sigma,
    crystal domain, wavelength, 2*theta, transmission, absorption weighted path length (Tbar),
    and thermal diffuse scattering correction (TDS).

    Currently the last three columns are hard coded to 1.0, 0.0, and 0.0 respectively.
    """

    class FileBuilder(object):
        """Encapsulate information to build a single Jana file"""

        def __init__(self, filepath, workspace, num_mod_vec, modulation_col_num=None):
            self._filepath = filepath
            self._workspace = workspace
            self._num_mod_vec = num_mod_vec
            self._modulation_col_num = modulation_col_num

            self._headers = []
            self._peaks = []
            self._num_cols = None

        def build_headers(self):
            headers = self._headers
            sample = self._workspace.sample()
            lattice = sample.getOrientedLattice() if sample.hasOrientedLattice() else None

            def append_mod_vector(index, col_num):
                vec = lattice.getModVec(index)
                x, y, z = vec.X(), vec.Y(), vec.Z()
                if abs(x) > 0 or abs(y) > 0 or abs(z) > 0:
                    headers.append("       {}{: >13.6f}{: >13.6f}{: >13.6f}\n".format(col_num, x, y, z))

            # propagation vector information if required
            if lattice is not None:
                if self._num_mod_vec > 0:
                    headers.append("# Structural propagation vectors used\n")
                    if self._modulation_col_num is not None:
                        headers.append("           1\n")
                        append_mod_vector(self._modulation_col_num - 1, 1)
                        headers.append("# Linear combination table used\n")
                        headers.append("           2\n")
                        headers.append("       1       1\n")
                        headers.append("       2      -1\n")
                    else:
                        headers.append("           {}\n".format(self._num_mod_vec))
                        for mod_vec_index in range(3):
                            append_mod_vector(mod_vec_index, mod_vec_index + 1)
                # lattice parameters
                lattice_params = [lattice.a(), lattice.b(), lattice.c(), lattice.alpha(), lattice.beta(), lattice.gamma()]
                lattice_params = "".join(["{: >10.4f}".format(value) for value in lattice_params])
                headers.append("# Lattice parameters   {}\n".format(lattice_params))
            # column headers and format
            column_names = ["h", "k", "l"]
            if self._modulation_col_num is not None:
                modulated_cols = ["m1"]
                headers.append("(3i5,1i5,2f12.2,i5,4f10.4)\n")
            else:
                modulated_cols = ["m{}".format(i + 1) for i in range(self._num_mod_vec)]
                headers.append("(3i5," + num_modulation_vectors(self._workspace) * "1i5," + "2f12.2,i5,4f10.4)\n")
            column_names.extend(modulated_cols)
            column_names.extend(["Fsqr", "s(Fsqr)", "Cod", "Lambda", "Twotheta", "Transm.", "Tbar", "TDS"])

            column_format = "#{:>4}{:>4}{:>4}"
            column_format += "".join(["{:>4}" for _ in range(len(modulated_cols))])
            column_format += "{:>12}{:>12}{:>5}{:>10}{:>10}{:>10}{:>10}{:>10}\n"
            headers.append(column_format.format(*column_names))
            self._num_cols = len(column_names)

        def build_peaks_info(self):
            for peak in self._workspace:
                if self._num_mod_vec > 0:
                    # if this is a main peak write it out. if not decide if it should be in this file
                    hkl, mnp = get_intHKLM(peak, self._workspace)
                    if self._modulation_col_num is None:
                        # write all modulation indices
                        modulation_indices = [mnp[i] for i in range(self._num_mod_vec)]
                    else:
                        # is this a main peak or one with the modulation vector matching this file
                        mnp_index = -1
                        for i in range(self._num_mod_vec):
                            if abs(mnp[i]) > 0.0:
                                mnp_index = i
                                break
                        if mnp_index < 0:
                            # all(mnp) == 0: main peak
                            modulation_indices = [0]
                        elif mnp_index == self._modulation_col_num - 1:
                            # mnp index matches this file
                            modulation_indices = [mnp[mnp_index]]
                        else:
                            # mnp doesn't match this file
                            continue
                else:
                    # no modulated structure information
                    hkl = peak.getHKL()
                    modulation_indices = []
                self._peaks.append(
                    self.create_peak_line(
                        hkl,
                        modulation_indices,
                        peak.getIntensity(),
                        peak.getSigmaIntensity(),
                        peak.getRunNumber(),
                        peak.getWavelength(),
                        get_two_theta(peak.getDSpacing(), peak.getWavelength()),
                        peak.getAbsorptionWeightedPathLength(),
                    )
                )

        def create_peak_line(self, hkl, mnp, intensity, sig_int, batch_num, wavelength, two_theta, t_bar):
            """
            Write the raw peak data to a file.

            :param f_handle: handle to the file to write to.
            :param hkl: Integer HKL indices
            :param mnp: List of mnp values
            :param intensity: Intensity of the peak
            :param sig_int: Signal value
            :param batch_num: Or ScaleID written in COD column
            :param wavelength: Wavelength in angstroms
            :param two_theta: Two theta of detector
            :param t_bar: absorption weighted path length for the detector/wavelength
            """
            template = "{: >5.0f}{: >5.0f}{: >5.0f}{}{: >12.2f}{: >12.2f}{: >5.0f}{: >10.4f}{: >10.4f}{: >10.4f}{: >10.4f}{: >10.4f}\n"
            mod_indices = "".join(["{: >5.0f}".format(value) for value in mnp])
            return template.format(
                hkl[0],
                hkl[1],
                hkl[2],
                mod_indices,
                intensity,
                sig_int,
                batch_num,
                wavelength,
                two_theta,
                1.0,
                t_bar,
                0.0,
            )

        def write(self):
            with open(self._filepath, "w") as handle:
                handle.write("".join(self._headers))
                handle.write("".join(self._peaks))

    def __call__(self, file_name, workspace, split_files):
        """Write a PeaksWorkspace or TableWorkspace with the appropriate columns
        to an ASCII file using this formatter.

        :param file_name: the file name to output data to.
        :param workspace: the PeaksWorkspace to write to file.
        :param split_files: if True the peaks associated with each
                            modulation vector are saved to separate
                            files. The suffix -mi where identifier=1,2...n
                            is appended to each file
        """
        builders = self._create_file_builders(file_name, workspace, split_files)
        for builder in builders:
            builder.build_headers()
            builder.build_peaks_info()
            builder.write()

    def _create_file_builders(self, file_name, workspace, split_files):
        """Create a sequence of JanaFileBuilder to contain the information.

        :param file_name: Filename given by user
        :param workspace: the PeaksWorkspace to write to file.
        :param split_files: If true create a file for each modulation vector
                            if there are more than 1
        """
        num_mod_vec = num_modulation_vectors(workspace)
        if split_files and num_mod_vec > 1:
            name, ext = osp.splitext(file_name)
            builders = [
                JanaFormat.FileBuilder("{}-m{}{}".format(name, col_num, ext), workspace, num_mod_vec, col_num)
                for col_num in range(1, num_mod_vec + 1)
            ]
        else:
            builders = [JanaFormat.FileBuilder(file_name, workspace, num_mod_vec, None)]

        return builders


# ------------------------------------------------------------------------------------------------------


class SHELXFormat(object):
    """Writes a PeaksWorkspace to an ASCII file in the format required
    by the SHELX crystallographic refinement program.
    For constant wavelength workspaces this is an ASCII file
    consisting of 4 columns: H, K, L, intensity, sigma.
    For TOF Laue there are two extra columns: scaleID and wavelength.
    """

    def __call__(self, file_name, workspace, split_files):
        """
        Write a PeaksWorkspace to an ASCII file using this formatter.
        :param file_name: the file name to output data to.
        :param workspace: the PeaksWorkspace to write to file.
        """
        if has_modulated_indexing(workspace):
            raise RuntimeError("Cannot currently save modulated structures to GSAS or SHELX formats")

        with open(file_name, "w") as f_handle:
            self.write_peaks(f_handle, workspace)

    def write_peaks(self, f_handle, workspace):
        """Write all the peaks in the workspace to file.

        :param f_handle: handle to the file to write to.
        :param workspace: the PeaksWorkspace to save to file.
        """
        col_format = "{:>4.0f}{:>4.0f}{:>4.0f}{:>8.2f}{:>8.2f}"  # H, K, L, Intens, Sig
        is_CW = np.std([pk.getWavelength() for pk in workspace]) < 0.01  # constant wavelength
        if not is_CW:
            col_format += "{:>4.0f}{:>8.4f}"  # scaleID, wavelength
        col_format += "\n"
        for i, peak in enumerate(workspace):
            hkl, _ = get_intHKLM(peak, workspace)  # no mnp as not modulated
            data = hkl + [peak.getIntensity(), peak.getSigmaIntensity()]
            if not is_CW:
                data.extend([peak.getRunNumber(), peak.getWavelength()])  # ScaleID (from run number), wavelength
            line = col_format.format(*data)
            f_handle.write(line)
        # write row of zeros to end file
        f_handle.write(col_format.format(*col_format.count("{") * [0.0]))


# ------------------------------------------------------------------------------------------------------


class SaveHKLFormat(object):
    """Writes a PeaksWorkspace to an ASCII file in the format output from
    the SaveHKL algorithm.

    The SaveHKL algorithm currently supports both the GSAS and SHELX formats. For
    more information see the SaveHKL algorithm documentation.
    """

    def __call__(self, file_name, workspace, _):
        """Write a PeaksWorkspace to an ASCII file using this formatter.

        :param file_name: the file name to output data to.
        :param workspace: the PeaksWorkspace to write to file.
        :param _: Ignored parameter for compatability with other savers
        """
        if has_modulated_indexing(workspace):
            raise RuntimeError("Cannot currently save modulated structures to GSAS or SHELX formats")

        from mantid.dataobjects import LeanElasticPeaksWorkspace

        if isinstance(workspace, LeanElasticPeaksWorkspace):
            from mantid.simpleapi import SaveHKLCW

            SaveHKLCW(OutputFile=file_name, Workspace=workspace)
        else:
            from mantid.simpleapi import SaveHKL

            SaveHKL(Filename=file_name, InputWorkspace=workspace)


class ReflectionFormat(Enum):
    Fullprof = 1
    GSAS = 2
    Jana = 3
    SHELX = 4


FORMAT_MAP = {
    ReflectionFormat.Fullprof: FullprofFormat,
    ReflectionFormat.GSAS: SaveHKLFormat,
    ReflectionFormat.Jana: JanaFormat,
    ReflectionFormat.SHELX: SHELXFormat,
}

AlgorithmFactory.subscribe(SaveReflections)
