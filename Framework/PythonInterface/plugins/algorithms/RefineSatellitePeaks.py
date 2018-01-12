from mantid.kernel import *
from mantid.api import *
from mantid.simpleapi import *

import numpy as np
from scipy.spatial import KDTree
from scipy.cluster.vq import kmeans2
import scipy.cluster.hierarchy as hcluster


class RefineSatellitePeaks(PythonAlgorithm):

    def PyInit(self):
        self.declareProperty(IPeaksWorkspaceProperty(name="MainPeaks",
                                                     defaultValue="",
                                                     direction=Direction.Input),
                             doc="Main integer HKL peaks")

        self.declareProperty(IPeaksWorkspaceProperty(name="SatellitePeaks",
                                                     defaultValue="",
                                                     direction=Direction.Input),
                             doc="Positions of seeded satellite peaks")

        self.declareProperty(WorkspaceProperty(name="MDWorkspace",
                                               defaultValue="",
                                               direction=Direction.Input),
                             doc="Workspace to integrate predicted satellite peaks in")

        self.declareProperty(IPeaksWorkspaceProperty(name="OutputWorkspace",
                                                     defaultValue="",
                                                     direction=Direction.Output),
                             doc="All found satellite peaks")

        self.declareProperty('NumOfQs', -1, direction=Direction.Input)
        self.declareProperty('ClusterThreshold', 1.5, direction=Direction.Input)
        self.declareProperty('PeakRadius', 0.1, direction=Direction.Input)
        self.declareProperty('BackgroundInnerRadius', 0.1, direction=Direction.Input)
        self.declareProperty('BackgroundOuterRadius', 0.2, direction=Direction.Input)
        self.declareProperty('I/sigma', 2, direction=Direction.Input)

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
        return np.array([np.array([peak['h'], peak['k'], peak['l']]) for peak in peaks_workspace])

    def cluster_qs(self, qs, k=None, threshold=1.5):
        if k is not None:
            centroid, clusters = kmeans2(qs, k)
        else:
            clusters = hcluster.fclusterdata(qs, threshold, criterion="distance")
        return clusters, len(set(clusters))

    def average_clusters(self, qs, clusters):
        averaged_qs = []
        for cluster_index in set(clusters):
            averaged_qs.append(np.median(qs[clusters==cluster_index], axis=0))
        return np.array(averaged_qs)

    def find_q_vectors(self, nuclear_hkls, sats_hkls):
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
        predicted_satellites = nuclear.clone()
        for _ in range(predicted_satellites.getNumberPeaks()):
            predicted_satellites.removePeak(0)

        for q in qs:
            predicted_q = PredictFractionalPeaks(nuclear, HOffset=q[0], KOffset=q[1], LOffset=q[2])
            predicted_satellites = CombinePeaksWorkspaces(predicted_satellites, predicted_q)

        DeleteWorkspace(predicted_q)
        return predicted_satellites


AlgorithmFactory.subscribe(RefineSatellitePeaks)
