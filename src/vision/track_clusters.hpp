#pragma once

#include "vision/cluster_detections.hpp"
#include <vector>
#include <optional>
#include <limits>


struct TimestampedPosition {
    size_t frame;
    Eigen::Vector3f position;
};

struct Track {
    size_t id;
    std::vector<TimestampedPosition> positions;
    size_t age = 0;                // frames since creation
    size_t frames_missing = 0;     // consecutive frames without match
    bool confirmed = false;
};


/**
 * Tracks clusters over time using minimum-displacement matching.
 * 
 * Maintains a list of active tracks and matches incoming clusters
 * based on which assignment requires the least total movement.
 * Tracks must survive `min_age` frames to be confirmed (noise rejection).
 * 
 */
class ClusterTracker {
public:
    struct Config {
        size_t min_age = 3;              // frames before track is confirmed
        size_t max_missing = 5;          // frames before track is killed
        float max_distance = 5.0f;       // max displacement to consider a match (meters)
    };

    ClusterTracker() : config_() {}
    explicit ClusterTracker(Config config) : config_(config) {}

    /**
     * Process new frame of clusters.
     * Call once per frame with the clustered detections.
     */
    void update(const std::vector<Cluster>& clusters, size_t frame) {
        // Mark all tracks as unmatched for this frame
        std::vector<bool> track_matched(tracks_.size(), false);
        std::vector<bool> cluster_matched(clusters.size(), false);

        // Greedy matching: for each cluster, find closest track within threshold
        // (For single-object tracking this is fine; Hungarian would be overkill)
        for (size_t ci = 0; ci < clusters.size(); ++ci) {
            float best_dist = config_.max_distance;
            size_t best_track = SIZE_MAX;

            for (size_t ti = 0; ti < tracks_.size(); ++ti) {
                if (track_matched[ti]) continue;

                const auto& last_pos = tracks_[ti].positions.back().position;
                float dist = (clusters[ci].centroid - last_pos).norm();

                if (dist < best_dist) {
                    best_dist = dist;
                    best_track = ti;
                }
            }

            if (best_track != SIZE_MAX) {
                // Match found
                track_matched[best_track] = true;
                cluster_matched[ci] = true;

                Track& track = tracks_[best_track];
                track.positions.push_back({frame, clusters[ci].centroid});
                track.age++;
                track.frames_missing = 0;

                if (!track.confirmed && track.age >= config_.min_age) {
                    track.confirmed = true;
                }
            }
        }

        // Handle unmatched tracks: increment missing counter or kill
        for (size_t ti = tracks_.size(); ti-- > 0;) {
            if (!track_matched[ti]) {
                tracks_[ti].frames_missing++;
                if (tracks_[ti].frames_missing > config_.max_missing) {
                    tracks_.erase(tracks_.begin() + ti);
                }
            }
        }

        // Spawn new tracks for unmatched clusters
        for (size_t ci = 0; ci < clusters.size(); ++ci) {
            if (!cluster_matched[ci]) {
                Track new_track;
                new_track.id = next_id_++;
                new_track.positions.push_back({frame, clusters[ci].centroid});
                new_track.age = 1;
                tracks_.push_back(std::move(new_track));
            }
        }
    }

    /**
     * Get all confirmed tracks (real-time output).
     * These have survived long enough to be considered real objects.
     */
    std::vector<const Track*> getConfirmedTracks() const {
        std::vector<const Track*> result;
        for (const auto& track : tracks_) {
            if (track.confirmed) {
                result.push_back(&track);
            }
        }
        return result;
    }

    /**
     * Get all tracks (including tentative ones, for debugging).
     */
    const std::vector<Track>& getAllTracks() const {
        return tracks_;
    }

private:
    Config config_;
    std::vector<Track> tracks_;
    size_t next_id_ = 0;
};
