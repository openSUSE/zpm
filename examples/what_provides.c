
#include "zpm.h"
#include "stdio.h"

int main()
{
    zpm *z = zpm_create("/");
    zpm_solvable_iter *it = zpm_query_what_provides("http_daemon");
    zpm_solvable *s;
    while (s = zpm_solvable_iter_next(it)) {
      printf("%s %s\n", zpm_solvable_name(s), zpm_solvable_version(s));
      zpm_solvable_destroy(s);
    }
    zpm_destroy(z);
    return 0;
}
