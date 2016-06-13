# from SANSFileInformation import (SANSInstrument, SANSFileInformationFactory)
# from mantid.api import(AlgorithmManager, AlgorithmFactory)
#
#
# class SANSMoveWorkspace(DataProcessorAlgorithm):
#     def category(self):
#         return 'SANS'
#
#     def summary(self):
#         return 'Load SANS data'
#
#     def PyInit(self):
#         self.declareProperty(MatrixWorkspaceProperty('InputWorkspace', '', direction=Direction.Input),
#                              doc='The input workspace')
#         self.declareProperty(MatrixWorkspaceProperty('OutputWorkspaceP', '',
#                                                      direction=Direction.Output),
#                              doc='The moved workspace')
#         # Translation
#         self.declareProperty(name='x', defaultValue=0,
#                              doc='The translation along the x direction.')
#         self.declareProperty(name='y', defaultValue=0,
#                              doc='The translation along the y direction.')
#         self.declareProperty(name='z', defaultValue=0,
#                              doc='The translation along the z direction.')
#
#         # Rotation
#         self.declareProperty(name='RotationAngleX', defaultValue=0,
#                              doc='The rotation around the x axis.'
#                                  'Note that the order of rotation is defined internally.')
#
#         self.declareProperty(name='RotationAngleY', defaultValue=0,
#                              doc='The rotation around the y axis.'
#                                  'Note that the order of rotation is defined internally.')
#
#         self.declareProperty(name='RotationAngleZ', defaultValue=0,
#                              doc='The rotation around the y axis.'
#                                  'Note that the order of rotation is defined internally.')
#
#     def PyExec(self):
#         pass
#
#
# class SANSMoveWorkspaceLOQ(SANSMoveWorkspace):
#     def category(self):
#         return "SANS"
#
#     def summary(self):
#         return "Moves a LOQ workspace"
#
#     def PyInit(self):
#         super(SANSMoveWorkspaceLOQ, self).PyInit()
#
#     def PyExec(self):
#         input_workspace = self.getProperty("InputWorkspace").value
#
#
# class SANSMoveWorkspaceLARMOR(SANSMoveWorkspace):
#     def category(self):
#         return "SANS"
#
#     def summary(self):
#         return "Moves a LARMOR workspace"
#
#     def PyInit(self):
#         pass
#
#     def PyExec(self):
#         pass
#
#
# class SANSMoveWorkspaceSANS2D(SANSMoveWorkspace):
#     def category(self):
#         return "SANS"
#
#     def summary(self):
#         return "Moves a SANS2D workspace"
#
#     def PyInit(self):
#         pass
#
#     def PyExec(self):
#         pass
#
#
# # Register algorithm with Mantid
# AlgorithmFactory.subscribe(SANSMoveWorkspace)
# AlgorithmFactory.subscribe(SANSMoveWorkspaceLOQ)
# AlgorithmFactory.subscribe(SANSMoveWorkspaceLARMOR)
# AlgorithmFactory.subscribe(SANSMoveWorkspaceSANS2D)
#
#
# class SANSMoveWorkspaceFactory(object):
#     def __init__(self):
#         super(SANSMoveWorkspaceFactory, self).__init__()
#         self._file_information_factory = SANSFileInformationFactory()
#
#     def create_mover(self, file_name):
#         # Get the instrument from the file_name
#         file_information = self._file_information_factory.create_sans_file_information(file_name)
#         instrument_type = file_information.get_instrument(file_name)
#
#         if instrument_type == SANSInstrument.LARMOR:
#             mover = AlgorithmManager.createUnmanaged("SANSMoveWorkspaceLARMOR")
#         elif instrument_type == SANSInstrument.LOQ:
#             mover = AlgorithmManager.createUnmanaged("SANSMoveWorkspaceLOQ")
#         elif instrument_type == SANSInstrument.SANS2D:
#             mover = AlgorithmManager.createUnmanaged("SANSMoveWorkspaceSANS2D")
#         else:
#             NotImplementedError("SANSLoaderFactory: Other instruments are not implemented yet.")
#         mover.setChild(True)
#         mover.initialize()
#         return mover
