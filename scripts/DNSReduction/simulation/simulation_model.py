# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2019 ISIS Rutherford Appleton Laboratory UKRI,
#     NScD Oak Ridge National Laboratory, European Spallation Source
#     & Institut Laue - Langevin
# SPDX - License - Identifier: GPL - 3.0 +
"""
Model for DNS simulation
"""

from __future__ import (absolute_import, division, print_function)
from numpy import tan, radians, pi, log, sqrt
import numpy as np

from DNSReduction.data_structures.dns_obs_model import DNSObsModel
from DNSReduction.simulation.sim_reflstruc import SimReflStruc
import DNSReduction.simulation.simulation_helpers as sim_help

from mantid.geometry import ReflectionGenerator
from mantid.geometry import ReflectionConditionFilter, OrientedLattice
from mantid.geometry import SpaceGroupFactory, CrystalStructure, UnitCell
from mantid.simpleapi import LoadCIF, CreateWorkspace


def tth_to_q(tth, wavelength):
    return np.pi * 4 * np.sin(tth / 2.0) / wavelength


def tth_to_d(tth, wavelength):
    return wavelength / (2 * np.sin(tth / 2.0))


class DNSSimulation_model(DNSObsModel):
    """
    Model for DNS simulation

    generates HKL list from CIF files or given lattice parameters and
    creates powder diffractogram and single crystal diffraction map
    """
    def __init__(self, parent):
        super(DNSSimulation_model, self).__init__(parent)
        self.sim_ws = None
        self.orilat = None
        self.refls = None
        self.generator = None
        self.cryst = None
        self.non_rot_lat = None

    def create_dns_surface(self, q1, q2):
        tth_start = -self.own_dict['sc_det_start']
        tth_end = -self.own_dict['sc_det_end'] + 23 * 5
        omega_start = self.own_dict['sc_sam_start'] + tth_start
        omega_end = self.own_dict['sc_sam_end'] + tth_start
        wavelength = self.own_dict['wavelength']
        tth_rang = np.linspace(tth_start, tth_end,
                               int(abs((tth_end - tth_start))))
        omega_rang = np.linspace(omega_start, omega_end,
                                 int(abs(omega_end - omega_start)))
        surface = sim_help.return_dns_surface_shape(self.orilat, tth_rang,
                                                    omega_rang, q1, q2,
                                                    wavelength)
        return surface

    def create_powder_profile(self, tth_start, tth_end, tth_step, shift):
        u = 0.1791
        v = -0.4503
        w = 0.4
        tth = np.arange(tth_start, tth_end, tth_step)
        x = tth
        y = tth * 0
        for refl in self.refls:
            shiftedtth = refl.tth + shift
            FWHM = sqrt(u + v * tan(radians(shiftedtth) / 2.0) +
                        w * tan(radians(shiftedtth) / 2.0)**2)
            c = FWHM / (2 * sqrt(2 * log(2)))
            peak = (1 / (c * sqrt(2 * pi)) * np.exp(-0.5 *
                                                    ((x - shiftedtth) / c)**2))
            y += refl.fs_lc * peak
        bins = tth - tth_step / 2.0
        bins = np.append(bins, tth[-1] + tth_step / 2.0)
        CreateWorkspace(OutputWorkspace='mat_simulation',
                        DataX=bins,
                        DataY=y,
                        NSpec=1,
                        UnitX='Degrees')
        return [x, y]

    def filter_refls(self, refls):
        inplane = self.own_dict['inplane']
        unique = self.own_dict['unique']
        if inplane and not unique:
            refls = [refl for refl in refls if refl.inplane]
        elif not inplane and unique:
            refls = [refl for refl in refls if refl.unique]
        elif inplane and unique:
            newrefls = []
            for refl in refls:
                if refl.inplane:
                    if not any(refl.hkl in nrefl.equivalents
                               for nrefl in newrefls):
                        newrefls.append(refl)
            refls = newrefls
        return refls

    def get_ds(self):
        [d_hkl1, d_hkl2, d_hkl2_p] = self.generator.getDValues([
            sim_help.listtoV3D(self.own_dict['hkl1_v']),
            sim_help.listtoV3D(self.own_dict['hkl2_v']),
            sim_help.listtoV3D(self.own_dict['hkl2_p_v'])
        ])
        return [d_hkl1, d_hkl2, d_hkl2_p]

    def get_hkl2_p(self):
        q2_p = self.non_rot_lat.getvVector()
        q2_p = q2_p / np.linalg.norm(q2_p)
        return q2_p

    def get_refls_and_set_orientation(self):
        q1 = self.own_dict['hkl1_v']
        q2 = self.own_dict['hkl2_v']
        self.refls = []
        if self.own_dict['cifset'] is False:
            self.setcellfromparameters()
        wavelength = self.own_dict['wavelength']
        self.orilat.setUFromVectors(sim_help.listtoV3D(q1),
                                    sim_help.listtoV3D(q2))
        self.non_rot_lat.setUFromVectors(sim_help.listtoV3D(q1),
                                         sim_help.listtoV3D(q2))
        self.generator = ReflectionGenerator(self.cryst)
        maxq = sim_help.max_q(wavelength)
        if not self.own_dict['cif_filename']:
            reffilter = ReflectionConditionFilter.SpaceGroup
        else:
            reffilter = ReflectionConditionFilter.StructureFactor
        hkls_unique = self.generator.getUniqueHKLsUsingFilter(
            1 / maxq, 100, reffilter)  # dived by 2pi
        hkls = self.generator.getHKLsUsingFilter(1 / maxq, 100,
                                                 reffilter)  # dived by 2pi
        dval = self.generator.getDValues(hkls)
        qval = [2 * pi / d for d in dval]
        fSquared = self.generator.getFsSquared(hkls)
        pg = self.cryst.getSpaceGroup().getPointGroup()
        UB = self.orilat.getUB()
        if self.own_dict['fix_omega']:
            UB = sim_help.rotate_UB(self.own_dict['omega_offset'], UB)
            self.orilat.setUB(UB)

        identify_tth = sim_help.det_rot_nmb_to_tth(self.own_dict['det_rot'],
                                                   self.own_dict['det_number'])

        for i in range(len(hkls) - 1, 0, -1):
            #reverse order to get positive hkl
            refl = SimReflStruc()
            refl.hkl = hkls[i]
            refl.unique = refl.hkl in hkls_unique
            refl.q = qval[i]
            refl.fs = fSquared[i]
            refl.d = dval[i]
            refl.tth = sim_help.q_to_tth(refl.q, wavelength)
            refl.fs_lc = sim_help.lorentz_correction(refl.fs, refl.tth)
            if not self.own_dict['cif_filename']:
                # if dummy scatterer set I = 1
                refl.fs = 1
                refl.fs_lc = 1
            refl.h = refl.hkl[0]
            refl.k = refl.hkl[1]
            refl.l = refl.hkl[2]
            refl.equivalents = pg.getEquivalents(refl.hkl)
            refl.M = len(refl.equivalents)
            refl.diff = abs(identify_tth - refl.tth)
            refl.det_rot, refl.channel = sim_help.tth_to_rot_nmb(refl.tth)
            refl.inplane = sim_help.check_inplane(q1, q2, refl.hkl)

            refl.channel, refl.det_rot = sim_help.shift_channels_below_23(
                refl.channel, refl.det_rot)
            refl.omega = sim_help.hkl_to_omega(refl.hkl, UB, wavelength,
                                               refl.tth)
            refl.sample_rot = sim_help.omega_to_samp_rot(
                refl.omega, refl.det_rot)
            self.refls.append(refl)
        return self.refls

    def get_refls(self):
        return self.refls

    def loadCIF(self, fileName):
        """uses mantid to load CIF and set crystalstructure and oriented
        lattice, calls set unitcell"""
        self.sim_ws = CreateWorkspace(OutputWorkspace='__sim_ws',
                                      DataX=[0],
                                      DataY=[0],
                                      NSpec=1,
                                      UnitX='Degrees')
        LoadCIF(self.sim_ws, fileName)
        self.cryst = self.sim_ws.sample().getCrystalStructure()
        self.orilat = OrientedLattice(self.cryst.getUnitCell())
        self.non_rot_lat = OrientedLattice(self.cryst.getUnitCell())
        self.own_dict['a'] = self.cryst.getUnitCell().a()
        self.own_dict['b'] = self.cryst.getUnitCell().b()
        self.own_dict['c'] = self.cryst.getUnitCell().c()
        self.own_dict['alpha'] = self.cryst.getUnitCell().alpha()
        self.own_dict['beta'] = self.cryst.getUnitCell().beta()
        self.own_dict['gamma'] = self.cryst.getUnitCell().gamma()
        self.own_dict['spacegroup'] = self.cryst.getSpaceGroup().getHMSymbol()
        return

    def return_reflections_in_map(self, q1, q2, refls):
        refls = sim_help.return_qx_qx_inplane_refl(self.orilat, q1, q2, refls)
        return refls

    def setcellfromparameters(self):
        spacegroup = self.own_dict['spacegroup']
        if spacegroup.isdigit():  # if user gives SG number convert to HM
            spacegroup = SpaceGroupFactory.subscribedSpaceGroupSymbols(
                int(spacegroup))
            spacegroup = spacegroup[0]
            # number is not unique just use one rep.
            self.presenter.set_spacegroup(spacegroup)
        unitcell = UnitCell(self.own_dict['a'],
                            self.own_dict['b'],
                            self.own_dict['c'],
                            self.own_dict['alpha'],
                            self.own_dict['beta'],
                            self.own_dict['gamma'],
                            Unit=0)
        orilat = OrientedLattice(unitcell)
        self.non_rot_lat = OrientedLattice(unitcell)
        cellstr = "{} {} {} {} {} {}".format(unitcell.a(), unitcell.b(),
                                             unitcell.c(), unitcell.alpha(),
                                             unitcell.beta(), unitcell.gamma())
        if not self.own_dict['cif_filename']:
            atomstr = "Si 0 0 1 1.0 0.01"  # dummy
        else:
            atomstr = ';'.join(self.cryst.getScatterers())
        self.cryst = CrystalStructure(cellstr, spacegroup, atomstr)
        self.orilat = orilat
