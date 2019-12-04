# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2019 ISIS Rutherford Appleton Laboratory UKRI,
#     NScD Oak Ridge National Laboratory, European Spallation Source
#     & Institut Laue - Langevin
# SPDX - License - Identifier: GPL - 3.0 +
"""
Presenter for DNS simulation
"""

from __future__ import (absolute_import, division, print_function)
import numpy as np

from DNSReduction.data_structures.dns_observer import DNSObserver
from DNSReduction.simulation.simulation_view import DNSSimulation_view
from DNSReduction.simulation.simulation_model import DNSSimulation_model
import DNSReduction.simulation.simulation_helpers as sim_help


class DNSSimulation_presenter(DNSObserver):
    TTHLIMIT = 5  ## limit for 2theta difference under which entries marked
    ## as matching, 5deg is detector distance at DNS

    def __init__(self, parent):
        super(DNSSimulation_presenter, self).__init__(parent, 'simulation')
        self.name = 'simulation'  #cannot contain spaces is used as dict key
        self.view = DNSSimulation_view(self.parent.view)
        self.model = DNSSimulation_model(parent=self)
        self.refls = None
        self.own_dict['cifset'] = False
        self.set_ki()

        ## connect Signals
        self.view.sig_cif_set.connect(self.cif_set)
        self.view.sig_unitcell_changed.connect(self.unitcell_changed)
        self.view.sig_wavelength_changed.connect(self.set_ki)
        self.view.sig_calculate_clicked.connect(self.calculate)
        self.view.sig_inplane_unique_switched.connect(self.inplane_unique)
        self.view.sig_table_item_clicked.connect(self.tableitemdclicked)
        self.view.sig_powderplot_clicked.connect(self.powderplot)
        self.view.sig_scplot_clicked.connect(self.sc_plot)
        self.view.sig_mouse_pos_changed.connect(self.set_hkl_pos_on_plot)

    def get_and_validate(self):
        self.get_option_dict()
        hkl1 = self.own_dict['hkl1_v']
        hkl2 = self.own_dict['hkl2_v']
        if (hkl1[0] is None or hkl2[0] is None):
            self.raise_error('Could not parse hkl, enter 3 comma'
                             'seperated numbers.')
            return False
        if (np.cross(hkl1, hkl2) == 0).all():
            self.raise_error('hkl1 cannot be paralell hkl2')
            return False
        return True

    def calculate(self):
        if not self.get_and_validate():
            return
        try:
            self.refls = self.model.get_refls_and_set_orientation()
        except ValueError:
            self.raise_error('Spacegroup not valid, use HM'
                             'Symbol or IT Number.')
            return
        filtered_refls = self.model.filter_refls(self.refls)
        self.perp_inplane()
        self.d_tooltip()
        self.view.writetable(filtered_refls, self.TTHLIMIT)
        self.powderplot()
        self.sc_plot()
        return

    def cif_set(self, filename):
        self.model.loadCIF(filename)
        self.view.set_unitcell(
            self.own_dict['a'],
            self.own_dict['b'],
            self.own_dict['c'],
            self.own_dict['alpha'],
            self.own_dict['beta'],
            self.own_dict['gamma'],
            self.own_dict['spacegroup'],
        )
        self.own_dict['cifset'] = True

    def d_tooltip(self):
        d_hkl1, d_hkl2, d_hkl2_p = self.model.get_ds()
        self.view.set_d_tooltip(d_hkl1, d_hkl2, d_hkl2_p)

    def inplane_unique(self):
        self.get_option_dict()
        filtered_refls = self.model.filter_refls(self.refls)
        self.view.writetable(filtered_refls, self.TTHLIMIT)
        return

    def perp_inplane(self):
        """returns vector perpendicular to hkl1 in the scatteringt plane """
        q2_p = self.model.get_hkl2_p()
        self.own_dict['hkl2_p_v'] = q2_p
        self.view.set_hkl2_p(q2_p)

    def powderplot(self):
        self.get_option_dict()
        start = -self.own_dict['powder_start']
        end = -self.own_dict['powder_end']
        shift = self.own_dict['shift']
        tth_step = 0.1
        tth_end = end + 23*5 + shift
        tth_start = start + shift
        x, y = self.model.create_powder_profile(tth_start,
                                                tth_end,
                                                tth_step,
                                                shift)
        refls = [refl for refl in self.refls if refl.unique]
        annotate_list = [[], [], []]
        for refl in refls:
            if refl.tth + shift <= tth_end and round(
                    refl.tth, 2) not in annotate_list[0]:
                xnumb = int((refl.tth - start) / tth_step)
                annotate_list[0].append(round(refl.tth, 2))
                annotate_list[1].append(refl.hkl)
                annotate_list[2].append(y[xnumb])
        self.view.powderplot(x, y, annotate_list)

    def sc_plot(self):
        self.get_option_dict()
        q1 = self.own_dict['hkl1_v']
        q2 = self.own_dict['hkl2_p_v']
        line = self.model.create_dns_surface(q1, q2)
        refls = self.model.return_reflections_in_map(q1, q2, self.refls)
        if refls.any():
            maxI = refls[:2].max()
            minI = refls[:2].min()
            self.view.sc_plot(line, refls, maxI, minI, q1, q2)
        else:
            self.view.sc_plot(line, refls, 1, 0, q1, q2)

    def set_hkl_pos_on_plot(self, x, y):
        hkl1 = np.asarray(self.own_dict['hkl1_v'])
        hkl2_p = np.asarray(self.own_dict['hkl2_p_v'])
        if x is not None and y is not None:
            hkl = hkl1*x + hkl2_p*y
        self.view.set_hkl_position_on_plot(hkl)

    def set_ki(self):
        self.own_dict['wavelength'] = self.view.get_state()['wavelength']
        wavelength = self.own_dict['wavelength']
        ki = sim_help.ki_from_wavelength(wavelength)
        self.view.set_ki(ki)

    def set_spacegroup(self, spacegroup):
        self.view.set_spacegroup(spacegroup)

    def tableitemdclicked(self, det_rot, sample_rot):
        """ sets the omega offset based on identified reflection """
        self.get_option_dict()
        if not self.own_dict['fix_omega']:
            id_sr = self.own_dict['sample_rot']
            id_dr = self.own_dict['det_rot']
            omegaoffset = (sample_rot - det_rot) - (id_sr - id_dr)
            self.view.set_omega_offset(omegaoffset)
        return

    def unitcell_changed(self):
        self.own_dict['cifset'] = False
