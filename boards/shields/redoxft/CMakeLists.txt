get_property(_redoxft_done GLOBAL PROPERTY REDOXFT_DONGLE_CMAKE_DONE)
if(_redoxft_done)
    return()
endif()
set_property(GLOBAL PROPERTY REDOXFT_DONGLE_CMAKE_DONE TRUE)

if(CONFIG_SHIELD_REDOXFT_DONGLE)
    target_include_directories(app PRIVATE
        ${CMAKE_CURRENT_LIST_DIR}/include
    )
    target_sources(app PRIVATE
        ${CMAKE_CURRENT_LIST_DIR}/src/split_central_status_changed.c
        ${CMAKE_CURRENT_LIST_DIR}/src/central_status_changed_observer.c
    )

    if(CONFIG_ZMK_DISPLAY)
        target_sources(app PRIVATE
            ${CMAKE_CURRENT_LIST_DIR}/src/status_screen.c
            ${CMAKE_CURRENT_LIST_DIR}/src/zmk_studio_logo.c
            ${CMAKE_CURRENT_LIST_DIR}/src/falbatech_logo_large.c
            ${CMAKE_CURRENT_LIST_DIR}/src/falbatech_logo_small.c
        )
    endif()
endif()
