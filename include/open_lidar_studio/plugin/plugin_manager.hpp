#pragma once

#include <string>
#include <vector>
#include <memory>
#include <open_lidar_studio/plugin/i_plugin.hpp>
#include <open_lidar_studio/plugin/plugin_context.hpp>

namespace ols::plugin {

struct LoadedPlugin {
    std::string name;
    std::string path;
    void* handle{nullptr};
    IPlugin* instance{nullptr};
};

class PluginManager {
public:
    PluginManager();
    ~PluginManager();

    // Loads all plugins from the specified directory
    void loadPlugins(const std::string& directory_path, PluginContext* ctx);
    
    // Unloads all loaded plugins
    void unloadAll();

    // Calls onUpdate on all active plugins
    void updateAll(float dt);

    // Calls onRenderUI on all active plugins
    void renderUIAll();

    const std::vector<LoadedPlugin>& getLoadedPlugins() const { return plugins_; }

private:
    std::vector<LoadedPlugin> plugins_;
};

} // namespace ols::plugin
