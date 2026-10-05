#include <open_lidar_studio/plugin/plugin_manager.hpp>
#include <iostream>
#include <filesystem>

#ifdef _WIN32
#include <windows.h>
#else
#include <dlfcn.h>
#endif

namespace ols::plugin {

PluginManager::PluginManager() {}

PluginManager::~PluginManager() {
    unloadAll();
}

void PluginManager::loadPlugins(const std::string& directory_path, PluginContext* ctx) {
    if (!std::filesystem::exists(directory_path)) {
        std::cerr << "Plugin directory not found: " << directory_path << "\n";
        return;
    }

    for (const auto& entry : std::filesystem::directory_iterator(directory_path)) {
        if (!entry.is_regular_file()) continue;
        
        std::string path = entry.path().string();
#ifdef _WIN32
        if (entry.path().extension() != ".dll") continue;
        void* handle = LoadLibraryA(path.c_str());
        if (!handle) {
            std::cerr << "Failed to load plugin: " << path << "\n";
            continue;
        }
        
        using CreateFunc = IPlugin* (*)();
        CreateFunc create_func = (CreateFunc)GetProcAddress((HMODULE)handle, "createPlugin");
#else
        if (entry.path().extension() != ".so" && entry.path().extension() != ".dylib") continue;
        void* handle = dlopen(path.c_str(), RTLD_NOW | RTLD_LOCAL);
        if (!handle) {
            std::cerr << "Failed to load plugin: " << path << "\nError: " << dlerror() << "\n";
            continue;
        }

        using CreateFunc = IPlugin* (*)();
        // Clear any existing error
        dlerror();
        CreateFunc create_func = (CreateFunc)dlsym(handle, "createPlugin");
        const char* dlsym_error = dlerror();
        if (dlsym_error) {
            std::cerr << "Failed to find createPlugin symbol in: " << path << "\nError: " << dlsym_error << "\n";
            dlclose(handle);
            continue;
        }
#endif

        if (!create_func) {
            std::cerr << "Failed to find createPlugin function in: " << path << "\n";
#ifdef _WIN32
            FreeLibrary((HMODULE)handle);
#else
            dlclose(handle);
#endif
            continue;
        }

        IPlugin* plugin_instance = create_func();
        if (plugin_instance) {
            if (plugin_instance->onInit(ctx)) {
                LoadedPlugin lp;
                lp.name = entry.path().stem().string();
                lp.path = path;
                lp.handle = handle;
                lp.instance = plugin_instance;
                plugins_.push_back(lp);
                std::cout << "Successfully loaded plugin: " << lp.name << "\n";
            } else {
                std::cerr << "Plugin initialization failed for: " << path << "\n";
                plugin_instance->onShutdown();
                delete plugin_instance;
#ifdef _WIN32
                FreeLibrary((HMODULE)handle);
#else
                dlclose(handle);
#endif
            }
        }
    }
}

void PluginManager::unloadAll() {
    for (auto& plugin : plugins_) {
        if (plugin.instance) {
            plugin.instance->onShutdown();
            delete plugin.instance;
            plugin.instance = nullptr;
        }
        
        if (plugin.handle) {
#ifdef _WIN32
            FreeLibrary((HMODULE)plugin.handle);
#else
            dlclose(plugin.handle);
#endif
            plugin.handle = nullptr;
        }
    }
    plugins_.clear();
}

void PluginManager::updateAll(float dt) {
    for (auto& plugin : plugins_) {
        if (plugin.instance) {
            plugin.instance->onUpdate(dt);
        }
    }
}

void PluginManager::renderUIAll() {
    for (auto& plugin : plugins_) {
        if (plugin.instance) {
            plugin.instance->onRenderUI();
        }
    }
}

} // namespace ols::plugin
