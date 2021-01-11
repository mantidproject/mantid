# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2021 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
import json
import os
from mantid.api import (AlgorithmFactory,WorkspaceProperty,PythonAlgorithm,ITableWorkspaceProperty,
                        FileProperty,FileAction,ITableWorkspace)
from mantid.simpleapi import CreateEmptyTableWorkspace
from mantid.kernel import Direction,StringMandatoryValidator
from mantid import mtd
from Muon.GUI import ElementalAnalysis

class PeakMatching(PythonAlgorithm):

    def category(self):
        return "Muon"

    def summary(self):
        return 'Matches peaks from table to a database to find probable transitions'

    def seeAlso(self):
        return [
            'FitGaussianPeaks', 'FindPeaks', 'FindPeaksAutomatic'
        ]

    def PyInit(self):

        self.declareProperty(ITableWorkspaceProperty(name="PeakTable",
                                               defaultValue="",
                                               direction=Direction.Input),
                             doc="Table containing peaks to match to database")

        self.declareProperty(FileProperty(name="PeakDatabase", defaultValue="",
                                          action=FileAction.OptionalLoad,
                                          extensions=["json"]),
                             doc = "json file with peak database, if none is given default database will be used")

        self.declareProperty("PeakCentreColumn", "centre",
                             doc="Name of column containing centre of peaks",
                             direction=Direction.Input)

        self.declareProperty("SigmaColumn","sigma",
                             doc="Name of column containing standard deviation of peaks",
                             direction=Direction.Input)

        # Output tables
        self.declareProperty(
            ITableWorkspaceProperty(name='AllPeaks',
                                    defaultValue='all_matches',
                                    direction=Direction.Output),
            doc='Name of the table containing all of the peak matches')

        self.declareProperty(
            ITableWorkspaceProperty(name='PrimaryPeaks',
                                    defaultValue='primary_matches',
                                    direction=Direction.Output),
            doc='Name of the table containing the primary peak matches')

        self.declareProperty(
            ITableWorkspaceProperty(name='SecondaryPeaks',
                                    defaultValue='secondary_matches',
                                    direction=Direction.Output),
            doc='Name of the table containing the secondary peak matches')

        self.declareProperty(
            ITableWorkspaceProperty(name='SortedByEnergy',
                                defaultValue='all_matches_sorted_by_energy',
                                direction=Direction.Output),
        doc='Name of the table containing all of the peak matches sorted by energy')

        self.declareProperty(
            ITableWorkspaceProperty(name='ElementCount',
                                    defaultValue='element_count',
                                    direction=Direction.Output),
            doc='Name of the table containing the count of elements in all matches')

    def get_default_peak_data(self):
        path = os.path.join(os.path.dirname(ElementalAnalysis.__file__), "peak_data.json")
        with open(path, 'r') as f:
            peak_data = json.load(f)
        return peak_data

    def PyExec(self):
        table = self.getPropertyValue("PeakTable")
        path = self.getPropertyValue("PeakDatabase")
        peak_data = self.process_peak_data(path)

        sigma_column = self.getProperty("SigmaColumn").value
        peak_column = self.getProperty("PeakCentreColumn").value
        input_peaks = self.get_input_peaks(table,peak_column,sigma_column)

        primary_data, secondary_data, all_data = self.get_matches(peak_data, input_peaks)
        names = [self.getPropertyValue("PrimaryPeaks"),
                 self.getPropertyValue("SecondaryPeaks"),
                 self.getPropertyValue("AllPeaks"),
                 self.getPropertyValue("SortedByEnergy"),
                 self.getPropertyValue("ElementCount")]
        self.output_data(primary_data, secondary_data, all_data,names)

    def process_peak_data(self,path):
        '''
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
                '''
        peak_data = None
        if path:
            with open(path, 'r') as f:
                peak_data = json.load(f)
        else:
            peak_data = self.get_default_peak_data()

        primary_energies = {}

        secondary_energies = {}

        all_energies = {}

        for element in peak_data:
            primary_energy = [float(x[1]) for x in list(peak_data[element]['Primary'].items())]
            secondary_energy = [float(x[1]) for x in list(peak_data[element]['Secondary'].items())]
            primary_trans = [x[0] for x in list(peak_data[element]['Primary'].items())]
            secondary_trans = [x[0] for x in list(peak_data[element]['Secondary'].items())]

            primary_energies[element] = dict(zip(primary_trans, primary_energy))
            secondary_energies[element] = dict(zip(secondary_trans, secondary_energy))
            all_energies[element] = dict(zip(primary_trans + secondary_trans,
                                             primary_energy + secondary_energy))

        peak_data = {}
        peak_data['Primary energy'] = primary_energies
        peak_data['Secondary energy'] = secondary_energies
        peak_data['All energies'] = all_energies

        return peak_data

    def make_count_table(self,name,data):
        table = CreateEmptyTableWorkspace(OutputWorkspace=name)

        table.addColumn("str", "Element")
        table.addColumn("int", "Counts")

        counts = {}

        for entry in data:
            if not(entry['element'] in counts):
                counts[entry['element']] = 1
            counts[entry['element']] += 1

        sorted_data_by_count = sorted(counts.items(),key=lambda x: x[1], reverse=True)

        for match in sorted_data_by_count:
            row = [str(match[0]),match[1]]
            table.addRow(row)
        return table

    def get_input_peaks(self,table,centre_column_name,sigma_column_name):
        '''DEFINING WIDTH OF PEAKS
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

        '''
        table_dict = mtd[table].toDict()
        peaks = []
        sigma = []
        for i in range(len(table_dict[centre_column_name])):
            peaks.append(table_dict[centre_column_name][i])
            sigma.append(table_dict[sigma_column_name][i])

        return list(zip(peaks, sigma))

    def get_matches(self,peak_data, input_peaks):
        all_matches = []
        for x in peak_data:
            raw_data = peak_data[x]
            matches = []
            for peak, sigma in input_peaks:
                for element in raw_data:
                    for transition, energy in raw_data[element].items():

                        if peak == energy:
                            data = {}
                            data['element'] = element
                            data['energy'] = energy
                            data['error'] = 0
                            data['peak_centre'] = peak
                            data['transition'] = transition
                            data['diff'] = 0
                            matches.append(data)

                        elif peak >= (energy - sigma) and peak <= (energy + sigma):
                            data = {}
                            data['element'] = element
                            data['energy'] = energy
                            data['error'] = sigma
                            data['peak_centre'] = peak
                            data['transition'] = transition
                            data['diff'] = abs(peak - energy)
                            matches.append(data)

                        elif peak >= (energy - (2 * sigma)) and peak <= (energy + (2 * sigma)):
                            data = {}
                            data['element'] = element
                            data['energy'] = energy
                            data['error'] = 2 * sigma
                            data['peak_centre'] = peak
                            data['transition'] = transition
                            data['diff'] = abs(peak - energy)
                            matches.append(data)

                        elif peak >= (energy - (3 * sigma)) and peak <= (energy + (3 * sigma)):
                            data['element'] = element
                            data['energy'] = energy
                            data['error'] = 3 * sigma
                            data['peak_centre'] = peak
                            data['transition'] = transition
                            data['diff'] = abs(peak - energy)
                            matches.append(data)
            matches = sorted(matches, key=lambda x: x['diff'])
            all_matches.append(matches)
        return all_matches

    def output_data(self,primary_data, secondary_data, all_data,names):
        prim_table = self.make_peak_table(names[0], primary_data)
        self.setPropertyValue('PrimaryPeaks', names[0])
        self.setProperty('PrimaryPeaks', prim_table)

        secon_table = self.make_peak_table(names[1], secondary_data)
        self.setPropertyValue('SecondaryPeaks', names[1])
        self.setProperty('SecondaryPeaks', secon_table)

        all_table = self.make_peak_table(names[2], all_data)
        self.setPropertyValue('AllPeaks', names[2])
        self.setProperty('AllPeaks', all_table)

        sorted_table = self.make_peak_table(names[3],all_data,True,"energy")
        self.setPropertyValue('SortedByEnergy', names[3])
        self.setProperty('SortedByEnergy', sorted_table)

        count_table = self.make_count_table(names[4],all_data)
        self.setPropertyValue('ElementCount', names[4])
        self.setProperty('ElementCount', count_table)


    def make_peak_table(self,name, data,sortBy = False,valueToSortBy = "energy"):
        table = CreateEmptyTableWorkspace(OutputWorkspace=name)

        table.addColumn("double", "Peak centre")

        table.addColumn("double", "Database Energy")

        table.addColumn("str", "Element")

        table.addColumn("str", "Transition")

        table.addColumn("double", "Error")

        table.addColumn("double", "Difference")

        if sortBy:
            data = sorted(data , key = lambda x: x[valueToSortBy])

        for match in data:
            row = [match['peak_centre'], match['energy'], match['element'],
                   match['transition'], match['error'], match['diff']]
            table.addRow(row)
        return table

AlgorithmFactory.subscribe(PeakMatching)
