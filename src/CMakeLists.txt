
set(zpm_SRCS zpm.cc zpm_query.cc)
add_library(zpm SHARED ${zpm_SRCS})
target_link_libraries(zpm ${ZYPP_LIBRARY})