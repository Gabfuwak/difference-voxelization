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
};

struct CameraFrame {
    scene::Camera camera;
    cv::Mat current_frame;
    cv::Mat previous_frame;  // For temporal differencing
};




std::vector<Ray> generateRays(const scene::Camera& camera, 
                               const std::vector<std::pair<float, float>>& pixels,
                               float screenWidth, float screenHeight, int camera_id) {
    Eigen::Matrix4f invViewProj = camera.getViewProjectionMatrix().inverse();
    
    std::vector<Ray> rays;
    rays.reserve(pixels.size());
    
    for (const auto& [pixelX, pixelY] : pixels) {
        // Convert to NDC
        float ndcX = (2.0f * pixelX) / screenWidth - 1.0f;
        float ndcY = 1.0f - (2.0f * pixelY) / screenHeight;
        
        Eigen::Vector4f clipCoords(ndcX, ndcY, 1.0f, 1.0f);
        Eigen::Vector4f worldCoords = invViewProj * clipCoords;
        Eigen::Vector3f worldPoint = worldCoords.head<3>() / worldCoords.w();
        
        rays.push_back({camera.position, (worldPoint - camera.position).normalized(), camera_id});
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
 * Populates the "detections" vector with all voxels where we might have found an object
 *
 *
 */
void recursive_detection(Voxel& target_zone, std::vector<Ray>& candidate_rays, float min_voxel_size, size_t min_ray_threshold, std::vector<Voxel>& detections){


    // If we reached target size, make final detection
    float current_size = target_zone.half_size * 2.0;
    if (current_size <= min_voxel_size) {
        // count intersecting rays
        std::unordered_set<int> unique_cameras;
        for (auto ray : candidate_rays) {
            if (rayIntersectsVoxel(ray, target_zone)) {
                unique_cameras.insert(ray.camera_id);
            }

        }
        
        // if enough rays from different cameras hit, this is a true detection
        if (unique_cameras.size() >= min_ray_threshold) {
            detections.push_back(target_zone);
        }
        return;
    }
    // Separate into 8 smaller voxels
    float new_half_size = target_zone.half_size / 2.0;
    for(int x = 0; x<2; ++x){
        for(int y = 0; y<2; ++y){
            for(int z = 0; z<2; ++z){
                // 0 is the negative direction, 1 is the positive direction
                Eigen::Vector3f offset(
                    (x == 0) ? -new_half_size : new_half_size,
                    (y == 0) ? -new_half_size : new_half_size,
                    (z == 0) ? -new_half_size : new_half_size
                );

                Voxel child{target_zone.center + offset, new_half_size};

                // We only keep the rays that interact with this child for optimisation
                std::vector<Ray> child_rays;
                std::unordered_set<int> child_cameras;
                for (const auto ray : candidate_rays) {
                    if (rayIntersectsVoxel(ray, child)) {
                        child_rays.push_back(ray);
                        child_cameras.insert(ray.camera_id);
                    }
                }
                
                // Only recurse if the child is a potential detection
                if (child_cameras.size() >= min_ray_threshold) {
                    recursive_detection(child, child_rays, min_voxel_size, min_ray_threshold, detections);
                }
            }
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
std::vector<Voxel> detect_objects(Voxel target_zone, const std::vector<CameraFrame>& camera_frames, float min_voxel_size = 0.1f, size_t min_ray_threshold = 3){

    std::vector<Ray> all_rays;
    std::vector<Voxel> detections;

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
        //cv::imshow("Camera " + std::to_string(&frame - &camera_frames[0]) + " Diff", binary);

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
    recursive_detection(target_zone, all_rays, min_voxel_size, min_ray_threshold, detections);

    return detections;

}





