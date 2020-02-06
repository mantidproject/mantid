# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
#     NScD Oak Ridge National Laboratory, European Spallation Source
#     & Institut Laue - Langevin
# SPDX - License - Identifier: GPL - 3.0 +
from __future__ import (absolute_import, division, unicode_literals)

import os.path as osp
import numpy as np
from mantid.api import (AlgorithmFactory, FileProperty, FileAction, IPeaksWorkspaceProperty, PythonAlgorithm)
from mantid.kernel import StringListValidator, Direction
from mantid.py3compat.enum import Enum


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
    theta = 2. * np.arcsin(0.5 * (wavelength / dspacing))
    return np.rad2deg(theta)


def has_modulated_indexing(workspace):
    """Check if this workspace has more than 3 indices

    :params: workspace :: the workspace to check
    :returns: True if the workspace > 3 indices else False
    """
    return num_modulation_vectors(workspace) > 0


def modulation_indices(peak, num_mod_vec):
    """
    Gather non-zero modulated structure indices from a peak

    :param workspace: A single Peak
    :param num_mod_vec: The number of modulation vectors set on the workspace
    :return: A list of the modulation indices
    """
    mnp = peak.getIntMNP()
    return [mnp[i] for i in range(num_mod_vec)]


def get_additional_index_names(workspace):
    """Get the names of the additional indices to export

    :params: workspace :: the workspace to get column names from
    :returns: the names of any additional columns in the workspace
    """
    num_mod_vec = num_modulation_vectors(workspace)
    return ["m{}".format(i + 1) for i in range(num_mod_vec)]


class SaveReflections(PythonAlgorithm):
    def category(self):
        return "DataHandling\\Text;Crystal\\DataHandling"

    def summary(self):
        return "Saves single crystal reflections to a variety of formats"

    def PyInit(self):
        """Initilize the algorithms properties"""

        self.declareProperty(IPeaksWorkspaceProperty("InputWorkspace", '', Direction.Input),
                             doc="The name of the peaks worksapce to save")

        self.declareProperty(FileProperty("Filename",
                                          "",
                                          action=FileAction.Save,
                                          direction=Direction.Input),
                             doc="File with the data from a phonon calculation.")

        self.declareProperty(name="Format",
                             direction=Direction.Input,
                             defaultValue="Fullprof",
                             validator=StringListValidator(dir(ReflectionFormat)),
                             doc="The output format to export reflections to")

        self.declareProperty(name="SplitFiles",
                             defaultValue=False,
                             direction=Direction.Input,
                             doc="If True save separate files with only the peaks associated"
                             "with a single modulation vector in a single file. Only "
                             "applies to JANA format.")

    def PyExec(self):
        """Execute the algorithm"""
        workspace = self.getProperty("InputWorkspace").value
        output_format = ReflectionFormat[self.getPropertyValue("Format")]
        file_name = self.getPropertyValue("Filename")
        split_files = self.getProperty("SplitFiles").value

        FORMAT_MAP[output_format]()(file_name, workspace, split_files)


# ------------------------------------------------------------------------------------------------------


class FullprofFormat(object):
    """Writes a PeaksWorkspace to an ASCII file in the format required
    by the Fullprof crystallographic refinement program.

    This is a 7 columns file format consisting of H, K, L, instensity,
    sigma, crystal domain, and wavelength.
    """
    def __call__(self, file_name, workspace, split_files):
        """Write a PeaksWorkspace to an ASCII file using this formatter.

        :param file_name: the file name to output data to.
        :param workspace: the PeaksWorkspace to write to file.
        :param _: Ignored parameter for compatability with other savers
        """
        with open(file_name, 'w') as f_handle:
            self.write_header(f_handle, workspace)
            self.write_peaks(f_handle, workspace)

    def write_header(self, f_handle, workspace):
        """Write the header of the Fullprof file format

        :param f_handle: handle to the file to write to.
        :param workspace: the PeaksWorkspace to save to file.
        """
        num_hkl = 3 + num_modulation_vectors(workspace)
        f_handle.write(workspace.getTitle())
        f_handle.write("({}i4,2f12.2,i5,4f10.4)\n".format(num_hkl))
        f_handle.write("  0 0 0\n")
        names = "".join(["  {}".format(name) for name in get_additional_index_names(workspace)])
        f_handle.write("#  h   k   l{}      Fsqr       s(Fsqr)   Cod   Lambda\n".format(names))

    def write_peaks(self, f_handle, workspace):
        """Write all the peaks in the workspace to file.

        :param f_handle: handle to the file to write to.
        :param workspace: the PeaksWorkspace to save to file.
        """
        num_mod_vec = num_modulation_vectors(workspace)
        for i, peak in enumerate(workspace):
            data = [peak.getH(), peak.getK(), peak.getL()]
            data.extend(modulation_indices(peak, num_mod_vec))
            hkls = "".join(["{:>4.0f}".format(item) for item in data])

            data = (peak.getIntensity(), peak.getSigmaIntensity(), i + 1, peak.getWavelength())
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
                    headers.append("       {}{: >13.6f}{: >13.6f}{: >13.6f}\n".format(
                        col_num, x, y, z))

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
                lattice_params = [
                    lattice.a(),
                    lattice.b(),
                    lattice.c(),
                    lattice.alpha(),
                    lattice.beta(),
                    lattice.gamma()
                ]
                lattice_params = "".join(["{: >10.4f}".format(value) for value in lattice_params])
                headers.append("# Lattice parameters   {}\n".format(lattice_params))
            headers.append("(3i5,2f12.2,i5,4f10.4)\n")
            # column headers
            column_names = ["h", "k", "l"]
            if self._modulation_col_num is not None:
                modulated_cols = ["m1"]
            else:
                modulated_cols = ["m{}".format(i + 1) for i in range(self._num_mod_vec)]
            column_names.extend(modulated_cols)
            column_names.extend(
                ["Fsqr", "s(Fsqr)", "Cod", "Lambda", "Twotheta", "Transm.", "Tbar", "TDS"])

            column_format = "#{:>4}{:>4}{:>4}"
            column_format += "".join(["{:>4}" for _ in range(len(modulated_cols))])
            column_format += "{:>12}{:>12}{:>5}{:>10}{:>10}{:>10}{:>10}{:>10}\n"
            headers.append(column_format.format(*column_names))
            self._num_cols = len(column_names)

        def build_peaks_info(self):
            for peak in self._workspace:
                if self._num_mod_vec > 0:
                    # if this is a main peak write it out. if not decide if it should be in this file
                    hkl = peak.getIntHKL()
                    mnp = peak.getIntMNP()
                    if self._modulation_col_num is None:
                        # write all modulation indices
                        modulation_indices = [mnp[i] for i in range(self._num_mod_vec)]
                    else:
                        # is this a main peak or one with the modulation vector matching this file
                        mnp_index = -1
                        for i in range(3):
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
                    self.create_peak_line(hkl, modulation_indices,
                                          peak.getIntensity(), peak.getSigmaIntensity(),
                                          peak.getWavelength(),
                                          get_two_theta(peak.getDSpacing(), peak.getWavelength())))

        def create_peak_line(self, hkl, mnp, intensity, sig_int, wavelength, two_theta):
            """
            Write the raw peak data to a file.

            :param f_handle: handle to the file to write to.
            :param hkl: Integer HKL indices
            :param mnp: List of mnp values
            :param intensity: Intensity of the peak
            :param sig_int: Signal value
            :param wavelength: Wavelength in angstroms
            :param two_theta: Two theta of detector
            """
            template = "{: >5.0f}{: >5.0f}{: >5.0f}{}{: >12.2f}{: >12.2f}{: >5.0f}{: >10.4f}{: >10.4f}{: >10.4f}{: >10.4f}{: >10.4f}\n"
            mod_indices = "".join(["{: >5.0f}".format(value) for value in mnp])
            return template.format(hkl[0], hkl[1], hkl[2], mod_indices, intensity, sig_int, 1, wavelength,
                                   two_theta, 1.0, 0.0, 0.0)

        def write(self):
            with open(self._filepath, 'w') as handle:
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
                JanaFormat.FileBuilder('{}-m{}{}'.format(name, col_num, ext), workspace,
                                       num_mod_vec, col_num)
                for col_num in range(1, num_mod_vec + 1)
            ]
        else:
            builders = [JanaFormat.FileBuilder(file_name, workspace, num_mod_vec, None)]

        return builders


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
            raise RuntimeError(
                "Cannot currently save modulated structures to GSAS or SHELX formats")

        from mantid.simpleapi import SaveHKL
        SaveHKL(Filename=file_name, InputWorkspace=workspace, OutputWorkspace=workspace.name())


class ReflectionFormat(Enum):
    Fullprof = 1
    GSAS = 2
    Jana = 3
    SHELX = 4


FORMAT_MAP = {
    ReflectionFormat.Fullprof: FullprofFormat,
    ReflectionFormat.GSAS: SaveHKLFormat,
    ReflectionFormat.Jana: JanaFormat,
    ReflectionFormat.SHELX: SaveHKLFormat
}

AlgorithmFactory.subscribe(SaveReflections)
