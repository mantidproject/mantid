# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2020 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +

"""
This file was last edited by Giovanni Romanelli on 23/05/2020.
The structure of the script is as follows:
    First, the collection of functions allowing the reduction and analysis of the spectra;
        Second, a list of input parameters specific of the VESUVIO experiment;
            Last, the reduction and analysis procedure, iterative in TOF and finally in y-space for hydrogen.

The script has been tested to be run in Mantid Workbench in a Windows operative system.

PLEASE, DO NOT MODIFY THE "TECHNICAL SECTION" UNLESS YOU ARE AN
EXPERT VESUVIO INSTRUMENT SCIENTIST.
"""

##########################################################
####        TECHNICAL SECTION - NOT FOR USERS
##########################################################

from __future__ import absolute_import, division, print_function, unicode_literals


import numpy as np
import mantid.simpleapi as sapi
from scipy import optimize

#
#   INITIALISING FUNCTIONS AND USEFUL FUNCTIONS
#

"""
A wrapper for the mantid logger to make it take inputs the same way
as a Python print statement

debug is equivalent to verbose.
"""


class logger:
    def __init__(self, g_log):
        self._log = g_log

    def get_string(self, *args):
        string = ""
        for msg in args:
            string += str(msg) + " "
        return string

    def warning(self, *args):
        self._log.warning(self.get_string(args))

    def notice(self, *args):
        self._log.notice(self.get_string(args))

    def debug(self, *args):
        self._log.debug(self.get_string(args))


def safe_delete_ws(ws):
    if sapi.AnalysisDataService.doesExist(str(ws)):
        sapi.DeleteWorkspace(ws)
    return


def fun_gaussian(x, sigma):
    gaussian = np.exp(-(x**2) / 2 / sigma**2)
    gaussian /= np.sqrt(2.0 * np.pi) * sigma
    return gaussian


def fun_lorentzian(x, gamma):
    lorentzian = gamma / np.pi / (x**2 + gamma**2)
    return lorentzian


def fun_pseudo_voigt(x, sigma, gamma):  # input std gaussian hwhm lorentziana
    fg, fl = 2.0 * sigma * np.sqrt(2.0 * np.log(2.0)), 2.0 * gamma  # parameters transformed to gaussian and lorentzian FWHM
    f = 0.5346 * fl + np.sqrt(0.2166 * fl**2 + fg**2)
    eta = 1.36603 * fl / f - 0.47719 * (fl / f) ** 2 + 0.11116 * (fl / f) ** 3
    sigma_v, gamma_v = f / (2.0 * np.sqrt(2.0 * np.log(2.0))), f / 2.0
    pseudo_voigt = eta * fun_lorentzian(x, gamma_v) + (1.0 - eta) * fun_gaussian(x, sigma_v)
    # norm=np.sum(pseudo_voigt)*(x[1]-x[0])
    _ = np.sum(pseudo_voigt) * (x[1] - x[0])
    return pseudo_voigt  # /np.abs(norm)


def fun_derivative3(x, fun, g_log):  # Used to evaluate numerically the FSE term.
    derivative = np.zeros(len(fun))
    denom = np.zeros(len(fun))
    for i in range(6, len(fun) - 6):
        denom[i] = x[i + 1] - x[i]

    denom = np.power(denom, 3.0)
    indicies = [1, 2, 3, 4, 5, 6]
    factors = [-1584.0, 387.0, 488.0, -192.0, 24.0, -1.0]

    for i in range(6, len(fun) - 6):
        tmp = 0.0
        for j in range(len(indicies)):
            tmp += factors[j] * fun[i + indicies[j]] - factors[j] * fun[i - indicies[j]]
        if denom[i] != 0:
            derivative[i] = tmp / denom[i]
        else:
            g_log.warning("divide by zero")
    derivative = derivative / 1728.0  # 12**3

    return derivative


def fun_derivative4(x, fun):  # not used at present. Can be used for the H4 polynomial in TOF fitting.
    derivative = [0.0] * len(fun)
    for i in range(8, len(fun) - 8):
        derivative[i] = (
            fun[i - 8]
            - 32.0 * fun[i - 7]
            + 384 * fun[i - 6]
            - 2016.0 * fun[i - 5]
            + 3324.0 * fun[i - 4]
            + 6240.0 * fun[i - 3]
            - 16768 * fun[i - 2]
            - 4192.0 * fun[i - 1]
            + 26118.0 * fun[i]
        )
        derivative[i] += (
            fun[i + 8]
            - 32.0 * fun[i + 7]
            + 384 * fun[i + 6]
            - 2016.0 * fun[i + 5]
            + 3324.0 * fun[i + 4]
            + 6240.0 * fun[i + 3]
            - 16768 * fun[i + 2]
            - 4192.0 * fun[i + 1]
        )
        derivative[i] /= (x[i + 1] - x[i]) ** 4
    derivative = np.array(derivative) / (12**4)
    return derivative


def load_ip_file(spectrum, ipfile):
    f = open(ipfile, "r")
    data = f.read()
    lines = data.split("\n")
    for line in range(lines.__len__()):
        col = lines[line].split("\t")
        if col[0].isdigit() and int(col[0]) == spectrum:
            angle = float(col[2])
            # at present many VESUVIO scripts work only if a value of T0 is provided,
            # yet this is not included in the instrument definition file!
            T0 = float(col[3])
            L0 = float(col[4])
            L1 = float(col[5])
    f.close()
    return angle, T0, L0, L1


def load_resolution_parameters(spectrum):
    if spectrum > 134:  # resolution parameters for front scattering detectors, in case of single difference
        dE1 = 73.0  # meV , gaussian standard deviation
        dTOF = 0.37  # us
        dTheta = 0.016  # rad
        dL0 = 0.021  # meters
        dL1 = 0.023  # meters
        lorentzian_res_width = 24.0  # meV , HFHM
    if spectrum < 135:  # resolution parameters for back scattering detectors, in case of double difference
        dE1 = 88.7  # meV , gaussian standard deviation
        dTOF = 0.37  # us
        dTheta = 0.016  # rad
        dL0 = 0.021  # meters
        dL1 = 0.023  # meters
        lorentzian_res_width = 40.3  # meV , HFHM
    return dE1, dTOF, dTheta, dL0, dL1, lorentzian_res_width


def load_constants():
    mN = 1.008  # a.m.u.
    Ef = 4906.0  # meV
    en_to_vel = 4.3737 * 1.0e-4
    vf = np.sqrt(Ef) * en_to_vel  # m/us
    hbar = 2.0445
    return mN, Ef, en_to_vel, vf, hbar


def load_workspace(ws_name, spectrum):
    ws = sapi.mtd[str(ws_name)]
    ws_len, ws_spectra = ws.blocksize() - 1, ws.getNumberHistograms()
    ws_x, ws_y, ws_e = [0.0] * ws_len, [0.0] * ws_len, [0.0] * ws_len
    for spec in range(ws_spectra):
        if ws.getSpectrum(spec).getSpectrumNo() == spectrum:
            # mantid ws has convert to points
            # mantid.ConvertToPointData(InputWorkspace=ws.name(), OutputWorkspace=ws.name())
            for i in range(ws_len):
                # converting the histogram into points
                ws_y[i] = ws.readY(spec)[i] / (ws.readX(spec)[i + 1] - ws.readX(spec)[i])
                ws_e[i] = ws.readE(spec)[i] / (ws.readX(spec)[i + 1] - ws.readX(spec)[i])
                ws_x[i] = 0.5 * (ws.readX(spec)[i + 1] + ws.readX(spec)[i])
    ws_x, ws_y, ws_e = np.array(ws_x), np.array(ws_y), np.array(ws_e)
    return ws_x, ws_y, ws_e


#
#   FITTING FUNCTIONS
#


def block_fit_ncp(par, first_spectrum, last_spectrum, masses, ws_name, fit_arguments, verbose, IPFile, g_log):
    g_log.notice("\n" + "Fitting Workspace: " + str(ws_name))
    g_log.debug("Fitting parameters are given as: [Intensity Width Centre] for each NCP")
    widths = np.zeros((len(masses), last_spectrum - first_spectrum + 1))
    intensities = np.zeros((len(masses), last_spectrum - first_spectrum + 1))
    centres = np.zeros((len(masses), last_spectrum - first_spectrum + 1))
    spectra = np.zeros((last_spectrum - first_spectrum + 1))
    tof_fit_ws = sapi.CloneWorkspace(InputWorkspace=str(ws_name), OutputWorkspace=str(ws_name) + "_fit")
    j = 0
    for j, spectrum in enumerate(range(first_spectrum, last_spectrum + 1)):
        data_x, data_y, data_e = load_workspace(ws_name, spectrum)
        ncp, fitted_par, result = fit_ncp(par, spectrum, masses, data_x, data_y, data_e, fit_arguments, IPFile, g_log)
        for bin in range(len(data_x) - 1):
            tof_fit_ws.dataY(j)[bin] = ncp[bin] * (data_x[bin + 1] - data_x[bin])
            tof_fit_ws.dataE(j)[bin] = 0.0
        # Calculate the reduced chi2 from the fitting Cost function:
        reduced_chi2 = result.fun / (len(data_x) - len(par))
        if reduced_chi2 > 1.0e-3:
            g_log.debug(spectrum, fitted_par, "%.4g" % reduced_chi2)
        else:
            g_log.debug(spectrum, " ... skipping ...")
        npars = len(par) / len(masses)
        for m in range(len(masses)):
            if reduced_chi2 > 1.0e-3:
                index = int(npars * m)
                intensities[m][j] = float(fitted_par[index])
                widths[m][j] = float(fitted_par[index + 1])
                centres[m][j] = float(fitted_par[index + 2])
            else:
                widths[m][j] = None
                intensities[m][j] = None
                centres[m][j] = None

        spectra[j] = spectrum
    return spectra, widths, intensities, centres


def fit_ncp(par, spectrum, masses, data_x, data_y, data_e, fit_arguments, IPFile, g_log):
    boundaries, constraints = fit_arguments[0], fit_arguments[1]
    result = optimize.minimize(
        errfunc,
        par[:],
        args=(spectrum, masses, data_x, data_y, data_e, IPFile, g_log),
        method="SLSQP",
        bounds=boundaries,
        constraints=constraints,
    )
    fitted_par = result.x
    ncp = calculate_ncp(fitted_par, spectrum, masses, data_x, IPFile, g_log)
    return ncp, fitted_par, result


def errfunc(par, spectrum, masses, data_x, data_y, data_e, IPFile, g_log):
    # this function provides the scalar to be minimised, with meaning of the non-reduced chi2
    ncp = calculate_ncp(par, spectrum, masses, data_x, IPFile, g_log)
    if np.sum(data_e) > 0:
        chi2 = (ncp - data_y) ** 2 / (data_e) ** 2  # Distance to the target function
    else:
        chi2 = (ncp - data_y) ** 2
    return chi2.sum()


def calculate_ncp(par, spectrum, masses, data_x, IPFile, g_log):
    angle, T0, L0, L1 = load_ip_file(spectrum, IPFile)
    mN, Ef, en_to_vel, vf, hbar = load_constants()
    ncp = 0.0  # initialising the function values
    # velocities in m/us, times in us, energies in meV
    v0, E0, delta_E, delta_Q = calculate_kinematics(data_x, angle, T0, L0, L1)
    npars = len(par) / len(masses)
    for m in range(len(masses)):  #   [parameter_index + number_of_mass * number_of_parameters_per_mass ]
        index = int(m * npars)
        mass, hei, width, centre = masses[m], par[index], par[1 + index], par[2 + index]
        E_r = (hbar * delta_Q) ** 2 / (2.0 * mass)
        y = mass * (delta_E - E_r) / (hbar * hbar * delta_Q)

        if np.isnan(centre):
            centre = 0.0  # the line below give error if centre = nan, i.e., if detector is masked
        joy = fun_gaussian(y - centre, 1.0)
        pcb = np.where(joy == joy.max())  # this finds the peak-centre bin (pcb)
        gaussian_res_width, lorentzian_res_width = calculate_resolution(spectrum, data_x[pcb], mass, IPFile)
        # definition of the experimental neutron compton profile
        gaussian_width = np.sqrt(width**2 + gaussian_res_width**2)
        joy = fun_pseudo_voigt(y - centre, gaussian_width, lorentzian_res_width)
        # 0.72 is an empirical coefficient. One can alternatively add a fitting parameter for this term.
        FSE = -0.72 * fun_derivative3(y, joy, g_log) / delta_Q
        # H4  = some_missing_coefficient *  fun_derivative4(y,joy) /(4.*(width**4)*32.)
        ncp += hei * (joy + FSE) * E0 * E0 ** (-0.92) * mass / delta_Q  # Here -0.92 is a parameter describing the epithermal flux tail.
    return ncp


def calculate_kinematics(data_x, angle, T0, L0, L1):
    mN, Ef, en_to_vel, vf, hbar = load_constants()
    t_us = data_x - T0
    v0 = vf * L0 / (vf * t_us - L1)
    E0 = (v0 / en_to_vel) ** 2
    delta_E = E0 - Ef
    delta_Q2 = 2.0 * mN / hbar**2 * (E0 + Ef - 2.0 * np.sqrt(E0 * Ef) * np.cos(angle / 180.0 * np.pi))
    delta_Q = np.sqrt(delta_Q2)
    return v0, E0, delta_E, delta_Q


def calculate_resolution(spectrum, data_x, mass, IPFile):
    angle, T0, L0, L1 = load_ip_file(spectrum, IPFile)
    mN, Ef, en_to_vel, vf, hbar = load_constants()
    # all standard deviations, apart from lorentzian hwhm
    dE1, dTOF, dTheta, dL0, dL1, lorentzian_res_width = load_resolution_parameters(spectrum)
    v0, E0, delta_E, delta_Q = calculate_kinematics(data_x, angle, T0, L0, L1)
    # definition of the resolution components in meV:
    dEE = (1.0 + (E0 / Ef) ** 1.5 * (L1 / L0)) ** 2 * dE1**2 + (2.0 * E0 * v0 / L0) ** 2 * dTOF**2
    dEE += (2.0 * E0**1.5 / Ef**0.5 / L0) ** 2 * dL1**2 + (2.0 * E0 / L0) ** 2 * dL0**2
    dQQ = (1.0 - (E0 / Ef) ** 1.5 * L1 / L0 - np.cos(angle / 180.0 * np.pi) * ((E0 / Ef) ** 0.5 - L1 / L0 * E0 / Ef)) ** 2 * dE1**2
    DQQ_2 = np.abs(Ef / E0 * np.cos(angle / 180.0 * np.pi) - 1.0)
    dQQ += ((2.0 * E0 * v0 / L0) ** 2 * dTOF**2 + (2.0 * E0**1.5 / L0 / Ef**0.5) ** 2 * dL1**2 + (2.0 * E0 / L0) ** 2 * dL0**2) * DQQ_2
    dQQ += (2.0 * np.sqrt(E0 * Ef) * np.sin(angle / 180.0 * np.pi)) ** 2 * dTheta**2
    # conversion from meV^2 to A^-2
    dEE *= (mass / hbar**2 / delta_Q) ** 2
    dQQ *= (mN / hbar**2 / delta_Q) ** 2
    gaussian_res_width = np.sqrt(dEE + dQQ)  # in A-1
    # lorentzian component in meV
    dEE_lor = (1.0 + (E0 / Ef) ** 1.5 * (L1 / L0)) ** 2  # is it - or +?
    dQQ_lor = (1.0 - (E0 / Ef) ** 1.5 * L1 / L0 - np.cos(angle / 180.0 * np.pi) * ((E0 / Ef) ** 0.5 + L1 / L0 * E0 / Ef)) ** 2
    # conversion from meV^2 to A^-2
    dEE_lor *= (mass / hbar**2 / delta_Q) ** 2
    dQQ_lor *= (mN / hbar**2 / delta_Q) ** 2
    lorentzian_res_width *= np.sqrt(dEE_lor + dQQ_lor)  # in A-1
    return gaussian_res_width, lorentzian_res_width  # gaussian std dev, lorentzian hwhm


#
#       CORRECTION FUNCTIONS
#


def calculate_mean_widths_and_intensities(masses, widths, intensities, spectra, g_log):
    better_widths, better_intensities = np.zeros((len(masses), len(widths[0]))), np.zeros((len(masses), len(widths[0])))
    mean_widths, widths_std, mean_intensity_ratios = np.zeros((len(masses))), np.zeros((len(masses))), np.zeros((len(masses)))
    for m in range(len(masses)):
        mean_widths[m] = np.nanmean(widths[m])
        widths_std[m] = np.nanstd(widths[m])
        for index in range(len(widths[0])):  # over all spectra
            if abs(widths[m][index] - mean_widths[m]) > widths_std[m]:
                better_widths[m][index], better_intensities[m][index] = None, None
            else:
                better_widths[m][index], better_intensities[m][index] = widths[m][index], intensities[m][index]
        mean_widths[m] = np.nanmean(better_widths[m])
        widths_std[m] = np.nanstd(better_widths[m])
    for spec in range(len(spectra)):
        normalisation = better_intensities[:, spec]
        better_intensities[:, spec] /= normalisation.sum()
    for m in range(len(masses)):
        mean_intensity_ratios[m] = np.nanmean(better_intensities[m])
        g_log.notice("\n", "Mass: ", masses[m], " width: ", mean_widths[m], r" \pm ", widths_std[m])
    return mean_widths, mean_intensity_ratios


def calculate_sample_properties(masses, mean_widths, mean_intensity_ratios, mode, g_log):
    if mode == "GammaBackground":
        profiles = ""
        for m in range(len(masses)):
            mass, width, intensity = str(masses[m]), str(mean_widths[m]), str(mean_intensity_ratios[m])
            profiles += "name=GaussianComptonProfile,Mass=" + mass + ",Width=" + width + ",Intensity=" + intensity + ";"
        sample_properties = profiles
    elif mode == "MultipleScattering":
        MS_properties = []
        for m in range(len(masses)):
            MS_properties.append(masses[m])
            MS_properties.append(mean_intensity_ratios[m])
            MS_properties.append(mean_widths[m])
        sample_properties = MS_properties
    g_log.debug("\n", "The sample properties for ", mode, " are: ", sample_properties)
    return sample_properties


def correct_for_gamma_background(ws_name, first_spectrum, last_spectrum, sample_properties, g_log):
    g_log.debug("Evaluating the Gamma Background Correction.")
    gamma_background_correction = sapi.CloneWorkspace(ws_name)
    for spec in range(first_spectrum, last_spectrum + 1):
        ws_index = gamma_background_correction.getIndexFromSpectrumNumber(spec)
        tmp_bkg, tmp_cor = sapi.VesuvioCalculateGammaBackground(
            InputWorkspace=ws_name, ComptonFunction=sample_properties, WorkspaceIndexList=ws_index
        )
        for bin in range(gamma_background_correction.blocksize()):
            gamma_background_correction.dataY(ws_index)[bin] = tmp_bkg.dataY(0)[bin]
            gamma_background_correction.dataE(ws_index)[bin] = 0.0
    sapi.RenameWorkspace(InputWorkspace="gamma_background_correction", OutputWorkspace=str(ws_name) + "_gamma_background")
    safe_delete_ws("tmp_cor")
    safe_delete_ws("tmp_bkg")
    return


def create_slab_geometry(ws_name, vertical_width, horizontal_width, thickness):
    half_height, half_width, half_thick = 0.5 * vertical_width, 0.5 * horizontal_width, 0.5 * thickness
    xml_str = (
        ' <cuboid id="sample-shape"> '
        + '<left-front-bottom-point x="%f" y="%f" z="%f" /> ' % (half_width, -half_height, half_thick)
        + '<left-front-top-point x="%f" y="%f" z="%f" /> ' % (half_width, half_height, half_thick)
        + '<left-back-bottom-point x="%f" y="%f" z="%f" /> ' % (half_width, -half_height, -half_thick)
        + '<right-front-bottom-point x="%f" y="%f" z="%f" /> ' % (-half_width, -half_height, half_thick)
        + "</cuboid>"
    )
    sapi.CreateSampleShape(ws_name, xml_str)
    return


############################
### functions to fit the NCP in the y space
############################
def subtract_other_masses(ws_name, widths, intensities, centres, spectra, masses, IPFile, g_log):
    hydrogen_ws = sapi.CloneWorkspace(InputWorkspace=ws_name)
    for index in range(len(spectra)):  # for each spectrum
        data_x, data_y, data_e = load_workspace(ws_name, spectra[index])  # get the experimental data after the last correction
        for m in range(len(masses) - 1):  # for all the masses but the first (generally H)
            other_par = (
                intensities[m + 1, index],
                widths[m + 1, index],
                centres[m + 1, index],
            )  # define the input parameters to get the NCPs
            ncp = calculate_ncp(other_par, spectra[index], [masses[m + 1]], data_x, IPFile, g_log)
            for bin in range(len(data_x) - 1):
                hydrogen_ws.dataY(index)[bin] -= ncp[bin] * (data_x[bin + 1] - data_x[bin])
    return hydrogen_ws


def convert_to_y_space_and_symmetrise(ws_name, mass):
    # phenomenological roule-of-thumb to define the y-range for a given mass
    max_Y = np.ceil(2.5 * mass + 27)
    rebin_parameters = str(-max_Y) + "," + str(2.0 * max_Y / 120) + "," + str(max_Y)
    # converting to y-space, rebinning, and defining a normalisation matrix to take into account the kinetic cut-off
    sapi.ConvertToYSpace(InputWorkspace=ws_name, Mass=mass, OutputWorkspace=ws_name + "_JoY", QWorkspace=ws_name + "_Q")
    ws = sapi.Rebin(InputWorkspace=ws_name + "_JoY", Params=rebin_parameters, FullBinsOnly=True, OutputWorkspace=ws_name + "_JoY")
    tmp = sapi.CloneWorkspace(InputWorkspace=ws_name + "_JoY")
    for j in range(tmp.getNumberHistograms()):
        for k in range(tmp.blocksize()):
            tmp.dataE(j)[k] = 0.0
            if np.isnan(tmp.dataY(j)[k]):
                ws.dataY(j)[k] = 0.0
                tmp.dataY(j)[k] = 0.0
            if tmp.dataY(j)[k] != 0:
                tmp.dataY(j)[k] = 1.0
    tmp = sapi.SumSpectra("tmp")
    sapi.SumSpectra(InputWorkspace=ws_name + "_JoY", OutputWorkspace=ws_name + "_JoY_sum")
    sapi.Divide(LHSWorkspace=ws_name + "_JoY_sum", RHSWorkspace="tmp", OutputWorkspace=ws_name + "_JoY_sum")
    # rewriting the temporary workspaces ws and tmp
    ws = sapi.mtd[ws_name + "_JoY_sum"]
    tmp = sapi.CloneWorkspace(InputWorkspace=ws_name + "_JoY_sum")
    for k in range(tmp.blocksize()):
        tmp.dataE(0)[k] = (ws.dataE(0)[k] + ws.dataE(0)[ws.blocksize() - 1 - k]) / 2.0
        tmp.dataY(0)[k] = (ws.dataY(0)[k] + ws.dataY(0)[ws.blocksize() - 1 - k]) / 2.0
    sapi.RenameWorkspace(InputWorkspace="tmp", OutputWorkspace=ws_name + "_JoY_sym")
    normalise_workspace(ws_name + "_JoY_sym")
    return max_Y


def correct_for_multiple_scattering(
    ws_name,
    first_spectrum,
    last_spectrum,
    sample_properties,
    transmission_guess,
    multiple_scattering_order,
    number_of_events,
    g_log,
    masses,
    mean_intensity_ratios,
):
    g_log.debug("Evaluating the Multiple Scattering Correction.")
    dens, trans = sapi.VesuvioThickness(
        Masses=masses, Amplitudes=mean_intensity_ratios, TransmissionGuess=transmission_guess, Thickness=0.1
    )
    _TotScattering, _MulScattering = sapi.VesuvioCalculateMS(
        ws_name,
        NoOfMasses=len(masses),
        SampleDensity=dens.cell(9, 1),
        AtomicProperties=sample_properties,
        BeamRadius=2.5,
        NumScatters=multiple_scattering_order,
        NumEventsPerRun=int(number_of_events),
    )
    data_normalisation = sapi.Integration(ws_name)
    simulation_normalisation = sapi.Integration("_TotScattering")
    for workspace in ("_MulScattering", "_TotScattering"):
        ws = sapi.mtd[workspace]
        for j in range(ws.getNumberHistograms()):
            for k in range(ws.blocksize()):
                ws.dataE(j)[k] = 0.0  # set the errors from the MonteCarlo simulation to zero - no propagation of such uncertainties
                # - Use high number of events for final corrections!!!
        sapi.Divide(LHSWorkspace=workspace, RHSWorkspace=simulation_normalisation, OutputWorkspace=workspace)
        sapi.Multiply(LHSWorkspace=workspace, RHSWorkspace=data_normalisation, OutputWorkspace=workspace)
        sapi.RenameWorkspace(InputWorkspace=workspace, OutputWorkspace=str(ws_name) + workspace)
    safe_delete_ws(data_normalisation)
    safe_delete_ws(simulation_normalisation)
    safe_delete_ws(trans)
    safe_delete_ws(dens)
    return


def calculate_mantid_resolutions(ws_name, mass):
    max_Y = np.ceil(2.5 * mass + 27)
    rebin_parameters = str(-max_Y) + "," + str(2.0 * max_Y / 240) + "," + str(max_Y)  # twice the binning as for the data
    ws = sapi.mtd[ws_name]
    for index in range(ws.getNumberHistograms()):
        sapi.VesuvioResolution(Workspace=ws, WorkspaceIndex=index, Mass=mass, OutputWorkspaceYSpace="tmp")
        tmp = sapi.Rebin("tmp", rebin_parameters)
        if index == 0:
            sapi.RenameWorkspace(tmp, "resolution")
        else:
            sapi.AppendSpectra("resolution", tmp, OutputWorkspace="resolution")
    sapi.SumSpectra(InputWorkspace="resolution", OutputWorkspace="resolution")
    normalise_workspace("resolution")
    safe_delete_ws(tmp)


def normalise_workspace(ws_name):
    tmp_norm = sapi.Integration(ws_name)
    sapi.Divide(LHSWorkspace=ws_name, RHSWorkspace="tmp_norm", OutputWorkspace=ws_name)
    safe_delete_ws(tmp_norm)


def final_fit(fit_ws_name, constraints, y_range, correct_for_offsets, masses, g_log):
    function = """
    composite=Convolution,FixResolution=true,NumDeriv=true;
        name=Resolution,Workspace=resolution,WorkspaceIndex=0,X=(),Y=();
        name=UserFunction,Formula=exp( -x^2/2./sigma1^2)
        *(1.+c4/32.*(16.*(x/sqrt(2)/sigma1)^4-48.*(x/sqrt(2)/sigma1)^2+12)
              +c6/384.*( 64.*(x/sqrt(2)/sigma1)^6 -480.*(x/sqrt(2)/sigma1)^4 +720.*(x/sqrt(2)/sigma1)^2 -120.) )*A + B0,
        sigma1=3.0,c4=0.0, c6=0.0,A=0.08, B0=0.00, ties = (c6=0. )
        """
    function += constraints
    minimiser = "Simplex"
    sapi.Fit(
        Function=function,
        InputWorkspace=fit_ws_name,
        MaxIterations=2000,
        Minimizer=minimiser,
        Output=fit_ws_name,
        OutputCompositeMembers=True,
        StartX=y_range[0],
        EndX=y_range[1],
    )
    ws = sapi.mtd[fit_ws_name + "_Parameters"]
    g_log.notice("\n Final parameters \n")
    g_log.notice("width: ", ws.cell(0, 1), " +/- ", ws.cell(0, 2), " A-1 ")
    g_log.notice("c4: ", ws.cell(1, 1), " +/- ", ws.cell(1, 2), " A-1 ")
    sigma_to_energy = 1.5 * 2.0445**2 / masses[0]
    g_log.notice(
        "mean kinetic energy: ",
        sigma_to_energy * ws.cell(0, 1) ** 2,
        " +/- ",
        2.0 * sigma_to_energy * ws.cell(0, 2) * ws.cell(0, 1),
        " meV ",
    )
    if correct_for_offsets:
        sapi.Scale(InputWorkspace=fit_ws_name, Factor=-ws.cell(4, 1), Operation="Add", OutputWorkspace=fit_ws_name + "_cor")
        sapi.Scale(
            InputWorkspace=fit_ws_name + "_cor",
            Factor=(2.0 * np.pi) ** (-0.5) / ws.cell(0, 1) / ws.cell(3, 1),
            Operation="Multiply",
            OutputWorkspace=fit_ws_name + "_cor",
        )


class element:
    def __init__(self, mass, intensity_range, width_range, centre_range):
        self.mass = float(mass)
        self.intensity_low, self.intensity, self.intensity_high = intensity_range[0], intensity_range[1], intensity_range[2]
        self.width_low, self.width, self.width_high = width_range[0], width_range[1], width_range[2]
        self.centre_low, self.centre, self.centre_high = centre_range[0], centre_range[1], centre_range[2]


class constraint:  # with reference to the "elements" vector positions
    def __init__(self, lhs_element_position, rhs_element_position, rhs_factor, type):
        self.lhs_element_position = lhs_element_position
        self.rhs_element_position = rhs_element_position
        self.rhs_factor = rhs_factor
        self.type = type


def prepare_fit_arguments(elements, constraints):
    masses = list(np.zeros(len(elements)))
    masses[0] = elements[0].mass
    par = (elements[0].intensity, elements[0].width, elements[0].centre)
    bounds = (
        (elements[0].intensity_low, elements[0].intensity_high),
        (elements[0].width_low, elements[0].width_high),
        (elements[0].centre_low, elements[0].centre_high),
    )
    for m in range(len(elements) - 1):
        m += 1
        masses[m] = elements[m].mass
        par += (elements[m].intensity, elements[m].width, elements[m].centre)
        bounds += (
            (elements[m].intensity_low, elements[m].intensity_high),
            (elements[m].width_low, elements[m].width_high),
            (elements[m].centre_low, elements[m].centre_high),
        )
    fit_constraints = []
    for k in range(len(constraints)):
        # from element position in elements to intensity position in par
        lhs_int, rhs_int = 3 * constraints[k].lhs_element_position, 3 * constraints[k].rhs_element_position
        fit_constraints.append({"type": constraints[k].type, "fun": lambda par: par[lhs_int] - constraints[k].rhs_factor * par[rhs_int]})
    return masses, par, bounds, fit_constraints


def cleanNames(list):
    return [name.replace(" ", "").lower() for name in list]


def generate_elements(table):
    table_cols = table.getColumnNames()
    clean_names = cleanNames(table.getColumnNames())
    num_rows = table.rowCount()
    elements = []
    for row in range(num_rows):
        value = {}
        for name, clean in zip(table_cols, clean_names):
            data = table.row(row)[name]
            if isinstance(data, float) and data == 9.9e9:
                data = None
            value[clean] = data
        intensity = [value["intensitylowerlimit"], value["intensityvalue"], value["intensityupperlimit"]]
        width = [value["widthlowerlimit"], value["widthvalue"], value["widthupperlimit"]]
        centre = [value["centrelowerlimit"], value["centrevalue"], value["centreupperlimit"]]
        el = element(mass=value["mass(a.u.)"], intensity_range=intensity, width_range=width, centre_range=centre)
        elements.append(el)
    return elements


def generate_constraints(table):
    constraints = []
    if table and table.rowCount() > 0:
        table_cols = table.getColumnNames()
        clean_names = cleanNames(table.getColumnNames())
        num_rows = table.rowCount()
        for row in range(num_rows):
            value = {}
            for name, clean in zip(table_cols, clean_names):
                data = table.row(row)[name]
                value[clean] = data
            # provide LHS element, RHS element, mult. factor, flag
            # if flag=True inequality; if flag = False equality
            cons = constraint(value["lhselement"], value["rhselement"], evaluate(value["scatteringcrosssection"]), value["state"])
            constraints.append(cons)
    return constraints


def evaluate(input):
    if input.isdigit():
        return float(input)
    return eval(input)
