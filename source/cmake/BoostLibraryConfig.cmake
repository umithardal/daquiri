
set(Boost_USE_STATIC_LIBS OFF)
set(Boost_USE_MULTITHREADED ON)
set(Boost_USE_STATIC_RUNTIME OFF)

find_package(Boost 1.41 COMPONENTS filesystem REQUIRED)

# deprecate
add_definitions(-DBOOST_LOG_DYN_LINK)
