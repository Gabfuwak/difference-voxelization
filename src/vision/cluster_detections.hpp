#pragma once

#include "vision/detect_object.hpp"


struct Cluster {
    std::vector<Voxel> voxels;
    Eigen::Vector3f centroid;
};



/**
 * Clusters detection voxels using DBSCAN-style connectivity.
 * 
 * Voxels within epsilon distance are considered neighbors.
 * Connected components form clusters, filtered by minimum size.
 *
 * Args:
 * - detections: voxels from detect_objects
 * - min_voxel_size: used to compute epsilon (epsilon = epsilon_factor * min_voxel_size)
 * - epsilon_factor: multiplier for min_voxel_size to get neighbor distance threshold
 * - min_cluster_size: clusters smaller than this are discarded as noise
 */
std::vector<Cluster> clusterDetections(
    const std::vector<Voxel>& detections,
    float min_voxel_size,
    float epsilon_factor = 2.5f,
    size_t min_cluster_size = 3
) {
    if (detections.empty()) {
        return {};
    }

    float epsilon = epsilon_factor * min_voxel_size;
    float epsilon_sq = epsilon * epsilon;
    size_t n = detections.size();

    // Build adjacency list using squared distances
    std::vector<std::vector<size_t>> neighbors(n);
    for (size_t i = 0; i < n; ++i) {
        for (size_t j = i + 1; j < n; ++j) {
            float dist_sq = (detections[i].center - detections[j].center).squaredNorm();
            if (dist_sq <= epsilon_sq) {
                neighbors[i].push_back(j);
                neighbors[j].push_back(i);
            }
        }
    }

    // BFS to find connected components
    std::vector<int> labels(n, -1);
    int current_label = 0;

    for (size_t i = 0; i < n; ++i) {
        if (labels[i] != -1) continue;

        // BFS from this voxel
        std::vector<size_t> queue;
        queue.push_back(i);
        labels[i] = current_label;

        size_t head = 0;
        while (head < queue.size()) {
            size_t curr = queue[head++];
            for (size_t neighbor : neighbors[curr]) {
                if (labels[neighbor] == -1) {
                    labels[neighbor] = current_label;
                    queue.push_back(neighbor);
                }
            }
        }
        current_label++;
    }

    // Group voxels by cluster and compute centroids
    std::vector<Cluster> clusters(current_label);
    for (size_t i = 0; i < n; ++i) {
        clusters[labels[i]].voxels.push_back(detections[i]);
    }

    for (auto& cluster : clusters) {
        Eigen::Vector3f sum = Eigen::Vector3f::Zero();
        for (const auto& voxel : cluster.voxels) {
            sum += voxel.center;
        }
        cluster.centroid = sum / static_cast<float>(cluster.voxels.size());
    }

    // Filter by minimum size
    std::vector<Cluster> result;
    for (auto& cluster : clusters) {
        if (cluster.voxels.size() >= min_cluster_size) {
            result.push_back(std::move(cluster));
        }
    }

    return result;
}


