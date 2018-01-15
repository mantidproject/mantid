from mantid.kernel import *
from mantid.api import *
from mantid.simpleapi import *

import numpy as np
from scipy.spatial import KDTree
from scipy.cluster.vq import kmeans2
import scipy.cluster.hierarchy as hcluster


class RefineSatellitePeaks(DataProcessorAlgorithm):

    def PyInit(self):
        self.declareProperty(IPeaksWorkspaceProperty(name="MainPeaks",
                                                     defaultValue="",
                                                     direction=Direction.Input),
                             doc="Main integer HKL peaks. Q vectors will be calculated relative to these peaks.")

        self.declareProperty(IPeaksWorkspaceProperty(name="SatellitePeaks",
                                                     defaultValue="",
                                                     direction=Direction.Input),
                             doc="Positions of seed satellite peaks. These will be used to define the q vectors for each satellite.")

        self.declareProperty(WorkspaceProperty(name="MDWorkspace",
                                               defaultValue="",
                                               direction=Direction.Input),
                             doc="Workspace to search for satellites peak in.")

        self.declareProperty(IPeaksWorkspaceProperty(name="OutputWorkspace",
                                                     defaultValue="",
                                                     direction=Direction.Output),
                             doc="All found satellite peaks. These will be given with satellite coordinates.")

        self.declareProperty('NumOfQs', -1, direction=Direction.Input,
                             doc="The number of satellite peaks to look for. If this option is not set to the default then all the \
                             provided satellites will be grouped into exactly this number of q vectors")
        self.declareProperty('ClusterThreshold', 1.5, direction=Direction.Input,
                             doc="Threshold for automaticallty deciding on the number of q vectors to use. If NumOfQs found is set then this \
                             is property is ignored.")
        self.declareProperty('PeakRadius', 0.1, direction=Direction.Input,
                             doc="The peak radius used to integrate the satellite peaks. This is passed direclty to IntegratePeaksMD")
        self.declareProperty('BackgroundInnerRadius', 0.1, direction=Direction.Input,
                             doc="The inner background radius used to integrate the satellite peaks. This is passed direclty to \
                             IntegratePeaksMD")
        self.declareProperty('BackgroundOuterRadius', 0.2, direction=Direction.Input,
                             doc="The outer background radius used to integrate satellite peaks. This is passed direclty to \
                             IntegratePeaksMD")
        self.declareProperty('I/sigma', 2, direction=Direction.Input,
                             doc="The I/sigma threshold use to identify if peaks exist. This is passed direclty to FilterPeaks")

    def PyExec(self):
        k = self.getProperty("NumOfQs").value
        peak_radius = self.getProperty("PeakRadius").value
        background_radii = (self.getProperty("BackgroundInnerRadius").value, self.getProperty("BackgroundOuterRadius").value)
        I_over_sigma = self.getProperty("I/sigma").value
        cluster_threshold = self.getProperty("ClusterThreshold").value

        # if user did not specify the number of qs then
        # set the k value to None
        if k == -1:
            k = None

        md = self.getProperty("MDWorkspace").value
        nuclear = self.getProperty("MainPeaks").value
        sats = self.getProperty("SatellitePeaks").value

        nuclear_hkls = self.get_hkls(nuclear)
        sats_hkls = self.get_hkls(sats)

        qs = self.find_q_vectors(nuclear_hkls, sats_hkls)
        clusters, k = self.cluster_qs(qs, threshold=cluster_threshold)
        qs = self.average_clusters(qs, clusters)
        predicted_satellites = self.create_fractional_peaks_workspace(qs, nuclear)

        centroid_satellites = CentroidPeaksMD(md, predicted_satellites, PeakRadius=peak_radius)
        satellites_int_spherical = IntegratePeaksMD(md, centroid_satellites, PeakRadius=peak_radius,
                                                    BackgroundInnerRadius=background_radii[0], BackgroundOuterRadius=background_radii[1], IntegrateIfOnEdge=True)
        satellites_int_spherical = FilterPeaks(satellites_int_spherical, FilterVariable="Intensity", FilterValue=0, Operator=">")
        satellites_int_spherical = FilterPeaks(satellites_int_spherical, FilterVariable="Signal/Noise",
                                               FilterValue=I_over_sigma, Operator=">")

        DeleteWorkspace(predicted_satellites)
        DeleteWorkspace(centroid_satellites)

        name = self.getPropertyValue("OutputWorkspace")
        RenameWorkspace(satellites_int_spherical, name)

        self.log().notice("Q vectors are: \n{}".format(qs))
        self.setProperty("OutputWorkspace", satellites_int_spherical)

    def get_hkls(self, peaks_workspace):
        """Return a 2D numpy array from a peaks workspace.

        :param peaks_workpace: the PeaksWorkspace to extract HKL values from
        :returns: np.ndarry -- 2D array of HKL values
        """
        return np.array([np.array([peak['h'], peak['k'], peak['l']]) for peak in peaks_workspace])

    def cluster_qs(self, qs, k=None, threshold=1.5):
        """Cluster q vectors into discrete groups.

        Classifies each of the q vectors into a number of clusters. The number of clusters used is decided by the parameters passed:
            * If the k parameter is supplied then the q vectors are grouped into k clusters using kmeans. 
            * If the threshold parameter is supplied then the q vectors a split into groups based on cophenetic distance.

        :param qs: list of q vectors to cluster. Each element should be a numpy array of length three.
        :param k: number of clusters to use (optional).
        :param threshold: cophenetic distance cut off point for new clusters (optional)
        :returns: tuple (clusters, k)
            Where:
                list -- clusters is a list of cluster indicies which each q belongs to
                int -- k is the number of clusters used
        """
        if k is not None:
            centroid, clusters = kmeans2(qs, k)
        else:
            clusters = hcluster.fclusterdata(qs, threshold, criterion="distance")
        return clusters, len(set(clusters))

    def average_clusters(self, qs, clusters):
        """Find the centroid of the clusters.

        For each q vector, group them by their designated cluster and then compute 
        the average of the group.

        :param qs: list of q vectors. Each element should be a numpy array.
        :param clusters: the indicies of the cluster that each q belongs to.
        :returns: np.ndarry -- the list of centroids for each cluster.
        """

        averaged_qs = []
        for cluster_index in set(clusters):
            averaged_qs.append(np.mean(qs[clusters==cluster_index], axis=0))
        return np.array(averaged_qs)

    def find_q_vectors(self, nuclear_hkls, sats_hkls):
        """Find the q vector between the nuclear HKLs and the satellite peaks

        Given a list of HKL positions and a list of fractional HKL positions of
        satellite peaks, find the difference between each satellite and its nearest
        integer HKL.

        :param nuclear_hkls: the positions of integer HKL peaks.
        :param sats_hkl: the positions of fractional "satellite" HKL peaks.
        :returns: np.ndarray -- array of q vectors.
        """
        peak_map = KDTree(nuclear_hkls)
        qs = []
        for sat in sats_hkls:
            distance, index = peak_map.query(sat, k=1)
            if distance > 2:
                #peak to far away from satellite ignore
                continue
            nearest_peak = nuclear_hkls[index]
            qs.append(sat - nearest_peak)
        return np.array(qs)

    def create_fractional_peaks_workspace(self, qs, nuclear):
        """Generate a peaks workspace of possible satellite peaks from a list of q vectors.

        :param qs: list of q vectors to use to generate fractional peaks.
        :param nuclear: list of integer HKL peak positions.
        :returns: PeaksWorkspace -- containing predicted locations of satellite peaks.
        """
        predicted_satellites = nuclear.clone()
        for _ in range(predicted_satellites.getNumberPeaks()):
            predicted_satellites.removePeak(0)

        for q in qs:
            predicted_q = PredictFractionalPeaks(nuclear, HOffset=q[0], KOffset=q[1], LOffset=q[2])
            predicted_satellites = CombinePeaksWorkspaces(predicted_satellites, predicted_q)

        DeleteWorkspace(predicted_q)
        return predicted_satellites


AlgorithmFactory.subscribe(RefineSatellitePeaks)
