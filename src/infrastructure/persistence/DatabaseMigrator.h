#ifndef FIRELANDS_INFRASTRUCTURE_PERSISTENCE_DATABASE_MIGRATOR_H
#define FIRELANDS_INFRASTRUCTURE_PERSISTENCE_DATABASE_MIGRATOR_H

#include <conncpp.hpp>
#include <string>
#include <vector>
#include <fstream>
#include <shared/Logger.h>

namespace Firelands {

    /**
     * @brief Utility class to handle database schema validation and automatic creation.
     */
    class DatabaseMigrator {
    public:
        /**
         * @brief Validates and executes an SQL schema file.
         * 
         * @param host Database host
         * @param port Database port
         * @param user Database user
         * @param password Database password
         * @param sqlFilePath Path to the .sql file to execute
         */
        static void Migrate(const std::string& host, const std::string& port, 
                           const std::string& user, const std::string& password, 
                           const std::string& sqlFilePath) {
            try {
                sql::Driver* driver = sql::mariadb::get_driver_instance();
                
                // We connect without a specific database to ensure we can create it if it doesn't exist
                // The SQL script should contain "CREATE DATABASE IF NOT EXISTS" and "USE" statements.
                sql::SQLString url("jdbc:mariadb://" + host + ":" + port + "/");
                sql::Properties properties({{"user", user}, {"password", password}});

                std::shared_ptr<sql::Connection> conn(driver->connect(url, properties));
                
                std::ifstream file(sqlFilePath);
                if (!file.is_open()) {
                    LOG_ERROR("Could not open SQL schema file: {}", sqlFilePath);
                    return;
                }

                LOG_INFO("Validating database schema: {}", sqlFilePath);

                std::string content((std::istreambuf_iterator<char>(file)), std::istreambuf_iterator<char>());
                
                // Split the file into individual statements
                std::vector<std::string> statements = SplitStatements(content);

                for (const auto& statement : statements) {
                    ExecuteStatement(conn, statement);
                }

                LOG_INFO("Schema validation completed for: {}", sqlFilePath);

            } catch (sql::SQLException& e) {
                LOG_ERROR("Migration error in {}: {}", sqlFilePath, e.what());
            } catch (std::exception& e) {
                LOG_ERROR("General error during migration of {}: {}", sqlFilePath, e.what());
            }
        }

    private:
        /**
         * @brief Splits a string of SQL commands into individual statements.
         * Handles basic string literal escaping to avoid splitting inside strings.
         */
        static std::vector<std::string> SplitStatements(const std::string& sql) {
            std::vector<std::string> statements;
            std::string current;
            bool inString = false;
            char stringChar = 0;

            for (size_t i = 0; i < sql.length(); ++i) {
                char c = sql[i];
                
                // Basic string handling (single quote, double quote, backtick)
                if ((c == '\'' || c == '"' || c == '`') && (i == 0 || sql[i-1] != '\\')) {
                    if (!inString) {
                        inString = true;
                        stringChar = c;
                    } else if (stringChar == c) {
                        inString = false;
                    }
                }

                if (c == ';' && !inString) {
                    statements.push_back(current);
                    current.clear();
                } else {
                    current += c;
                }
            }
            if (!current.empty()) {
                statements.push_back(current);
            }
            return statements;
        }

        /**
         * @brief Executes a single SQL statement.
         */
        static void ExecuteStatement(std::shared_ptr<sql::Connection> conn, const std::string& statement) {
            std::string trimmed = statement;
            
            // Trim whitespace
            trimmed.erase(0, trimmed.find_first_not_of(" \n\r\t"));
            trimmed.erase(trimmed.find_last_not_of(" \n\r\t") + 1);

            // Skip empty statements or pure comments
            if (trimmed.empty() || trimmed.find("--") == 0 || trimmed.find("#") == 0 || trimmed.find("/*") == 0) {
                return;
            }

            try {
                std::unique_ptr<sql::Statement> stmnt(conn->createStatement());
                stmnt->execute(trimmed);
            } catch (sql::SQLException& e) {
                // Log but don't stop (useful if some parts of the script are expected to fail if already present)
                LOG_DEBUG("SQL Execution note ({}): {}", trimmed.substr(0, 50), e.what());
            }
        }
    };

}

#endif // FIRELANDS_INFRASTRUCTURE_PERSISTENCE_DATABASE_MIGRATOR_H
