# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2021 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
import json
from mantid.api import AlgorithmFactory, PythonAlgorithm, ITableWorkspaceProperty, FileProperty, FileAction
from mantid.simpleapi import CreateEmptyTableWorkspace
from mantid.kernel import Direction
from mantid.utils.muon import PEAK_DATA_JSON
from mantid import mtd

LABELS = {"Primary": "Primary energy", "Secondary": "Secondary energy"}


class PeakMatching(PythonAlgorithm):
    def category(self):
        return "Muon"

    def summary(self):
        return "Matches peaks from table to a database to find probable transitions"

    def seeAlso(self):
        return ["FitGaussianPeaks", "FindPeaks", "FindPeaksAutomatic"]

    def PyInit(self):
        self.declareProperty(
            ITableWorkspaceProperty(name="PeakTable", defaultValue="", direction=Direction.Input),
            doc="Table containing peaks to match to database",
        )

        self.declareProperty(
            FileProperty(name="PeakDatabase", defaultValue="", action=FileAction.OptionalLoad, extensions=["json"]),
            doc="json file with peak database, if none is given default database will be used",
        )

        self.declareProperty("PeakCentreColumn", "centre", doc="Name of column containing centre of peaks", direction=Direction.Input)

        self.declareProperty("SigmaColumn", "sigma", doc="Name of column containing standard deviation of peaks", direction=Direction.Input)

        # Output tables
        self.declareProperty(
            ITableWorkspaceProperty(name="AllPeaks", defaultValue="all_matches", direction=Direction.Output),
            doc="Name of the table containing all of the peak matches",
        )

        self.declareProperty(
            ITableWorkspaceProperty(name="PrimaryPeaks", defaultValue="primary_matches", direction=Direction.Output),
            doc="Name of the table containing the primary peak matches",
        )

        self.declareProperty(
            ITableWorkspaceProperty(name="SecondaryPeaks", defaultValue="secondary_matches", direction=Direction.Output),
            doc="Name of the table containing the secondary peak matches",
        )

        self.declareProperty(
            ITableWorkspaceProperty(name="SortedByEnergy", defaultValue="all_matches_sorted_by_energy", direction=Direction.Output),
            doc="Name of the table containing all of the peak matches sorted by energy",
        )

        self.declareProperty(
            ITableWorkspaceProperty(name="ElementLikelihood", defaultValue="element_likelihood", direction=Direction.Output),
            doc="Name of the table containing the weighted count of elements in all matches",
        )

    def get_default_peak_data(self):
        # import locally dynamically so we don't introduce a qt dependency into the framework
        with open(PEAK_DATA_JSON, "r") as file_to_read:
            peak_data = json.load(file_to_read)
        return peak_data

    def PyExec(self):
        table = self.getPropertyValue("PeakTable")
        path = self.getPropertyValue("PeakDatabase")
        peak_data = self.process_peak_data(path)

        sigma_column = self.getProperty("SigmaColumn").value
        peak_column = self.getProperty("PeakCentreColumn").value
        input_peaks = self.get_input_peaks(table, peak_column, sigma_column)

        all_data, primary_data, secondary_data = self.get_matches(peak_data, input_peaks)

        output_table_names = [
            self.getPropertyValue("PrimaryPeaks"),
            self.getPropertyValue("SecondaryPeaks"),
            self.getPropertyValue("AllPeaks"),
            self.getPropertyValue("SortedByEnergy"),
            self.getPropertyValue("ElementLikelihood"),
        ]

        self.output_data(primary_data, secondary_data, all_data, output_table_names)

    def process_peak_data(self, path):
        """
        opening the json file containing the peak data
        Format of file:
            {
            'Element': {
                'Z': z,
                'A': a,
                'Primary' : {
                    'Transition':energy,
                    ..., ...
                    },
                'Secondary': {
                    'Transition': energy,
                    ..., ...
                    },
                'Gammas': {
                    'Isotope': energy,
                    ..., ...
                    },
                'Electron': {
                    'Energy': probability,
                    ..., ...
                    }
        """
        if path:
            with open(path, "r") as file_to_read:
                peak_data = json.load(file_to_read)
        else:
            peak_data = self.get_default_peak_data()

        processed_data = {"All energies": {}}
        for label in LABELS:
            processed_data[LABELS[label]] = {}

        for element in peak_data:
            processed_data["All energies"][element] = {}
            for label in LABELS:
                energy_list = [float(energy) for transition, energy in list(peak_data[element][label].items())]
                transition_list = [transition for transition, energy in list(peak_data[element][label].items())]
                processed_data[LABELS[label]][element] = dict(zip(transition_list, energy_list))
                processed_data["All energies"][element].update(dict(zip(transition_list, energy_list)))

        return processed_data

    def make_count_table(self, name, data):
        table = CreateEmptyTableWorkspace(OutputWorkspace=name)

        table.addColumn("str", "Element")
        table.addColumn("int", "Likelihood(arbitrary units)")

        counts = {}

        for entry in data:
            if entry["element"] not in counts:
                counts[entry["element"]] = entry["Rating"]
                continue
            counts[entry["element"]] += entry["Rating"]

        # will be sorted by second element in row
        sorted_data_by_count = sorted(counts.items(), key=lambda row: row[1], reverse=True)

        for row in sorted_data_by_count:
            table.addRow([str(row[0]), row[1]])
        return table

    def get_input_peaks(self, table, centre_column_name, sigma_column_name):
        """DEFINING WIDTH OF PEAKS
        Width input which will be used to determine the standard
        deviation and FWHM which will be used as error for fitting.

        From 'Gauss' function in origin:
        y = y0 + (A/(w*sqrt(PI/2)))*exp(-2*((x-xc)/w)^2)
        which has been used to fit all data to this point, where:
            A>0
            y0 = offset
            xc = centre
            w = width
            A = area

        FWHM = w(sqrt(ln4))
        sigma = w/2

        """
        table_dict = mtd[table].toDict()
        peaks = []
        sigma = []
        for i in range(len(table_dict[centre_column_name])):
            peaks.append(table_dict[centre_column_name][i])
            sigma.append(table_dict[sigma_column_name][i])

        return list(zip(peaks, sigma))

    def get_matches(self, peak_data, input_peaks):
        all_matches = []
        for label in peak_data:
            raw_data = peak_data[label]
            matches = []
            for peak, sigma in input_peaks:
                for element in raw_data:
                    for transition, energy in raw_data[element].items():
                        for num_sigma in range(0, 4):
                            if peak >= (energy - (num_sigma * sigma)) and peak <= (energy + (sigma * num_sigma)):
                                data = {
                                    "element": element,
                                    "energy": energy,
                                    "error": sigma * num_sigma,
                                    "peak_centre": peak,
                                    "transition": transition,
                                    "diff": abs(peak - energy),
                                    "Rating": 4 - num_sigma,
                                }
                                matches.append(data)
                                break
            matches = sorted(matches, key=lambda data: data["diff"])
            all_matches.append(matches)
        return all_matches

    def output_data(self, primary_data, secondary_data, all_data, output_table_names):
        prim_table = self.make_peak_table(output_table_names[0], primary_data)
        self.setPropertyValue("PrimaryPeaks", output_table_names[0])
        self.setProperty("PrimaryPeaks", prim_table)

        secon_table = self.make_peak_table(output_table_names[1], secondary_data)
        self.setPropertyValue("SecondaryPeaks", output_table_names[1])
        self.setProperty("SecondaryPeaks", secon_table)

        all_table = self.make_peak_table(output_table_names[2], all_data)
        self.setPropertyValue("AllPeaks", output_table_names[2])
        self.setProperty("AllPeaks", all_table)

        sorted_table = self.make_peak_table(output_table_names[3], all_data, True, "energy")
        self.setPropertyValue("SortedByEnergy", output_table_names[3])
        self.setProperty("SortedByEnergy", sorted_table)

        count_table = self.make_count_table(output_table_names[4], all_data)
        self.setPropertyValue("ElementLikelihood", output_table_names[4])
        self.setProperty("ElementLikelihood", count_table)

    def make_peak_table(self, name, data, sort_by=False, value_to_sort_by="energy"):
        table = CreateEmptyTableWorkspace(OutputWorkspace=name)

        table.addColumn("double", "Peak centre")
        table.addColumn("double", "Database Energy")
        table.addColumn("str", "Element")
        table.addColumn("str", "Transition")
        table.addColumn("double", "Error")
        table.addColumn("double", "Difference")

        if sort_by:
            data = sorted(data, key=lambda row: row[value_to_sort_by])

        for match in data:
            row = [match["peak_centre"], match["energy"], match["element"], match["transition"], match["error"], match["diff"]]
            table.addRow(row)
        return table


AlgorithmFactory.subscribe(PeakMatching)
