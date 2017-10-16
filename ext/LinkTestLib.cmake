function(LinkTestLib TargetName)

    add_dependencies(${TargetName} googletest)
    if(NOT WIN32 OR MINGW)
        FOREACH(LibName ${GTestLinkLibNames})
            target_link_libraries(${TargetName} ${GTestLibsDir}/lib${LibName}.a )
        ENDFOREACH()
    else()
        FOREACH(LibName ${GTestLinkLibNames})
            target_link_libraries(${TargetName}
                    debug ${GTestLibsDir}/DebugLibs/${CMAKE_FIND_LIBRARY_PREFIXES}${LibName}d${CMAKE_FIND_LIBRARY_SUFFIXES}
                    optimized ${GTestLibsDir}/ReleaseLibs/${CMAKE_FIND_LIBRARY_PREFIXES}${LibName}${CMAKE_FIND_LIBRARY_SUFFIXES})
        ENDFOREACH()
    endif()

    target_link_libraries(${TargetName} ${CMAKE_THREAD_LIBS_INIT})
endfunction(LinkTestLib)