
set(zpm_SRCS
  zpm.cc
  zpm_solvable_iter.cc
  zpm_checksum.cc
  zpm_download.cc
  zpm_metalink.cc
  zpm_query_what_provides.cc
  zpm_query_search.cc
  zpm_repoinfo.cc
  zpm_repos.cc
)

add_library(zpm SHARED ${zpm_SRCS})
target_link_libraries(zpm solvext)
target_link_libraries(zpm solv)
target_link_libraries(zpm expat)
target_link_libraries(zpm z)
target_link_libraries(zpm rpm)
