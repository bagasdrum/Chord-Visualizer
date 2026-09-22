set(CHORDSCOPE_SOURCES
    ChordVisualizerFactory.cpp
    ChordVisualizerProcessor.cpp
    ChordVisualizerController.cpp
    ChordVisualizerView.cpp
    ChordVisualizerShared.cpp
    ChordVisualizerProcessor.h
    ChordVisualizerController.h
    ChordVisualizerView.h
    ChordVisualizerShared.h
    ChordVisualizerIDs.h
)

smtg_add_vst3plugin(ChordScope ${CHORDSCOPE_SOURCES})

target_compile_features(ChordScope PRIVATE cxx_std_17)

target_include_directories(ChordScope PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})

target_link_libraries(ChordScope PRIVATE sdk)

if(WIN32)
    target_link_libraries(ChordScope PRIVATE gdi32 user32)
endif()

# Make the generated VST3 use the product name users see in their DAW.
set_target_properties(ChordScope PROPERTIES
    OUTPUT_NAME "ChordScope"
)

if(MSVC)
    target_compile_options(ChordScope PRIVATE /W4 /permissive-)
endif()
