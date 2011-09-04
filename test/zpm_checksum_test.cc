#include <stdio.h>
#include <stdlib.h>
#include <check.h>
#include "zpm_checksum.h"

START_TEST (test_verify_checksum)
{
    char templ[] = "/tmp/fileXXXXXX";
    Pool *pool = 0;
    int fd = -1;
    FILE *fp = 0;
    const unsigned char chksum[] = "0a4d55a8d778e5022fab701977c5d840bbc486d0";

    fd = mkstemp(templ);
    fail_unless(fd > 0, "Can't create tmp file");

    fp = fdopen(fd, "w");
    fail_unless(fp, "Can't open tmp file");

    fprintf(fp, "Hello World");
    fclose(fp);

    fail_unless(zpm_verify_checksum(fd, templ, chksum, REPOKEY_TYPE_SHA1), "checksum does not verify");
}
END_TEST

Suite *
verify_suite (void)
{
    Suite *s = suite_create ("Checksum");
    /* Core test case */
    TCase *tc_verif = tcase_create ("Verify");
    tcase_add_test (tc_verif, test_verify_checksum);
    suite_add_tcase (s, tc_verif);
    return s;
}

int
main (void)
{
    int number_failed;
    Suite *s = verify_suite ();
    SRunner *sr = srunner_create (s);
    srunner_run_all (sr, CK_NORMAL);
    number_failed = srunner_ntests_failed (sr);
    srunner_free (sr);
    return (number_failed == 0) ? EXIT_SUCCESS : EXIT_FAILURE;
}
