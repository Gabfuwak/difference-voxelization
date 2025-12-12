#pragma once

#include <opencv2/opencv.hpp>
#include <limits>
#include "scene/scene_object.hpp"
#include "scene/camera.hpp"
#include "core/renderer.hpp"
#include <Eigen/Dense>
#include <vector>
#include <unordered_set>


struct Voxel {
    Eigen::Vector3f center;
    float half_size;
};

struct Ray {
    Eigen::Vector3f origin;
    Eigen::Vector3f direction;
    int camera_id;
    float pixel_angular_size; // angular size of the area this ray represents
};


struct CameraFrame {
    scene::Camera camera;
    cv::Mat current_frame;
    cv::Mat previous_frame;  // For temporal differencing
};


struct DetectionStats {
    size_t ray_count = 0;
    size_t nodes_visited = 0;
    size_t voxels_visited = 0;
    size_t intersection_checks = 0;
    size_t total_depth = 0;
    std::vector<size_t> checks_per_depth = std::vector<size_t>(30, 0);
    size_t rays_subdivided = 0;
    size_t total_subrays_created = 0;
};

std::vector<Ray> subdivideRay(const Ray& ray) {
    // Find orthogonal basis
    Eigen::Vector3f perp = std::abs(ray.direction.z()) < 0.9f 
        ? Eigen::Vector3f::UnitZ() 
        : Eigen::Vector3f::UnitX();
    
    Eigen::Vector3f u = ray.direction.cross(perp).normalized();
    Eigen::Vector3f v = ray.direction.cross(u);  // Already unit length
    
    float offset = ray.pixel_angular_size * 0.25f;
    float new_size = ray.pixel_angular_size * 0.5f;
    
    std::vector<Ray> sub_rays(4);
    int idx = 0;
    
    for (int i : {-1, 1}) {
        for (int j : {-1, 1}) {
            Eigen::Vector3f new_dir = ray.direction + (i * offset) * u + (j * offset) * v;
            sub_rays[idx++] = {ray.origin, new_dir.normalized(), ray.camera_id, new_size};
        }
    }
    
    return sub_rays;
}


std::vector<Ray> generateRays(const scene::Camera& camera, 
                               const std::vector<std::pair<float, float>>& pixels,
                               float screenWidth, float screenHeight, int camera_id) {
    Eigen::Matrix4f invViewProj = camera.getViewProjectionMatrix().inverse();
    
    std::vector<Ray> rays;
    rays.reserve(pixels.size());
    
    float fov_radians = camera.fov * (M_PI / 180.0f);
    float pixel_angular_size = fov_radians / screenWidth;
    for (const auto& [pixelX, pixelY] : pixels) {
        // Convert to NDC
        float ndcX = (2.0f * pixelX) / screenWidth - 1.0f;
        float ndcY = 1.0f - (2.0f * pixelY) / screenHeight;
        
        Eigen::Vector4f clipCoords(ndcX, ndcY, 1.0f, 1.0f);
        Eigen::Vector4f worldCoords = invViewProj * clipCoords;
        Eigen::Vector3f worldPoint = worldCoords.head<3>() / worldCoords.w();
        
        rays.push_back({camera.position, (worldPoint - camera.position).normalized(), camera_id, pixel_angular_size});
    }
    
    return rays;
}

// https://en.wikipedia.org/wiki/Slab_method
bool rayIntersectsVoxel(const Ray& ray, const Voxel& voxel) {
    float tmin = 0.0f;
    float tmax = std::numeric_limits<float>::infinity();
    
    for (int i = 0; i < 3; ++i) {
        // Compute min and max from center + half_size
        float voxel_min = voxel.center[i] - voxel.half_size;
        float voxel_max = voxel.center[i] + voxel.half_size;
        
        float t1 = (voxel_min - ray.origin[i]) / ray.direction[i];
        float t2 = (voxel_max - ray.origin[i]) / ray.direction[i];
        
        tmin = std::max(tmin, std::min(t1, t2));
        tmax = std::min(tmax, std::max(t1, t2));
    }
    
    return tmax >= tmin && tmax >= 0.0f;
}

/**
 * Returns the parameter t of the ray at which it enters the voxel.
 *
 * To get the entry point in global referential, run (ray.origin + ray.direction * getRayEntryT(ray, voxel))
 *
 * If the ray does not intersect with the voxel, returns -1.
 *
 */
float getRayEntryT(const Ray& ray, const Voxel& voxel) {
    float tmin = 0.0f;
    float tmax = std::numeric_limits<float>::infinity();
    
    for (int i = 0; i < 3; ++i) {
        // Compute min and max from center + half_size
        float voxel_min = voxel.center[i] - voxel.half_size;
        float voxel_max = voxel.center[i] + voxel.half_size;
        
        float t1 = (voxel_min - ray.origin[i]) / ray.direction[i];
        float t2 = (voxel_max - ray.origin[i]) / ray.direction[i];
        
        tmin = std::max(tmin, std::min(t1, t2));
        tmax = std::min(tmax, std::max(t1, t2));
    }
    
    if (tmax >= tmin && tmax >= 0.0f){
        return tmin;
    }else{
        return -1;
    }
}


/**
 * Traverses a ray through an n×n×n grid inside target_voxel using DDA.
 * Returns a map from voxel index to the t value when the ray entered that voxel.
 * Key is the flattened index of the ray: 
 */
std::vector<std::pair<int, float>> traverseGrid(const Ray& ray, float t_entry, const Voxel& target_voxel, int n){
    float voxel_size = (target_voxel.half_size * 2) / n;
    Eigen::Vector3f grid_min = target_voxel.center - Eigen::Vector3f::Constant(target_voxel.half_size);

    Eigen::Vector3f ray_entry_point = ray.origin + t_entry * ray.direction;

    Eigen::Vector3i step = { // in which direction do we need to go for each dimension
        (ray.direction.x() >= 0) ? 1 : -1,
        (ray.direction.y() >= 0) ? 1 : -1,
        (ray.direction.z() >= 0) ? 1 : -1
    };

    Eigen::Vector3f tDelta = {
        voxel_size / std::abs(ray.direction.x()),
        voxel_size / std::abs(ray.direction.y()),
        voxel_size / std::abs(ray.direction.z())
    };

    // curr_idx, initialize with entry point idx
    Eigen::Vector3i curr_idx_vec = {
        static_cast<int>(floor((ray_entry_point.x() - grid_min.x()) / voxel_size)),
        static_cast<int>(floor((ray_entry_point.y() - grid_min.y()) / voxel_size)),
        static_cast<int>(floor((ray_entry_point.z() - grid_min.z()) / voxel_size)),
    };
    curr_idx_vec = curr_idx_vec.cwiseMax(0).cwiseMin(n - 1);

    Eigen::Vector3f t_max;
    
    // initial t_max
    for(int i = 0; i<3; ++i){
        float next_boundary_distance = 0;
        if(step[i] > 0){
            next_boundary_distance = grid_min[i] + (curr_idx_vec[i] + 1) * voxel_size;
        }else{
            next_boundary_distance = grid_min[i] + curr_idx_vec[i] * voxel_size;
        }


        if(std::abs(ray.direction[i]) <= 0.00001f){ 
            t_max[i] = std::numeric_limits<float>::infinity();
        }else{
            t_max[i] = (next_boundary_distance - ray.origin[i]) / ray.direction[i];
        }
    }

    std::vector<std::pair<int, float>> result;
    result.reserve(n * 3); // rough estimate
    float curr_t = t_entry;

    while (curr_idx_vec.x() >= 0 && curr_idx_vec.x() < n 
        && curr_idx_vec.y() >= 0 && curr_idx_vec.y() < n 
        && curr_idx_vec.z() >= 0 && curr_idx_vec.z() < n) {

        // record curr voxel to result
        int idx = curr_idx_vec.x() + curr_idx_vec.y() * n + curr_idx_vec.z() * n * n;
        result.emplace_back(idx, curr_t);

        

        int min_axis = 0;
        // Find which axis has smallest tMax (next boundary hit)
        if (t_max[1] < t_max[min_axis]) min_axis = 1;
        if (t_max[2] < t_max[min_axis]) min_axis = 2;


        // Step in that direction, update tMax for that axis
        curr_idx_vec[min_axis] += step[min_axis];
        curr_t = t_max[min_axis];  // we enter the new voxel at this t
        t_max[min_axis] += tDelta[min_axis];
    }

    return result;
    
}


Voxel indexToVoxel(int idx, const Voxel& parent, int n) {
    int ix = idx % n;
    int iy = (idx / n) % n;
    int iz = idx / (n * n);
    
    float child_half_size = parent.half_size / n;
    float voxel_size = parent.half_size * 2.0f / n;
    Eigen::Vector3f grid_min = parent.center - Eigen::Vector3f::Constant(parent.half_size);
    
    Eigen::Vector3f child_center = grid_min + Eigen::Vector3f(
        (ix + 0.5f) * voxel_size,
        (iy + 0.5f) * voxel_size,
        (iz + 0.5f) * voxel_size
    );
    
    return Voxel{child_center, child_half_size};
}

/**
 * Populates the "detections" vector with all voxels where we might have found an object
 *
 *
 */
void recursive_detection(Voxel& target_zone, std::vector<Ray>& candidate_rays, float min_voxel_size, size_t min_ray_threshold, std::vector<Voxel>& detections,DetectionStats& stats, int subdiv_n, int depth = 0){

    stats.nodes_visited++;
    stats.total_depth += depth;

    // If we reached target size, make final detection
    float current_size = target_zone.half_size * 2.0;
    if (current_size <= min_voxel_size) {
        // No need to check ray voxel intersection because if it wasn't intersecting the recursion would not be called on this voxel
        detections.push_back(target_zone);
        return;
    }

    // Clamp subdivision so child voxels don't go below min_voxel_size
    int max_subdiv = static_cast<int>(current_size / min_voxel_size);
    subdiv_n = std::min(subdiv_n, std::max(2, max_subdiv));

    // Separate into n*n*n smaller voxels
    int total_cells = subdiv_n * subdiv_n * subdiv_n;  // 512 for 8×8×8
    std::vector<std::vector<Ray>> child_rays_map(total_cells);



    for(auto ray : candidate_rays){
        float t_entry = getRayEntryT(ray, target_zone); // get where the ray enters the target zone

        // Calculate if we need to subdivide
        float distance = t_entry;
        float ray_footprint = distance * ray.pixel_angular_size;
        float child_voxel_size = (target_zone.half_size * 2.0f) / subdiv_n;
        
        std::vector<Ray> rays_to_process;
        
        float threshold = 0.2f;
        if (ray_footprint > child_voxel_size * threshold) { // if the ray is bigger than the voxel size, we subdivide to avoid missing intersections because of sampling
            stats.rays_subdivided++;
            stats.total_subrays_created += 3; // 4 new rays but net +3
            rays_to_process = subdivideRay(ray);
        } else {
            rays_to_process = {ray};
        }

        for (const auto& r : rays_to_process) {
            float t = getRayEntryT(r, target_zone);
            if (t < 0) continue;  // skip if doesn't intersect
            
            stats.intersection_checks++;
            std::vector<std::pair<int, float>> intersections = traverseGrid(r, t, target_zone, subdiv_n);
            stats.voxels_visited += intersections.size();

            for (const auto& [voxel_idx, t_val] : intersections) {
                child_rays_map[voxel_idx].push_back(r);
            }
        }

    }
    // Recurse for children with enough cameras
    for (int voxel_idx = 0; voxel_idx < total_cells; ++voxel_idx) {
        auto& child_rays = child_rays_map[voxel_idx];
        
        if (child_rays.empty()) continue;
        
        // Count unique cameras from the rays themselves
        std::unordered_set<int> cameras;
        
        for (const auto& ray : child_rays) {
            cameras.insert(ray.camera_id);
        }

        if (cameras.size() >= min_ray_threshold) {
            Voxel child = indexToVoxel(voxel_idx, target_zone, subdiv_n);
            recursive_detection(child, child_rays, min_voxel_size, min_ray_threshold, detections, stats, subdiv_n, depth + 1);
        }
    }
}

/** 
 * Returns a list of voxels in which there is a possible detection
 * Each returned voxel represents a quadrant of the initial voxel (octree)
 *
 * We cast a ray in the direction of all movements in the camera.
 * If multiple ray intersect with the same voxel, there is a detection in that voxel. 
 * If we subdivide voxels enough, we will get a precise 3D location for the detection.
 *
 * Args:
 * - target_zone: Initial voxel where we want to detect objects
 * - camera_frames: camera parameters, current frame, and previous frame for ray calculation
 * - min_voxel_size: voxel size at which the algorithm will stop the recursion
 * - min_ray_threshold: how many rays have to hit one voxel in order to consider that it's a detection (will depend on the number of cameras aiming at the target zone)
 *
 * */
std::vector<Voxel> detect_objects(Voxel target_zone, const std::vector<CameraFrame>& camera_frames, float min_voxel_size = 0.1f, size_t min_ray_threshold = 3, int subdiv_n = 8){
    std::vector<Ray> all_rays;
    std::vector<Voxel> detections;
    DetectionStats stats;

    for (size_t cam_idx = 0; cam_idx < camera_frames.size(); ++cam_idx) {
        const auto& frame = camera_frames[cam_idx];

    // compute temporal difference with previous image
        cv::Mat diff;
        cv::absdiff(frame.current_frame, frame.previous_frame, diff);

        // If color, turn it to greyscale
        if (diff.channels() > 1) {
            cv::cvtColor(diff, diff, cv::COLOR_BGR2GRAY);
        }

        cv::Mat binary;
        int threshold = 5;
        cv::threshold(diff, binary, threshold, 255, cv::THRESH_BINARY);
        cv::imshow("Camera " + std::to_string(&frame - &camera_frames[0]) + " Diff", diff);

    // Get camera rays from all pixels where movement is detected by the temporal image difference
        std::vector<std::pair<float, float>> movement_pixels;
        std::vector<cv::Point> points;
        cv::findNonZero(binary, points);
        for (const auto& pt : points) {
            movement_pixels.push_back({static_cast<float>(pt.x), static_cast<float>(pt.y)});
        }

        auto rays = generateRays(frame.camera, movement_pixels, 
                                frame.current_frame.cols, frame.current_frame.rows, cam_idx);
        all_rays.insert(all_rays.end(), rays.begin(), rays.end());
        stats.ray_count = all_rays.size();
    }


    /*
    std::cout << "Total rays generated: " << all_rays.size() << std::endl;

    // Check how many pixels detected movement per camera
    int total_movement_pixels = 0;
    for (const auto& frame : camera_frames) {
        cv::Mat diff, binary;
        cv::absdiff(frame.current_frame, frame.previous_frame, diff);
        if (diff.channels() > 1) cv::cvtColor(diff, diff, cv::COLOR_BGR2GRAY);
        cv::threshold(diff, binary, 25, 255, cv::THRESH_BINARY);
        int movement_count = cv::countNonZero(binary);
        std::cout << "Camera movement pixels: " << movement_count << std::endl;
        total_movement_pixels += movement_count;
    }
    */

    // populate detections
    recursive_detection(target_zone, all_rays, min_voxel_size, min_ray_threshold, detections, stats, subdiv_n, 0);



    std::cout << "Subdivisions: " << stats.rays_subdivided 
          << ", Total subrays: " << stats.total_subrays_created << std::endl;
    return detections;

}





