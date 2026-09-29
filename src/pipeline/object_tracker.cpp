#include <open_lidar_studio/pipeline/object_tracker.hpp>
#include <open_lidar_studio/core/math_utils.hpp>
#include <cmath>
#include <limits>
#include <algorithm>
#include <iostream>

#ifdef _WIN32
#undef min
#undef max
#endif

namespace ols::pipeline {

ObjectTracker::ObjectTracker() {}

void ObjectTracker::setClusterDistance(float dist) { cluster_dist_ = dist; }
void ObjectTracker::setPersonSizeBounds(float min_size, float max_size) {
    min_size_ = min_size;
    max_size_ = max_size;
}
void ObjectTracker::setMaxMissingFrames(int frames) { max_missing_frames_ = frames; }
void ObjectTracker::setMinHits(int hits) { min_hits_ = hits; }

void ObjectTracker::process(const std::vector<LaserPoint>& points, float dt, std::vector<ui::TrackedObject>& out_tracks) {
    // 1. Convert to cartesian for clustering
    struct CartPoint { float x, y; bool visited; int cluster_id; };
    std::vector<CartPoint> cart_points;
    cart_points.reserve(points.size());
    for (const auto& p : points) {
        if (p.range > 0) {
            float x, y;
            core::MathUtils::polarToCartesian(p.range, p.angle, x, y);
            cart_points.push_back({x, y, false, -1});
        }
    }

    // 2. Euclidean Clustering (simple O(N^2) for small N, fine for 2D LiDAR rings)
    int current_cluster = 0;
    std::vector<std::vector<CartPoint*>> clusters;

    for (size_t i = 0; i < cart_points.size(); ++i) {
        if (cart_points[i].visited) continue;
        
        std::vector<CartPoint*> cluster;
        std::vector<CartPoint*> queue;
        
        cart_points[i].visited = true;
        cart_points[i].cluster_id = current_cluster;
        queue.push_back(&cart_points[i]);
        
        while (!queue.empty()) {
            CartPoint* cp = queue.back();
            queue.pop_back();
            cluster.push_back(cp);
            
            for (size_t j = 0; j < cart_points.size(); ++j) {
                if (!cart_points[j].visited) {
                    float dx = cart_points[j].x - cp->x;
                    float dy = cart_points[j].y - cp->y;
                    if (dx*dx + dy*dy <= cluster_dist_*cluster_dist_) {
                        cart_points[j].visited = true;
                        cart_points[j].cluster_id = current_cluster;
                        queue.push_back(&cart_points[j]);
                    }
                }
            }
        }
        clusters.push_back(cluster);
        current_cluster++;
    }

    // 3. Filter clusters by size and compute centroids
    std::vector<ui::TrackedObject> current_frame_objects;
    for (const auto& cluster : clusters) {
        if (cluster.empty()) continue;
        
        float min_x = std::numeric_limits<float>::max();
        float max_x = std::numeric_limits<float>::lowest();
        float min_y = std::numeric_limits<float>::max();
        float max_y = std::numeric_limits<float>::lowest();
        
        float sum_x = 0, sum_y = 0;
        for (auto* cp : cluster) {
            min_x = std::min(min_x, cp->x);
            max_x = std::max(max_x, cp->x);
            min_y = std::min(min_y, cp->y);
            max_y = std::max(max_y, cp->y);
            sum_x += cp->x;
            sum_y += cp->y;
        }
        
        float width = max_x - min_x;
        float height = max_y - min_y;
        float size = std::max(width, height);
        
        if (size >= min_size_ && size <= max_size_) {
            ui::TrackedObject obj;
            obj.id = -1; // Unassigned
            obj.center_x = sum_x / cluster.size();
            obj.center_y = sum_y / cluster.size();
            obj.radius = size / 2.0f;
            obj.velocity_x = 0;
            obj.velocity_y = 0;
            obj.missing_frames = 0;
            obj.hit_count = 0;
            obj.is_confirmed = false;
            current_frame_objects.push_back(obj);
        }
    }

    // 4. Data Association (Nearest Neighbor)
    std::vector<ui::TrackedObject> next_tracks;
    
    // For each existing track, find best matching observation
    for (auto& track : current_tracks_) {
        int best_match = -1;
        float min_dist_sq = std::numeric_limits<float>::max();
        
        // Predict position based on velocity (simple linear)
        float pred_x = track.center_x + track.velocity_x * dt;
        float pred_y = track.center_y + track.velocity_y * dt;

        for (size_t j = 0; j < current_frame_objects.size(); ++j) {
            if (current_frame_objects[j].id != -1) continue; // Already assigned
            
            float dx = current_frame_objects[j].center_x - pred_x;
            float dy = current_frame_objects[j].center_y - pred_y;
            float dist_sq = dx*dx + dy*dy;
            
            // Allow matching within 1.0 meter (could be tuned)
            if (dist_sq < min_dist_sq && dist_sq < 1.0f) {
                min_dist_sq = dist_sq;
                best_match = j;
            }
        }
        
        if (best_match != -1) {
            // Found a match
            auto& obs = current_frame_objects[best_match];
            obs.id = track.id; // Assign existing ID
            obs.hit_count = track.hit_count + 1;
            if (obs.hit_count >= min_hits_) {
                obs.is_confirmed = true;
            } else {
                obs.is_confirmed = false;
            }
            
            // Update velocity
            if (dt > 0.0f) {
                obs.velocity_x = (obs.center_x - track.center_x) / dt;
                obs.velocity_y = (obs.center_y - track.center_y) / dt;
            } else {
                obs.velocity_x = track.velocity_x;
                obs.velocity_y = track.velocity_y;
            }
            
            next_tracks.push_back(obs);
        } else {
            // Track is missing
            track.missing_frames++;
            if (track.missing_frames <= max_missing_frames_) {
                // Keep the track, use predicted position
                track.center_x = pred_x;
                track.center_y = pred_y;
                next_tracks.push_back(track);
            }
        }
    }
    
    // Add new tracks for unmatched observations
    for (const auto& obs : current_frame_objects) {
        if (obs.id == -1) {
            auto new_track = obs;
            new_track.id = next_id_++;
            new_track.hit_count = 1;
            new_track.is_confirmed = (min_hits_ <= 1);
            next_tracks.push_back(new_track);
        }
    }

    current_tracks_ = next_tracks;
    out_tracks.clear();
    for (const auto& t : current_tracks_) {
        if (t.is_confirmed) {
            out_tracks.push_back(t);
        }
    }
}

} // namespace ols::pipeline
