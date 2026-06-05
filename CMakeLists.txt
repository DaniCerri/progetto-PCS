cmake_minimum_required(VERSION 3.20)

project(Circuiti_s325732_s311455_s321750)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -Wshadow -Werror=vla")
set(CMAKE_BUILD_TYPE Release CACHE STRING "Choose the type of build, options are: None Debug Release RelWithDebInfo MinSizeRel" FORCE)

# Libreria Eigen necessaria
find_package(Eigen3 CONFIG REQUIRED)

# Elencare tutti i file sorgente inseriti nella cartella source
set(SOURCES
    source/parser/parser.cpp
    source/algos/solver/gradiente_coniugato.cpp
    source/main.cpp
)

# Crea l'eseguibile
add_executable(Eseguibile ${SOURCES})

# Permette di non inserire tutto il percorso file nell'include
target_include_directories(Eseguibile PRIVATE 
    source
    source/parser
    source/unidirected_graph
    source/algos
    source/algos/cycles
    source/algos/visit
    source/algos/solver

)

# Collega Eigen3 all'eseguibile
target_link_libraries(Eseguibile PUBLIC Eigen3::Eigen)

# ----------------------------------------------------------------------------
# Test (CTest). Disattivabili con -DBUILD_TESTS=OFF per la sola consegna.
# Un eseguibile per area (parser, graph, cycles, solver); 'make test' li esegue
# tutti (richiede 'make' prima, per (ri)compilarli). Output dettagliato:
#   ctest --output-on-failure   (oppure ./test_<area> direttamente)
# ----------------------------------------------------------------------------
option(BUILD_TESTS "Compila la suite di test" ON)
if(BUILD_TESTS)
    enable_testing()

    # sorgenti non-header condivisi dai test (il resto e' header-only)
    set(TEST_COMMON_SOURCES
        source/parser/parser.cpp
        source/algos/solver/gradiente_coniugato.cpp
    )

    # una suite per area funzionale -> un test CTest separato
    foreach(area parser graph cycles solver)
        add_executable(test_${area} test/test_${area}.cpp ${TEST_COMMON_SOURCES})
        target_include_directories(test_${area} PRIVATE
            source
            source/parser
            source/unidirected_graph
            source/algos
            source/algos/cycles
            source/algos/visit
            source/algos/solver
        )
        target_link_libraries(test_${area} PUBLIC Eigen3::Eigen)
        # percorso assoluto delle netlist di test, indipendente dalla cwd
        target_compile_definitions(test_${area} PRIVATE
            TEST_DIR="${CMAKE_CURRENT_SOURCE_DIR}/test/netlists")
        add_test(NAME ${area} COMMAND test_${area})
    endforeach()
endif()