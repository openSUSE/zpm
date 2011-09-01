
#ifndef __ZPM_H
#define __ZPM_H

typedef struct _zpm zpm;
typedef struct _zpm_solvable zpm_solvable;
typedef struct _zpm_solvable_iter zpm_solvable_iter;

#ifdef __cplusplus
extern "C" {
#endif

zpm* zpm_create(const char *root);

zpm_solvable_iter *zpm_query_what_provides(const char *what);

zpm_solvable * zpm_solvable_iter_next(zpm_solvable_iter *);

const char * zpm_solvable_name(zpm_solvable *s);
const char * zpm_solvable_version(zpm_solvable *s);

void zpm_solvable_destroy(zpm_solvable *s);

void zpm_destroy(zpm *z);

#ifdef __cplusplus
}
#endif

#endif
