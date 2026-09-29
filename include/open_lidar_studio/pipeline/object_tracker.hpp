#pragma once

#include <vector>
#include <CYdLidar.h>
#include <open_lidar_studio/ui/ui_state.hpp>

namespace ols::pipeline {

class ObjectTracker {
public:
    ObjectTracker();
    
    // Config
    void setClusterDistance(float dist);
    void setPersonSizeBounds(float min_size, float max_size);
    void setMaxMissingFrames(int frames);
    void setMinHits(int hits);

    // Process a frame of points and update tracked objects
    void process(const std::vector<LaserPoint>& points, float dt, std::vector<ui::TrackedObject>& out_tracks);

private:
    float cluster_dist_{0.2f};
    float min_size_{0.2f};
    float max_size_{0.8f};
    int max_missing_frames_{10};
    int min_hits_{3};
    int next_id_{1};

    std::vector<ui::TrackedObject> current_tracks_;
};

} // namespace ols::pipeline
