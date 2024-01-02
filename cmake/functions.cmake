function(os_files_filter files_list_name os_specific_path)
    set(files_list ${${files_list_name}})
    list(FILTER files_list EXCLUDE REGEX "${os_specific_path}/.*/.*")

    set(OS_FILES)
    if (WIN32)
        file(GLOB_RECURSE OS_FILES
            "${os_specific_path}/win32/*"
        )
    else()
        message(FATAL_ERROR "Not implemented")
    endif()

    list(LENGTH OS_FILES OS_FILES_LENGTH)
    if (OS_FILES_LENGTH EQUAL 0)
        message(FATAL_ERROR "Files specific for your system could not be found")
    endif()

    list(APPEND files_list ${OS_FILES})
    set(${files_list_name} ${files_list} PARENT_SCOPE)
endfunction()