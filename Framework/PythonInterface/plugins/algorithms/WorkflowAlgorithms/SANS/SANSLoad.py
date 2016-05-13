# from mantid.api import *
# from SANSFileInformation import (SANSInstrument, SANSFileInformation, SANSFileInformationFactory)
#
#
# class SANSLoad(DataProcessorAlgorithm):
#     def category(self):
#         return 'SANS\\Load'
#
#     def summary(self):
#         return 'Load SANS data'
#
#     def PyInit(self):
#         # ----------
#         # INPUT
#         # ----------
#         self.declareProperty('SANSState', '', direction=Direction.In,
#                              doc='A property manager which fulfills the SANSState contract.')
#
#         self.declareProperty("PublishToCache", True,
#                              "Publish the loaded files to a cache, in order to avoid reloading for subsequent runs.")
#
#         self.declareProperty("UseCached", True,
#                              "Checks if there are loaded files available. If they are, those files are used.")
#
#         self.declareProperty("MoveWorkspace", False,
#                              "Move the workspace according to the SANSState setting. This might be useful"
#                              "for manual inspection.")
#
#         # ------------
#         #  OUTPUT
#         # ------------
#         # Sample Scatter Workspaces
#         self.declareProperty(MatrixWorkspaceProperty('SampleScatterWorkspace', '', direction=Direction.Output),
#                              doc='The sample scatter workspace. This workspace does not contain monitors.')
#         self.declareProperty(MatrixWorkspaceProperty('SampleScatterMonitorWorkspace', '', direction=Direction.Output),
#                              doc='The sample scatter monitor workspace. This workspace only contains monitors.')
#
#         # Sample Transmission Workspace
#
#         # Sample Direct Workspace
#
#         # Can Scatter Workspaces
#
#         # Can Transmission Workspace
#
#         # Can Direct Workspace
#
#     def PyExec(self):
#         pass
#
#
# class SANSLoadISIS(SANSLoad):
#     def category(self):
#         return 'SANS\\Load'
#
#     def summary(self):
#         return 'Load ISIS SANS data'
#
#     def PyInit(self):
#         super(SANSLoadISIS, self).PyInit()
#
#     def PyExec(self):
#         # Do loading work here
#         pass
#
#
# # Register algorithm with Mantid
# AlgorithmFactory.subscribe(SANSLoad)
# AlgorithmFactory.subscribe(SANSLoadISIS)
#
#
# class SANSLoadFactory(object):
#     def __init__(self):
#         super(SANSLoadFactory, self).__init__()
#         self._file_information_factory = SANSFileInformationFactory()
#
#     def create_loader(self, file_name):
#         # Get the instrument from the file_name
#         file_information = self._file_information_factory.create_sans_file_information(file_name)
#         instrument_type = file_information.get_instrument(file_name)
#
#         if instrument_type == SANSInstrument.LARMOR or instrument_type == SANSInstrument.LOQ or\
#            instrument_type == SANSInstrument.SANS2D:
#             loader = AlgorithmManager.createUnmanaged("SANSLoadISIS")
#         else:
#             NotImplementedError("SANSLoaderFactory: Other instruments are not implemented yet.")
#         loader.setChild(True)
#         loader.initialize()
#         return loader
#
#
