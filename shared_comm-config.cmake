get_filename_component(SELF_DIR "${CMAKE_CURRENT_LIST_FILE}" PATH)
include(${SELF_DIR}/shared_com-targets.cmake)
get_filename_component(shared_com_INCLUDE_DIRS "${SELF_DIR}/../../include/shared_com" ABSOLUTE)
set(shared_com_LIBRARIES shared_com)
