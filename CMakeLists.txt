cmake_minimum_required(VERSION 3.16)
project(InterfaceAvaliacoes VERSION 0.1 LANGUAGES CXX)

set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# 🔹 AQUI: adiciona PrintSupport
find_package(QT NAMES Qt6 Qt5 REQUIRED COMPONENTS Widgets PrintSupport)
find_package(Qt${QT_VERSION_MAJOR} REQUIRED COMPONENTS Widgets PrintSupport)

set(UI_SOURCES
    main.cpp
    ui/telas/janelaprincipal.h   ui/telas/janelaprincipal.cpp   ui/telas/janelaprincipal.ui
    ui/telas/dialogologin.h      ui/telas/dialogologin.cpp      ui/telas/dialogologin.ui
    ui/telas/paginaprojetos.h    ui/telas/paginaprojetos.cpp    ui/telas/paginaprojetos.ui
    ui/telas/paginaavaliadores.h ui/telas/paginaavaliadores.cpp ui/telas/paginaavaliadores.ui
    ui/telas/paginanotas.h       ui/telas/paginanotas.cpp       ui/telas/paginanotas.ui
    ui/telas/paginafichas.h      ui/telas/paginafichas.cpp      ui/telas/paginafichas.ui  # ✅ inclui aqui também
)

if(${QT_VERSION_MAJOR} GREATER_EQUAL 6)
    qt_add_executable(InterfaceAvaliacoes
        MANUAL_FINALIZATION
        ${UI_SOURCES}
        resources.qrc
    )
else()
    add_executable(InterfaceAvaliacoes
        ${UI_SOURCES}
        resources.qrc
    )
endif()

# facilita includes curtos como "paginaprojetos.h"
target_include_directories(InterfaceAvaliacoes PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/ui/telas
)

# 🔹 AQUI: linka também o PrintSupport
target_link_libraries(InterfaceAvaliacoes PRIVATE
    Qt${QT_VERSION_MAJOR}::Widgets
    Qt${QT_VERSION_MAJOR}::PrintSupport
)

if(${QT_VERSION_MAJOR} GREATER_EQUAL 6)
    qt_finalize_executable(InterfaceAvaliacoes)
endif()
