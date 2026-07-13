cmake_minimum_required(VERSION 3.19)
project(running-maodie LANGUAGES CXX)

find_package(Qt6 6.5 REQUIRED COMPONENTS Core Widgets)

qt_standard_project_setup()
set(CMAKE_AUTORCC ON)

qt_add_executable(running-maodie
    WIN32 MACOSX_BUNDLE
    src/main.cpp
    src/app/mainwindow.cpp
    src/app/mainwindow.h
    src/pages/homepage.cpp
    src/pages/homepage.h
    src/pages/gamepage.cpp
    src/pages/gamepage.h
    src/game/gamecontroller.cpp
    src/game/gamecontroller.h
    src/game/gamecanvas.cpp
    src/game/gamecanvas.h
    src/animation/playeranimator.cpp
    src/animation/playeranimator.h
    src/entities/player.cpp
    src/entities/player.h
    src/entities/obstacle.cpp
    src/entities/obstacle.h
    src/entities/collectible.cpp
    src/entities/collectible.h
    src/systems/collisionsystem.cpp
    src/systems/collisionsystem.h
    src/systems/spawnsystem.cpp
    src/systems/spawnsystem.h
    src/systems/scoresystem.cpp
    src/systems/scoresystem.h
    src/managers/resourcemanager.cpp
    src/managers/resourcemanager.h
    src/managers/audiomanager.cpp
    src/managers/audiomanager.h
    src/ui/hudwidget.cpp
    src/ui/hudwidget.h
    src/config/gameconfig.h
    resources/resources.qrc
)

file(GLOB PLAYER_FRAME_FILES CONFIGURE_DEPENDS
    "${CMAKE_CURRENT_SOURCE_DIR}/resources/images/player/*.png"
)
file(GLOB_RECURSE OBSTACLE_IMAGE_FILES CONFIGURE_DEPENDS
    "${CMAKE_CURRENT_SOURCE_DIR}/resources/images/obstacles/*.png"
)
file(GLOB HAQI_FRAME_FILES CONFIGURE_DEPENDS
    "${CMAKE_CURRENT_SOURCE_DIR}/resources/images/haqi_white/*.png"
)
file(GLOB_RECURSE EXPLODE_FRAME_FILES CONFIGURE_DEPENDS
    "${CMAKE_CURRENT_SOURCE_DIR}/resources/images/explodes/*.png"
)
file(GLOB COIN_FRAME_FILES CONFIGURE_DEPENDS
    "${CMAKE_CURRENT_SOURCE_DIR}/resources/images/items/coins/*.png"
)
file(GLOB BIGHAQI_FRAME_FILES CONFIGURE_DEPENDS
    "${CMAKE_CURRENT_SOURCE_DIR}/resources/images/bighaqi_frames/*.png"
)
file(GLOB TUXI_FRAME_FILES CONFIGURE_DEPENDS
    "${CMAKE_CURRENT_SOURCE_DIR}/resources/images/tuxi_frames/*.png"
)
qt_add_resources(running-maodie "player_frames"
    PREFIX "/"
    BASE "${CMAKE_CURRENT_SOURCE_DIR}/resources"
    FILES ${PLAYER_FRAME_FILES}
)
qt_add_resources(running-maodie "obstacle_images"
    PREFIX "/"
    BASE "${CMAKE_CURRENT_SOURCE_DIR}/resources"
    FILES ${OBSTACLE_IMAGE_FILES}
)
qt_add_resources(running-maodie "haqi_frames"
    PREFIX "/"
    BASE "${CMAKE_CURRENT_SOURCE_DIR}/resources"
    FILES ${HAQI_FRAME_FILES}
)
qt_add_resources(running-maodie "explode_frames"
    PREFIX "/"
    BASE "${CMAKE_CURRENT_SOURCE_DIR}/resources"
    FILES ${EXPLODE_FRAME_FILES}
)
qt_add_resources(running-maodie "coin_frames"
    PREFIX "/"
    BASE "${CMAKE_CURRENT_SOURCE_DIR}/resources"
    FILES ${COIN_FRAME_FILES}
)
qt_add_resources(running-maodie "bighaqi_frames"
    PREFIX "/"
    BASE "${CMAKE_CURRENT_SOURCE_DIR}/resources"
    FILES ${BIGHAQI_FRAME_FILES}
)
qt_add_resources(running-maodie "tuxi_frames"
    PREFIX "/"
    BASE "${CMAKE_CURRENT_SOURCE_DIR}/resources"
    FILES ${TUXI_FRAME_FILES}
)

target_include_directories(running-maodie PRIVATE src)

target_link_libraries(running-maodie
    PRIVATE
        Qt::Core
        Qt::Widgets
)

install(TARGETS running-maodie
    BUNDLE  DESTINATION .
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
)

qt_generate_deploy_app_script(
    TARGET running-maodie
    OUTPUT_SCRIPT deploy_script
    NO_UNSUPPORTED_PLATFORM_ERROR
)
install(SCRIPT ${deploy_script})
