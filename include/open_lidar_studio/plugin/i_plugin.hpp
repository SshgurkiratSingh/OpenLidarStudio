#pragma once

#include <open_lidar_studio/plugin/plugin_context.hpp>

namespace ols::plugin {

class IPlugin {
public:
    virtual ~IPlugin() = default;

    // Called when the plugin is loaded
    virtual bool onInit(PluginContext* ctx) = 0;

    // Called every frame to process data/logic
    virtual void onUpdate(float dt) = 0;

    // Called every frame during ImGui rendering phase
    virtual void onRenderUI() = 0;

    // Called when the application is shutting down or plugin is unloaded
    virtual void onShutdown() = 0;
};

} // namespace ols::plugin

// Export macro for plugins to implement
#ifdef _WIN32
    #define OLS_PLUGIN_EXPORT extern "C" __declspec(dllexport)
#else
    #define OLS_PLUGIN_EXPORT extern "C" __attribute__((visibility("default")))
#endif

// Expected function signature in the dynamic library:
// OLS_PLUGIN_EXPORT ols::plugin::IPlugin* createPlugin();
