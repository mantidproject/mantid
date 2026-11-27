# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2025 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +


import numpy as np
import os
import warnings


class PyChopPresenter:
    def __init__(self, model):
        self._model = model
        self._view = None  # to be set later by the View

        self.instruments = {}
        self.choppers = {}
        self.minE = {}
        self.maxE = {}

        # # Use a subscriber to avoid Qt connections in the presenter
        # self._view.subscribe_presenter(self)

    def attach_view(self, view):
        self._view = view
        # Use a subscriber to avoid Qt connections in the presenter
        self._view.subscribe_presenter(self)

    def get_instrument_names(self):
        return list(self.instruments.keys())

    def load_instruments(self, folder):
        for fname in os.listdir(folder):
            if fname.endswith(".yaml"):
                instobj = self._model(os.path.join(folder, fname))
                self.instruments[instobj.name] = instobj
                self.choppers[instobj.name] = instobj.getChopperNames()
                self.minE[instobj.name] = max([instobj.emin, 0.01])
                self.maxE[instobj.name] = instobj.emax

    # example function from MVP example files
    def handle_button_clicked(self):
        # An example method to handle a view event
        self._model.count += 1
        self._view.set_label(str(self._model.count))

    # helper function for View
    def getFrequency(self):
        return self.current_instrument.getFrequency()

    # unchanged function in the View
    def closeEvent(self, event):
        self.assistant_process.close()
        self.assistant_process.waitForFinished()
        event.accept()

    # moved setInstrument function to Presenter as it takes care of the data manipulation
    def set_instrument(self, name):
        # self.current_instrument is the equivalent of self.engine in PyChopGUI
        instrument = self.instruments[str(name)]
        self.current_instrument = instrument  # Track selected instrument

        data = {
            "choppers": instrument.getChopperNames(),
            "rep": instrument.moderator.source_rep,
            "maxfreq": instrument.chopper_system.max_frequencies,
            "multi_chopper": len(instrument.chopper_system.choppers) > 1,
            "freq_names": getattr(instrument.chopper_system, "frequency_names", ["Frequency"]),
            "default_freqs": getattr(instrument.chopper_system, "default_frequencies", []),
            "default_phases": getattr(instrument.chopper_system, "defaultPhase", []),
            "phase_names": getattr(instrument.chopper_system, "phaseNames", []),
            "phase_indep": getattr(instrument.chopper_system, "isPhaseIndependent", []),
            "maxE": self.maxE[instrument.instname],
            "flux_units": instrument.moderator.flux_units,
            "nframe": getattr(instrument.moderator, "n_frame", 1),
            "has_qe_tab": (getattr(instrument, "has_detector", False) and hasattr(instrument.detector, "tthlims")),
        }

        return data

    def setChopper(self, chopper_name, frequency):
        """
        Defines the Fermi chopper slit package type by name, or the disk chopper arrangement variant.
        """
        if not hasattr(self, "current_instrument"):
            raise ValueError("No instrument selected before setting chopper.")

        # now getting frequency from UI widget but changing it in the Presenter
        self.current_instrument.setChopper(str(chopper_name))
        self.current_instrument.setFrequency(float(frequency))

    def get_phase(self, chopper_number, raw_text):
        if not hasattr(self, "current_instrument"):
            raise ValueError("No instrument selected.")

        default = self.current_instrument.chopper_system.defaultPhase[chopper_number]
        rep = self.current_instrument.moderator.source_rep

        if isinstance(default, str):
            return str(raw_text)

        try:
            return float(raw_text) % (1e6 / rep)
        except ValueError:
            raise ValueError(f"Invalid phase value: {raw_text}")

    def setFreq(self, freq_in, phases):
        # mostly stays in the View
        if phases:
            self.current_instrument.setFrequency(freq_in, phase=phases)
        else:
            self.current_instrument.setFrequency(freq_in)

    def get_merlin_chopper_state(self):
        if "MERLIN" not in self.current_instrument.instname:
            return None  # or False or some indicator

        chopper = self.current_instrument.getChopper()
        needs_setting = chopper is None
        # checking for letter "G" in getChopper and letting View deal with that
        contains_G = chopper is not None and "G" in chopper
        return {"needs_setting": needs_setting, "contains_G": contains_G, "current_chopper": chopper}

    def setEi(self, eitxt):
        # original: self.engine.setEi(eitxt) in PyChopGUI

        self.current_instrument.setEi(eitxt)

    def setS2(self, S2txt):
        # same as the method above
        self.hyspecS2 = S2txt

    def calculate(self, multirep_enabled):
        """
        Performs the resolution and flux calculations.
        """
        errormess = None
        eis = []
        res = []
        flux = []

        # get Ei
        ei = self.current_instrument.getEi()
        if ei is None:
            raise ValueError("Ei is not set before calculation.")

        # check if enabled - from widgets "MultiRepCheck"
        if multirep_enabled:
            en = np.linspace(0, 0.95, 200)
            eis = self.current_instrument.getAllowedEi()
            # unchanged code
            with warnings.catch_warnings(record=True) as w:
                warnings.simplefilter("always", UserWarning)
                res = self.current_instrument.getMultiRepResolution(en)
                flux = self.current_instrument.getMultiRepFlux()
                if w:
                    mess = [str(msg.message) for msg in w]
                    errormess = "\n".join([m for m in mess if "tchop" in m])
        else:
            en = np.linspace(0, 0.95 * ei, 200)
            with warnings.catch_warnings(record=True) as w:
                warnings.simplefilter("always", UserWarning)
                res = self.current_instrument.getResolution(en)
                flux = self.current_instrument.getFlux()
                if w:
                    raise ValueError(w[0].message)

        # return calculations to View
        return {"eis": eis, "res": res, "flux": flux, "errormess": errormess}

    ## add getFlux and getResolution helper functions
