#include <open_lidar_studio/plugin/i_plugin.hpp>
#include <iostream>

using namespace ols::plugin;

class DummyPlugin : public IPlugin {
public:
    bool onInit(PluginContext* ctx) override {
        std::cout << "DummyPlugin: onInit called!\n";
        return true;
    }
    void onUpdate(float dt) override {}
    void onRenderUI() override {}
    void onShutdown() override {
        std::cout << "DummyPlugin: onShutdown called!\n";
    }
};

OLS_PLUGIN_EXPORT IPlugin* createPlugin() {
    return new DummyPlugin();
}
