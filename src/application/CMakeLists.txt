add_library(FirelandsApplication STATIC
    services/RealmListService.cpp
)

target_include_directories(FirelandsApplication PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/..
)

target_link_libraries(FirelandsApplication PUBLIC
    FirelandsDomain
    FirelandsShared
)
