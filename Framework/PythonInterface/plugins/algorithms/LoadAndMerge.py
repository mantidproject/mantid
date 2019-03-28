# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
#     NScD Oak Ridge National Laboratory, European Spallation Source
#     & Institut Laue - Langevin
# SPDX - License - Identifier: GPL - 3.0 +
from __future__ import (absolute_import, division, print_function)

import os.path
from mantid.kernel import Direction, StringContainsValidator, PropertyManagerProperty
from mantid.api import AlgorithmFactory, AlgorithmManager, MultipleFileProperty, \
    WorkspaceProperty, PythonAlgorithm, FileLoaderRegistry, Progress
from mantid.simpleapi import MergeRuns, RenameWorkspace, DeleteWorkspace, GroupWorkspaces, mtd


class LoadAndMerge(PythonAlgorithm):

    _loader = None
    _version = None
    _loader_options = None
    _prefix = ''
    _progress = None

    def seeAlso(self):
        return [ "Load","MergeRuns" ]

    def name(self):
        return "LoadMergeRuns"

    def category(self):
        return "DataHandling"

    def summary(self):
        return 'Loads and merges multiple runs. Similar to Load, but uses MergeRuns instead of Plus for summing.'

    def validateInputs(self):
        issues = dict()
        loader = self.getPropertyValue('LoaderName')
        version = self.getProperty('LoaderVersion').value
        try:
            AlgorithmManager.createUnmanaged(loader, version)
        except RuntimeError:
            message = loader + '-v' + str(version) + ' is not registered with Mantid.'
            issues['LoaderName'] = message
            issues['LoaderVersion'] = message
        return issues

    def PyInit(self):
        self.declareProperty(MultipleFileProperty('Filename'), doc='List of input files')
        self.declareProperty('LoaderName', defaultValue='Load', validator=StringContainsValidator(['Load']),
                             direction=Direction.InOut,
                             doc='The name of the specific loader. Generic Load by default.')
        self.declareProperty('LoaderVersion', defaultValue=-1, direction=Direction.InOut,
                             doc='The version of the specific loader')
        self.declareProperty(PropertyManagerProperty('LoaderOptions',dict()),
                             doc='Options for the specific loader')
        self.declareProperty(PropertyManagerProperty('MergeRunsOptions',dict()),
                             doc='Options for merging the metadata')
        self.declareProperty(WorkspaceProperty('OutputWorkspace', '', direction=Direction.Output),
                             doc='Output workspace or workspace group.')

    def _load(self, run, runnumber):
        """
            Loads the single run using the specific loader
            @param run : the full file path
            @param runnumber : the run number
        """
        self._progress.report('Loading '+runnumber)
        alg = self._create_fresh_loader()
        alg.setPropertyValue('Filename', run)
        alg.setPropertyValue('OutputWorkspace', runnumber)
        alg.execute()

    def _create_fresh_loader(self):
        """
            Creates a fresh instance of the specific loader.
            @return : initialized and configured loader
        """
        # We need to create a fresh instance for each file, since
        # there might be loaders that do not reset their private members after execution.
        # So running on the same instance can potentially cause problems.
        # Also the output will always be on ADS, since this algorithm relies on
        # MergeRuns, which does not work outside ADS (because of WorkspaceGroup input)
        alg = self.createChildAlgorithm(self._loader, self._version)
        alg.setAlwaysStoreInADS(True)
        alg.setLogging(self.isLogging())
        alg.initialize()
        for key in self._loader_options.keys():
            alg.setProperty(key, self._loader_options.getProperty(key).value)
        return alg

    def PyExec(self):
        runs = self.getProperty('Filename').value
        runs_as_str = self.getPropertyValue('Filename')
        number_runs = runs_as_str.count(',') + runs_as_str.count('+') + 1
        self._progress = Progress(self, start=0.0, end=1.0, nreports=number_runs)
        self._loader = self.getPropertyValue('LoaderName')
        self._version = self.getProperty('LoaderVersion').value
        self._loader_options = self.getProperty('LoaderOptions').value
        merge_options = self.getProperty('MergeRunsOptions').value
        output = self.getPropertyValue('OutputWorkspace')
        if output.startswith('__'):
            self._prefix = '__'

        # get the first run
        to_group = []
        first_run = runs[0]
        if isinstance(first_run, list):
            first_run = first_run[0]

        if self._loader == 'Load':
            # figure out the winning loader
            winning_loader = FileLoaderRegistry.Instance().chooseLoader(first_run)
            self._loader = winning_loader.name()
            self._version = winning_loader.version()
            self.setPropertyValue('LoaderName', self._loader)
            self.setProperty('LoaderVersion', self._version)

        for runs_to_sum in runs:
            if not isinstance(runs_to_sum, list):
                run = runs_to_sum
                runnumber = self._prefix + os.path.basename(run).split('.')[0]
                self._load(run, runnumber)
                to_group.append(runnumber)
            else:
                runnumbers = self._prefix
                merged = ''
                for i, run in enumerate(runs_to_sum):
                    runnumber = os.path.basename(run).split('.')[0]
                    runnumbers += '_' + runnumber
                    runnumber = self._prefix + runnumber
                    self._load(run, runnumber)
                    if i == 0:
                        merged = runnumber
                    else:
                        # we need to merge to a temp name, and rename later,
                        # since if the merged is a group workspace,
                        # it's items will be orphaned
                        tmp_merged = '__tmp_' + merged
                        MergeRuns(InputWorkspaces=[merged, runnumber],
                                  OutputWorkspace=tmp_merged, **merge_options)
                        DeleteWorkspace(Workspace=runnumber)
                        DeleteWorkspace(Workspace=merged)
                        RenameWorkspace(InputWorkspace=tmp_merged, OutputWorkspace=merged)

                runnumbers = runnumbers[1:]
                RenameWorkspace(InputWorkspace=merged, OutputWorkspace=runnumbers)
                to_group.append(runnumbers)

        if len(to_group) != 1:
            GroupWorkspaces(InputWorkspaces=to_group, OutputWorkspace=output)
        else:
            RenameWorkspace(InputWorkspace=to_group[0], OutputWorkspace=output)

        self.setProperty('OutputWorkspace', mtd[output])

AlgorithmFactory.subscribe(LoadAndMerge)
