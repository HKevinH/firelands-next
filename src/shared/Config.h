#ifndef FIRELANDS_SHARED_CONFIG_H
#define FIRELANDS_SHARED_CONFIG_H

#include <string>
#include <yaml-cpp/yaml.h>
#include <optional>
#include <shared/Logger.h>
#include <algorithm>
#include <cctype>
#include <vector>

namespace YAML {
    template<>
    struct convert<Firelands::LogLevel> {
        static Node encode(const Firelands::LogLevel& rhs) {
            Node node;
            switch (rhs) {
                case Firelands::LogLevel::Trace: node = "trace"; break;
                case Firelands::LogLevel::Debug: node = "debug"; break;
                case Firelands::LogLevel::Info: node = "info"; break;
                case Firelands::LogLevel::Warn: node = "warn"; break;
                case Firelands::LogLevel::Error: node = "error"; break;
                case Firelands::LogLevel::Critical: node = "critical"; break;
                case Firelands::LogLevel::Off: node = "off"; break;
                default: node = "info"; break;
            }
            return node;
        }

        static bool decode(const Node& node, Firelands::LogLevel& rhs) {
            if (!node.IsScalar()) {
                return false;
            }

            std::string val = node.as<std::string>();
            for (auto& c : val) {
                c = static_cast<char>(std::tolower(static_cast<unsigned char>(c)));
            }

            if (val == "trace") rhs = Firelands::LogLevel::Trace;
            else if (val == "debug") rhs = Firelands::LogLevel::Debug;
            else if (val == "info") rhs = Firelands::LogLevel::Info;
            else if (val == "warn" || val == "warning") rhs = Firelands::LogLevel::Warn;
            else if (val == "error") rhs = Firelands::LogLevel::Error;
            else if (val == "critical") rhs = Firelands::LogLevel::Critical;
            else if (val == "off") rhs = Firelands::LogLevel::Off;
            else return false;

            return true;
        }
    };
}

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
                if (Logger::IsInitialized()) {
                    LOG_INFO("Config loaded: {}", filename);
                }
                return true;
            } catch (const std::exception& e) {
                if (Logger::IsInitialized()) {
                    LOG_ERROR("Failed to load config {}: {}", filename, e.what());
                }
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
            try {
                YAML::Node current = _config;
                for (const auto& key : keys) {
                    if (!current[key]) return defaultValue;
                    current = current[key];
                }
                return current.as<T>();
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
