
set(zpm_SRCS zpm.c)

add_library(zpm SHARED ${zpm_SRCS})
target_link_libraries(zpm ${ZYPP_LIBRARY})