cmake_minimum_required(VERSION 3.20)

project(Circuiti_s325732_s311455_s321750)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -Wshadow -Werror=vla -Wno-sign-compare")
set(CMAKE_BUILD_TYPE Release CACHE STRING "Choose the type of build, options are: None Debug Release RelWithDebInfo MinSizeRel" FORCE)

# Libreria Eigen necessaria
find_package(Eigen3 CONFIG REQUIRED)

# Elencare tutti i file sorgente inseriti nella cartella source
set(SOURCES
    source/parser/parser.cpp
    source/main.cpp
)

# Crea l'eseguibile
add_executable(Eseguibile ${SOURCES})

# Permette di non inserire tutto il percorso file nell'include
target_include_directories(Eseguibile PRIVATE 
    source
    source/parser
    source/unidirected_graph
)

# Collega Eigen3 all'eseguibile
target_link_libraries(Eseguibile PUBLIC Eigen3::Eigen)