cmake_minimum_required(VERSION 3.20)
project(Circuiti_s325732_s311455_s321750)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -Wshadow -Werror=vla")

# Aggiungiamo la libreria Eigen
find_package(Eigen3 CONFIG REQUIRED) # path to Eigen headers
list(APPEND libraries_list PUBLIC Eigen3::Eigen) 


add_executable(${PROJECT_NAME} source/main.cpp)