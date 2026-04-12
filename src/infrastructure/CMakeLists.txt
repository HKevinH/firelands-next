add_library(FirelandsInfrastructure STATIC
    persistence/MySqlAccountRepository.cpp
    persistence/MySqlRealmRepository.cpp
    network/sessions/AuthSession.cpp
    network/asio/AsyncNetworkServer.cpp
)

target_link_libraries(FirelandsInfrastructure 
    PRIVATE FirelandsDomain FirelandsApplication
    PUBLIC mariadbcpp-static mariadbclient ${OPENSSL_LIBRARIES} ${MARIADB_EXTRA_LIBS}
    PUBLIC Boost::thread
)

target_include_directories(FirelandsInfrastructure PUBLIC 
    ${CMAKE_CURRENT_SOURCE_DIR}
)

target_include_directories(FirelandsInfrastructure SYSTEM PUBLIC
    ${mariadb-connector-cpp_SOURCE_DIR}/include
    ${mariadb-connector-c_SOURCE_DIR}/include
    ${mariadb-connector-c_BINARY_DIR}/include
)
