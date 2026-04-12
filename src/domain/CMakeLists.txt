add_library(FirelandsDomain STATIC
    models/Realm.cpp
)

target_include_directories(FirelandsDomain PUBLIC 
    ${CMAKE_CURRENT_SOURCE_DIR}/..
)

target_link_libraries(FirelandsDomain PUBLIC 
    FirelandsShared
)
