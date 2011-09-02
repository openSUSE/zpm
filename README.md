
# zpm

zpm is an experimental simple client API for accessing ZYpp package management.

## Goals

The goal is to keep simplicity while supporting the following requirements:

* Plain C API
* Easy to bind to other languages, either by hand or using SWIG
* Allow sysadmins to write custom scripts
* Powerful enough for client programs like zypper, PackageKit backend and
  YaST2 to use it as API instead of the ZYpp toolkit

## Example

```C
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
```

## License

zpm is licensed under the GNU General Public License version 2
or later. The text of the GNU General Public License can be viewed at
http://www.gnu.org/licenses/gpl.html

## Authors

* Duncan Mac-Vicar P. <dmacvicar@suse.de>
* Michael Andres <ma@suse.de>

Most of the code comes from solv.c example from libsolv
written by Michael Schroeder <mls@suse.de>

