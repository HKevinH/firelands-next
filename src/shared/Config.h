#ifndef FIRELANDS_SHARED_CONFIG_H
#define FIRELANDS_SHARED_CONFIG_H

#include <string>
#include <yaml-cpp/yaml.h>
#include <optional>
#include <shared/Logger.h>

namespace Firelands {

    class Config {
    public:
        static Config& Instance() {
            static Config instance;
            return instance;
        }

        bool Load(const std::string& filename) {
            try {
                _config = YAML::LoadFile(filename);
                _filename = filename;
                LOG_INFO("Config loaded: {}", filename);
                return true;
            } catch (const std::exception& e) {
                LOG_ERROR("Failed to load config {}: {}", filename, e.what());
                return false;
            }
        }

        template<typename T>
        T Get(const std::string& key, T defaultValue) const {
            try {
                if (_config[key]) {
                    return _config[key].as<T>();
                }
            } catch (...) {}
            return defaultValue;
        }

        template<typename T>
        std::optional<T> Get(const std::string& key) const {
            try {
                if (_config[key]) {
                    return _config[key].as<T>();
                }
            } catch (...) {}
            return std::nullopt;
        }

        // Support for nested keys like "Database.Auth.Host"
        template<typename T>
        T GetNested(const std::vector<std::string>& keys, T defaultValue) const {
            YAML::Node node = _config;
            for (const auto& key : keys) {
                if (!node[key]) return defaultValue;
                node = node[key];
            }
            try {
                return node.as<T>();
            } catch (...) {
                return defaultValue;
            }
        }

    private:
        Config() = default;
        YAML::Node _config;
        std::string _filename;
    };

} // namespace Firelands

#endif // FIRELANDS_SHARED_CONFIG_H
