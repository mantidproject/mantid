# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
from mantid.api import AlgorithmFactory, DataProcessorAlgorithm, MDFrameValidator, WorkspaceProperty
from mantid.dataobjects import PeaksWorkspaceProperty
from mantid.kernel import Direction
from mantid.simpleapi import CentroidPeaksMD, CloneWorkspace, CombinePeaksWorkspaces, FilterPeaks, IntegratePeaksMD, PredictFractionalPeaks
import fractional_indexing as indexing


class FindSatellitePeaks(DataProcessorAlgorithm):
    def category(self):
        return "Crystal\\Peaks"

    def seeAlso(self):
        return ["IndexSatellitePeaks"]

    def name(self):
        return "FindSatellitePeaks"

    def summary(self):
        return "Algorithm for finding satellite peaks in an MDWorkspace in the HKL frame."

    def PyInit(self):
        self.declareProperty(
            PeaksWorkspaceProperty(name="NuclearPeaks", defaultValue="", direction=Direction.Input),
            doc="Main integer HKL peaks. Q vectors will be calculated relative to these peaks.",
        )

        self.declareProperty(
            PeaksWorkspaceProperty(name="SatellitePeaks", defaultValue="", direction=Direction.Input),
            doc="Positions of seed satellite peaks. These will be used to define the modulation (q) \
                             vectors for each satellite.",
        )

        self.declareProperty(
            WorkspaceProperty(name="MDWorkspace", defaultValue="", direction=Direction.Input, validator=MDFrameValidator("HKL")),
            doc="MD workspace to search for satellites peak in. This data must be in the HKL frame.",
        )

        self.declareProperty(
            PeaksWorkspaceProperty(name="OutputWorkspace", defaultValue="", direction=Direction.Output),
            doc="All found satellite peaks. These will be given with satellite coordinates.",
        )

        self.declareProperty(
            "NumOfQs",
            -1,
            direction=Direction.Input,
            doc="The number of satellite peaks to look for. If this option is not set to the default then all the \
                             provided satellites will be grouped into exactly this number of modulation (q) vectors",
        )
        self.declareProperty(
            "ClusterThreshold",
            1.5,
            direction=Direction.Input,
            doc="Threshold for automaticallty deciding on the number of modulation (q) vectors to use. "
            "If NumOfQs found is set then this is property is ignored.",
        )
        self.declareProperty(
            "PeakRadius",
            0.1,
            direction=Direction.Input,
            doc="The peak radius used to integrate the satellite peaks. This is Euclidean distance in HKL space. \
                             This is passed directly to IntegratePeaksMD",
        )
        self.declareProperty(
            "BackgroundInnerRadius",
            0.1,
            direction=Direction.Input,
            doc="The inner background radius used to integrate the satellite peaks."
            " This is Euclidean distance in HKL space. This is passed directly to IntegratePeaksMD",
        )
        self.declareProperty(
            "BackgroundOuterRadius",
            0.2,
            direction=Direction.Input,
            doc="The outer background radius used to integrate satellite peaks. "
            "This is Euclidean distance in HKL space. his is passed directly to IntegratePeaksMD",
        )
        self.declareProperty(
            "IOverSigma",
            2,
            direction=Direction.Input,
            doc="The I/sigma threshold use to identify if peaks exist. This is passed direclty to FilterPeaks",
        )

    def PyExec(self):
        k = self.getProperty("NumOfQs").value
        peak_radius = self.getProperty("PeakRadius").value
        background_radii = (self.getProperty("BackgroundInnerRadius").value, self.getProperty("BackgroundOuterRadius").value)
        I_over_sigma = self.getProperty("IOverSigma").value
        cluster_threshold = self.getProperty("ClusterThreshold").value

        # if user did not specify the number of qs then
        # set the k value to None
        if k == -1:
            k = None

        md = self.getProperty("MDWorkspace").value
        nuclear = self.getProperty("NuclearPeaks").value
        sats = self.getProperty("SatellitePeaks").value

        nuclear_hkls = indexing.get_hkls(nuclear)
        sats_hkls = indexing.get_hkls(sats)

        qs = indexing.find_q_vectors(nuclear_hkls, sats_hkls)
        clusters, k = indexing.cluster_qs(qs, threshold=cluster_threshold, k=k)
        qs = indexing.average_clusters(qs, clusters)
        predicted_satellites = self.create_fractional_peaks_workspace(qs, nuclear)

        centroid_satellites = CentroidPeaksMD(
            InputWorkspace=md, PeaksWorkspace=predicted_satellites, PeakRadius=peak_radius, StoreInADS=False
        )
        satellites_int_spherical = IntegratePeaksMD(
            InputWorkspace=md,
            PeaksWorkspace=centroid_satellites,
            PeakRadius=peak_radius,
            BackgroundInnerRadius=background_radii[0],
            BackgroundOuterRadius=background_radii[1],
            IntegrateIfOnEdge=True,
            StoreInADS=False,
        )
        satellites_int_spherical = FilterPeaks(
            satellites_int_spherical, FilterVariable="Intensity", FilterValue=0, Operator=">", StoreInADS=False
        )
        satellites_int_spherical = FilterPeaks(
            satellites_int_spherical, FilterVariable="Signal/Noise", FilterValue=I_over_sigma, Operator=">", StoreInADS=False
        )

        self.log().notice("Q vectors are: \n{}".format(qs))
        self.setProperty("OutputWorkspace", satellites_int_spherical)

    def create_fractional_peaks_workspace(self, qs, nuclear):
        """Generate a peaks workspace of possible satellite peaks from a list of q vectors.

        :param qs: list of q vectors to use to generate fractional peaks.
        :param nuclear: list of integer HKL peak positions.
        :returns: PeaksWorkspace -- containing predicted locations of satellite peaks.
        """
        predicted_satellites = CloneWorkspace(nuclear, StoreInADS=False)
        for _ in range(predicted_satellites.getNumberPeaks()):
            predicted_satellites.removePeak(0)

        for q in qs:
            predicted_q = PredictFractionalPeaks(
                nuclear, HOffset=q[0], KOffset=q[1], LOffset=q[2], StoreInADS=False, FracPeaks="predicted_q"
            )
            predicted_satellites = CombinePeaksWorkspaces(
                predicted_satellites, predicted_q, StoreInADS=False, OutputWorkspace="predicted_satellites"
            )

        return predicted_satellites


AlgorithmFactory.subscribe(FindSatellitePeaks)
