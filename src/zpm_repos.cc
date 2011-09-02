
#include <stdlib.h>
#include <unistd.h>
#include <sys/stat.h>
#include <sys/time.h>
#include <time.h>

extern "C" {
#include <solv/pool.h>
#include <solv/repo.h>
#include <solv/repo_content.h>
#include <solv/repo_deltainfoxml.h>
#include <solv/repo_repomdxml.h>
#include <solv/repo_rpmdb.h>
#include <solv/repo_rpmmd.h>
#include <solv/repo_solv.h>
#include <solv/repo_susetags.h>
#include <solv/repo_updateinfoxml.h>
#include <solv/repo_write.h>
#include <solv/chksum.h>
}

#include "zpm_download.h"
#include "zpm_repos.h"
#include "zpm_repoinfo.h"
#include "zpm_checksum.h"

#define SOLVCACHE_PATH "/var/cache/solv"

static unsigned char installedcookie[32];

static inline int
iscompressed(const char *name)
{
  int l = strlen(name);
  return l > 3 && !strcmp(name + l - 3, ".gz") ? 1 : 0;
}

static Pool *
read_sigs()
{
  Pool *sigpool = pool_create();
#ifndef DEBIAN
  Repo *repo = repo_create(sigpool, "rpmdbkeys");
  repo_add_rpmdb_pubkeys(repo, 0, 0);
#endif
  return sigpool;
}

/* repomd helpers */

static inline const char *
repomd_find(Repo *repo, const char *what, const unsigned char **chksump, Id *chksumtypep)
{
  Pool *pool = repo->pool;
  Dataiterator di;
  const char *filename;

  filename = 0;
  *chksump = 0;
  *chksumtypep = 0;
  dataiterator_init(&di, pool, repo, SOLVID_META, REPOSITORY_REPOMD_TYPE, what, SEARCH_STRING);
  dataiterator_prepend_keyname(&di, REPOSITORY_REPOMD);
  if (dataiterator_step(&di))
    {
      dataiterator_setpos_parent(&di);
      filename = pool_lookup_str(pool, SOLVID_POS, REPOSITORY_REPOMD_LOCATION);
      *chksump = pool_lookup_bin_checksum(pool, SOLVID_POS, REPOSITORY_REPOMD_CHECKSUM, chksumtypep);
    }
  dataiterator_free(&di);
  if (filename && !*chksumtypep)
    {
      printf("no %s file checksum!\n", what);
      filename = 0;
    }
  return filename;
}

int
repomd_add_ext(Repo *repo, Repodata *data, const char *what)
{
  Id chksumtype, handle;
  const unsigned char *chksum;
  const char *filename;

  filename = repomd_find(repo, what, &chksum, &chksumtype);
  if (!filename)
    return 0;
  if (!strcmp(what, "prestodelta"))
    what = "deltainfo";
  handle = repodata_new_handle(data);
  repodata_set_poolstr(data, handle, REPOSITORY_REPOMD_TYPE, what);
  repodata_set_str(data, handle, REPOSITORY_REPOMD_LOCATION, filename);
  repodata_set_bin_checksum(data, handle, REPOSITORY_REPOMD_CHECKSUM, chksumtype, chksum);
  if (!strcmp(what, "deltainfo"))
    {
      repodata_add_idarray(data, handle, REPOSITORY_KEYS, REPOSITORY_DELTAINFO);
      repodata_add_idarray(data, handle, REPOSITORY_KEYS, REPOKEY_TYPE_FLEXARRAY);
    }
  if (!strcmp(what, "filelists"))
    {
      repodata_add_idarray(data, handle, REPOSITORY_KEYS, SOLVABLE_FILELIST);
      repodata_add_idarray(data, handle, REPOSITORY_KEYS, REPOKEY_TYPE_DIRSTRARRAY);
    }
  repodata_add_flexarray(data, SOLVID_META, REPOSITORY_EXTERNAL, handle);
  return 1;
}

int
repomd_load_ext(Repo *repo, Repodata *data)
{
  const char *filename, *repomdtype;
  char ext[3];
  FILE *fp;
  zpm_repoinfo *cinfo;
  const unsigned char *filechksum;
  Id filechksumtype;

  cinfo = (zpm_repoinfo *) repo->appdata;
  repomdtype = repodata_lookup_str(data, SOLVID_META, REPOSITORY_REPOMD_TYPE);
  if (!repomdtype)
    return 0;
  if (!strcmp(repomdtype, "filelists"))
    strcpy(ext, "FL");
  else if (!strcmp(repomdtype, "deltainfo"))
    strcpy(ext, "DL");
  else
    return 0;
#if 1
  printf("[%s:%s", repo->name, ext);
#endif
  if (usecachedrepo(repo, ext, cinfo->extcookie, 0))
    {
      printf(" cached]\n");fflush(stdout);
      return 1;
    }
  printf(" fetching]\n"); fflush(stdout);
  filename = repodata_lookup_str(data, SOLVID_META, REPOSITORY_REPOMD_LOCATION);
  filechksumtype = 0;
  filechksum = repodata_lookup_bin_checksum(data, SOLVID_META, REPOSITORY_REPOMD_CHECKSUM, &filechksumtype);
  if ((fp = curlfopen(cinfo, filename, iscompressed(filename), filechksum, filechksumtype, 0)) == 0)
    return 0;
  if (!strcmp(ext, "FL"))
    repo_add_rpmmd(repo, fp, ext, REPO_USE_LOADING|REPO_EXTEND_SOLVABLES);
  else if (!strcmp(ext, "DL"))
    repo_add_deltainfoxml(repo, fp, REPO_USE_LOADING);
  fclose(fp);
  writecachedrepo(repo, data, ext, cinfo->extcookie);
  return 1;
}


/* susetags helpers */

static inline const char *
susetags_find(Repo *repo, const char *what, const unsigned char **chksump, Id *chksumtypep)
{
  Pool *pool = repo->pool;
  Dataiterator di;
  const char *filename;

  filename = 0;
  *chksump = 0;
  *chksumtypep = 0;
  dataiterator_init(&di, pool, repo, SOLVID_META, SUSETAGS_FILE_NAME, what, SEARCH_STRING);
  dataiterator_prepend_keyname(&di, SUSETAGS_FILE);
  if (dataiterator_step(&di))
    {
      dataiterator_setpos_parent(&di);
      *chksump = pool_lookup_bin_checksum(pool, SOLVID_POS, SUSETAGS_FILE_CHECKSUM, chksumtypep);
      filename = what;
    }
  dataiterator_free(&di);
  if (filename && !*chksumtypep)
    {
      printf("no %s file checksum!\n", what);
      filename = 0;
    }
  return filename;
}

static Id susetags_langtags[] = {
  SOLVABLE_SUMMARY, REPOKEY_TYPE_STR,
  SOLVABLE_DESCRIPTION, REPOKEY_TYPE_STR,
  SOLVABLE_EULA, REPOKEY_TYPE_STR,
  SOLVABLE_MESSAGEINS, REPOKEY_TYPE_STR,
  SOLVABLE_MESSAGEDEL, REPOKEY_TYPE_STR,
  SOLVABLE_CATEGORY, REPOKEY_TYPE_ID,
  0, 0
};

void
susetags_add_ext(Repo *repo, Repodata *data)
{
  Pool *pool = repo->pool;
  Dataiterator di;
  char ext[3];
  Id handle, filechksumtype;
  const unsigned char *filechksum;
  int i;

  dataiterator_init(&di, pool, repo, SOLVID_META, SUSETAGS_FILE_NAME, 0, 0);
  dataiterator_prepend_keyname(&di, SUSETAGS_FILE);
  while (dataiterator_step(&di))
    {
      if (strncmp(di.kv.str, "packages.", 9) != 0)
	continue;
      if (!strcmp(di.kv.str + 9, "gz"))
	continue;
      if (!di.kv.str[9] || !di.kv.str[10] || (di.kv.str[11] && di.kv.str[11] != '.'))
	continue;
      ext[0] = di.kv.str[9];
      ext[1] = di.kv.str[10];
      ext[2] = 0;
      if (!strcmp(ext, "en"))
	continue;
      if (!susetags_find(repo, di.kv.str, &filechksum, &filechksumtype))
	continue;
      handle = repodata_new_handle(data);
      repodata_set_str(data, handle, SUSETAGS_FILE_NAME, di.kv.str);
      if (filechksumtype)
	repodata_set_bin_checksum(data, handle, SUSETAGS_FILE_CHECKSUM, filechksumtype, filechksum);
      if (!strcmp(ext, "DU"))
	{
	  repodata_add_idarray(data, handle, REPOSITORY_KEYS, SOLVABLE_DISKUSAGE);
	  repodata_add_idarray(data, handle, REPOSITORY_KEYS, REPOKEY_TYPE_DIRNUMNUMARRAY);
	}
      else if (!strcmp(ext, "FL"))
	{
	  repodata_add_idarray(data, handle, REPOSITORY_KEYS, SOLVABLE_FILELIST);
	  repodata_add_idarray(data, handle, REPOSITORY_KEYS, REPOKEY_TYPE_DIRSTRARRAY);
	}
      else
	{
	  for (i = 0; susetags_langtags[i]; i += 2)
	    {
	      repodata_add_idarray(data, handle, REPOSITORY_KEYS, pool_id2langid(pool, susetags_langtags[i], ext, 1));
	      repodata_add_idarray(data, handle, REPOSITORY_KEYS, susetags_langtags[i + 1]);
	    }
	}
      repodata_add_flexarray(data, SOLVID_META, REPOSITORY_EXTERNAL, handle);
    }
  dataiterator_free(&di);
}

int
susetags_load_ext(Repo *repo, Repodata *data)
{
  const char *filename, *descrdir;
  Id defvendor;
  char ext[3];
  FILE *fp;
  zpm_repoinfo *cinfo;
  const unsigned char *filechksum;
  Id filechksumtype;

  cinfo =  (zpm_repoinfo *) repo->appdata;
  filename = repodata_lookup_str(data, SOLVID_META, SUSETAGS_FILE_NAME);
  if (!filename)
    return 0;
  /* susetags load */
  ext[0] = filename[9];
  ext[1] = filename[10];
  ext[2] = 0;
#if 1
  printf("[%s:%s", repo->name, ext);
#endif
  if (usecachedrepo(repo, ext, cinfo->extcookie, 0))
    {
      printf(" cached]\n"); fflush(stdout);
      return 1;
    }
#if 1
  printf(" fetching]\n"); fflush(stdout);
#endif
  defvendor = repo_lookup_id(repo, SOLVID_META, SUSETAGS_DEFAULTVENDOR);
  descrdir = repo_lookup_str(repo, SOLVID_META, SUSETAGS_DESCRDIR);
  if (!descrdir)
    descrdir = "suse/setup/descr";
  filechksumtype = 0;
  filechksum = repodata_lookup_bin_checksum(data, SOLVID_META, SUSETAGS_FILE_CHECKSUM, &filechksumtype);
  if ((fp = curlfopen(cinfo, pool_tmpjoin(repo->pool, descrdir, "/", filename), iscompressed(filename), filechksum, filechksumtype, 0)) == 0)
    return 0;
  repo_add_susetags(repo, fp, defvendor, ext, REPO_USE_LOADING|REPO_EXTEND_SOLVABLES);
  fclose(fp);
  writecachedrepo(repo, data, ext, cinfo->extcookie);
  return 1;
}

Id
nscallback(Pool *pool, void *data, Id name, Id evr)
{
  if (name == NAMESPACE_PRODUCTBUDDY)
    {    
      /* SUSE specific hack: each product has an associated rpm */
      Solvable *s = pool->solvables + evr; 
      Id p, pp, cap; 
      
      cap = pool_str2id(pool, pool_tmpjoin(pool, "product(", pool_id2str(pool, s->name) + 8, ")"), 0);
      if (!cap)
        return 0;
      cap = pool_rel2id(pool, cap, s->evr, REL_EQ, 0);
      if (!cap)
        return 0;
      FOR_PROVIDES(p, pp, cap) 
        {
          Solvable *ps = pool->solvables + p; 
          if (ps->repo == s->repo && ps->arch == s->arch)
            break;
        }
      return p;
    }
  return 0;
}

/* load callback */

int
load_stub(Pool *pool, Repodata *data, void *dp)
{
  zpm_repoinfo *cinfo =  (zpm_repoinfo *) data->repo->appdata;
  if (cinfo->type == TYPE_SUSETAGS)
    return susetags_load_ext(data->repo, data);
  if (cinfo->type == TYPE_RPMMD)
    return repomd_load_ext(data->repo, data);
  return 0;
}

char *calccachepath(Repo *repo, const char *repoext)
{
  char *q, *p = pool_tmpjoin(repo->pool, SOLVCACHE_PATH, "/", repo->name);
  if (repoext)
    {
      p = pool_tmpappend(repo->pool, p, "_", repoext);
      p = pool_tmpappend(repo->pool, p, ".solvx", 0);
    }
  else
    p = pool_tmpappend(repo->pool, p, ".solv", 0);
  q = p + strlen(SOLVCACHE_PATH) + 1;
  if (*q == '.')
    *q = '_';
  for (; *q; q++)
    if (*q == '/')
      *q = '_';
  return p;
}

int
usecachedrepo(Repo *repo, const char *repoext, unsigned char *cookie, int mark)
{
  FILE *fp;
  unsigned char mycookie[32];
  unsigned char myextcookie[32];
  zpm_repoinfo *cinfo;
  int flags;

  cinfo = (zpm_repoinfo *) repo->appdata;
  if (!(fp = fopen(calccachepath(repo, repoext), "r")))
    return 0;
  if (fseek(fp, -sizeof(mycookie), SEEK_END) || fread(mycookie, sizeof(mycookie), 1, fp) != 1)
    {
      fclose(fp);
      return 0;
    }
  if (cookie && memcmp(cookie, mycookie, sizeof(mycookie)))
    {
      fclose(fp);
      return 0;
    }
  if (cinfo && !repoext)
    {
      if (fseek(fp, -sizeof(mycookie) * 2, SEEK_END) || fread(myextcookie, sizeof(myextcookie), 1, fp) != 1)
	{
	  fclose(fp);
	  return 0;
	}
    }
  rewind(fp);

  flags = 0;
  if (repoext)
    {
      flags = REPO_USE_LOADING|REPO_EXTEND_SOLVABLES;
      if (strcmp(repoext, "DL") != 0)
        flags |= REPO_LOCALPOOL;	/* no local pool for DL so that we can compare IDs */
    }

  if (repo_add_solv_flags(repo, fp, flags))
    {
      fclose(fp);
      return 0;
    }
  if (cinfo && !repoext)
    {
      memcpy(cinfo->cookie, mycookie, sizeof(mycookie));
      memcpy(cinfo->extcookie, myextcookie, sizeof(myextcookie));
    }
  if (mark)
    futimes(fileno(fp), 0);	/* try to set modification time */
  fclose(fp);
  return 1;
}

void
writecachedrepo(Repo *repo, Repodata *info, const char *repoext, unsigned char *cookie)
{
  FILE *fp;
  int i, fd;
  char *tmpl;
  zpm_repoinfo *cinfo;
  int onepiece;

  cinfo =  (zpm_repoinfo *) repo->appdata;
  mkdir(SOLVCACHE_PATH, 0755);
  /* use dupjoin instead of tmpjoin because tmpl must survive repo_write */
  tmpl = solv_dupjoin(SOLVCACHE_PATH, "/", ".newsolv-XXXXXX");
  fd = mkstemp(tmpl);
  if (fd < 0)
    {
      free(tmpl);
      return;
    }
  fchmod(fd, 0444);
  if (!(fp = fdopen(fd, "w")))
    {
      close(fd);
      unlink(tmpl);
      free(tmpl);
      return;
    }

  onepiece = 1;
  for (i = repo->start; i < repo->end; i++)
   if (repo->pool->solvables[i].repo != repo)
     break;
  if (i < repo->end)
    onepiece = 0;

  if (!info)
    repo_write(repo, fp, repo_write_stdkeyfilter, 0, 0);
  else if (repoext)
    repodata_write(info, fp, repo_write_stdkeyfilter, 0);
  else
    {
      int oldnrepodata = repo->nrepodata;
      repo->nrepodata = 1;	/* XXX: do this right */
      repo_write(repo, fp, repo_write_stdkeyfilter, 0, 0);
      repo->nrepodata = oldnrepodata;
      onepiece = 0;
    }

  if (!repoext && cinfo)
    {
      if (!cinfo->extcookie[0])
	{
	  /* create the ext cookie and append it */
	  /* we just need some unique ID */
	  struct stat stb;
	  if (!fstat(fileno(fp), &stb))
	    {
	      int i;

	      calc_checksum_stat(&stb, REPOKEY_TYPE_SHA256, cinfo->extcookie);
	      for (i = 0; i < 32; i++)
		cinfo->extcookie[i] ^= cookie[i];
	    }
	  if (cinfo->extcookie[0] == 0)
	    cinfo->extcookie[0] = 1;
	}
      if (fwrite(cinfo->extcookie, 32, 1, fp) != 1)
	{
	  fclose(fp);
	  unlink(tmpl);
	  free(tmpl);
	  return;
	}
    }
  /* append our cookie describing the metadata state */
  if (fwrite(cookie, 32, 1, fp) != 1)
    {
      fclose(fp);
      unlink(tmpl);
      free(tmpl);
      return;
    }
  if (fclose(fp))
    {
      unlink(tmpl);
      free(tmpl);
      return;
    }
  if (onepiece)
    {
      /* switch to just saved repo to activate paging and save memory */
      FILE *fp = fopen(tmpl, "r");
      if (fp)
	{
	  if (!repoext)
	    {
	      /* main repo */
	      repo_empty(repo, 1);
	      if (repo_add_solv_flags(repo, fp, SOLV_ADD_NO_STUBS))
		{
		  /* oops, no way to recover from here */
		  fprintf(stderr, "internal error\n");
		  exit(1);
		}
	    }
	  else
	    {
	      /* make sure repodata contains complete repo */
	      /* (this is how repodata_write saves it) */
	      repodata_extend_block(info, repo->start, repo->end - repo->start);
	      info->state = REPODATA_LOADING;
	      /* no need for LOCALPOOL as pool already contains ids */
	      repo_add_solv_flags(repo, fp, REPO_USE_LOADING|REPO_EXTEND_SOLVABLES);
	      info->state = REPODATA_AVAILABLE;	/* in case the load failed */
	    }
	  fclose(fp);
	}
    }
  if (!rename(tmpl, calccachepath(repo, repoext)))
    unlink(tmpl);
  free(tmpl);
}


void
read_repos(Pool *pool, zpm_repoinfo *repoinfos, int nrepoinfos)
{
  Repo *repo;
  zpm_repoinfo *cinfo;
  int i;
  FILE *fp;
  FILE *sigfp;
  const char *filename;
  const unsigned char *filechksum;
  Id filechksumtype;
  const char *descrdir;
  int defvendor;
  struct stat stb;
  Pool *sigpool = 0;
  Repodata *data;
  int badchecksum;
  int dorefresh;
#ifdef DEBIAN
  FILE *fpr;
  int j;
#endif

  repo = repo_create(pool, "@System");
#ifndef DEBIAN
  printf("rpm database:");
  if (stat("/var/lib/rpm/Packages", &stb))
    memset(&stb, 0, sizeof(&stb));
#else
  printf("dpgk database:");
  if (stat("/var/lib/dpkg/status", &stb))
    memset(&stb, 0, sizeof(&stb));
#endif
  calc_checksum_stat(&stb, REPOKEY_TYPE_SHA256, installedcookie);
  if (usecachedrepo(repo, 0, installedcookie, 0))
    printf(" cached\n");
  else
    {
#ifndef DEBIAN
      FILE *ofp;
      int done = 0;
#endif
      printf(" reading\n");

#ifdef PRODUCTS_PATH
      repo_add_products(repo, PRODUCTS_PATH, 0, REPO_NO_INTERNALIZE);
#endif
#ifndef DEBIAN
      if ((ofp = fopen(calccachepath(repo, 0), "r")) != 0)
	{
	  Repo *ref = repo_create(pool, "@System.old");
	  if (!repo_add_solv(ref, ofp))
	    {
	      repo_add_rpmdb(repo, ref, 0, REPO_REUSE_REPODATA);
	      done = 1;
	    }
	  fclose(ofp);
	  repo_free(ref, 1);
	}
      if (!done)
        repo_add_rpmdb(repo, 0, 0, REPO_REUSE_REPODATA);
#else
        repo_add_debdb(repo, 0, REPO_REUSE_REPODATA);
#endif
      writecachedrepo(repo, 0, 0, installedcookie);
    }
  pool_set_installed(pool, repo);

  for (i = 0; i < nrepoinfos; i++)
    {
      cinfo = repoinfos + i;
      if (!cinfo->enabled)
	continue;

      repo = repo_create(pool, cinfo->alias);
      cinfo->repo = repo;
      repo->appdata = cinfo;
      repo->priority = 99 - cinfo->priority;

      dorefresh = cinfo->autorefresh;
      if (dorefresh && cinfo->metadata_expire && stat(calccachepath(repo, 0), &stb) == 0)
	{
	  if (cinfo->metadata_expire == -1 || time(0) - stb.st_mtime < cinfo->metadata_expire)
	    dorefresh = 0;
	}
      if (!dorefresh && usecachedrepo(repo, 0, 0, 0))
	{
	  printf("repo '%s':", cinfo->alias);
	  printf(" cached\n");
	  continue;
	}
      badchecksum = 0;
      switch (cinfo->type)
	{
        case TYPE_RPMMD:
	  printf("rpmmd repo '%s':", cinfo->alias);
	  fflush(stdout);
	  if ((fp = curlfopen(cinfo, "repodata/repomd.xml", 0, 0, 0, 0)) == 0)
	    {
	      printf(" no repomd.xml file, skipped\n");
	      repo_free(repo, 1);
	      cinfo->repo = 0;
	      break;
	    }
	  calc_checksum_fp(fp, REPOKEY_TYPE_SHA256, cinfo->cookie);
	  if (usecachedrepo(repo, 0, cinfo->cookie, 1))
	    {
	      printf(" cached\n");
              fclose(fp);
	      break;
	    }
	  if (cinfo->repo_gpgcheck)
	    {
	      sigfp = curlfopen(cinfo, "repodata/repomd.xml.asc", 0, 0, 0, 0);
	      if (!sigfp)
		{
		  printf(" unsigned, skipped\n");
		  fclose(fp);
		  break;
		}
	      if (!sigpool)
		sigpool = read_sigs();
	      if (!checksig(sigpool, fp, sigfp))
		{
		  printf(" checksig failed, skipped\n");
		  fclose(sigfp);
		  fclose(fp);
		  break;
		}
	      fclose(sigfp);
	    }
	  repo_add_repomdxml(repo, fp, 0);
	  fclose(fp);
	  printf(" fetching\n");
	  filename = repomd_find(repo, "primary", &filechksum, &filechksumtype);
	  if (filename && (fp = curlfopen(cinfo, filename, iscompressed(filename), filechksum, filechksumtype, &badchecksum)) != 0)
	    {
	      repo_add_rpmmd(repo, fp, 0, 0);
	      fclose(fp);
	    }
	  if (badchecksum)
	    break;	/* hopeless */

	  filename = repomd_find(repo, "updateinfo", &filechksum, &filechksumtype);
	  if (filename && (fp = curlfopen(cinfo, filename, iscompressed(filename), filechksum, filechksumtype, &badchecksum)) != 0)
	    {
	      repo_add_updateinfoxml(repo, fp, 0);
	      fclose(fp);
	    }

	  data = repo_add_repodata(repo, 0);
	  if (!repomd_add_ext(repo, data, "deltainfo"))
	    repomd_add_ext(repo, data, "prestodelta");
	  repomd_add_ext(repo, data, "filelists");
	  repodata_internalize(data);
	  if (!badchecksum)
	    writecachedrepo(repo, 0, 0, cinfo->cookie);
	  repodata_create_stubs(repo_last_repodata(repo));
	  break;

        case TYPE_SUSETAGS:
	  printf("susetags repo '%s':", cinfo->alias);
	  fflush(stdout);
	  descrdir = 0;
	  defvendor = 0;
	  if ((fp = curlfopen(cinfo, "content", 0, 0, 0, 0)) == 0)
	    {
	      printf(" no content file, skipped\n");
	      repo_free(repo, 1);
	      cinfo->repo = 0;
	      break;
	    }
	  calc_checksum_fp(fp, REPOKEY_TYPE_SHA256, cinfo->cookie);
	  if (usecachedrepo(repo, 0, cinfo->cookie, 1))
	    {
	      printf(" cached\n");
	      fclose(fp);
	      break;
	    }
	  if (cinfo->repo_gpgcheck)
	    {
	      sigfp = curlfopen(cinfo, "content.asc", 0, 0, 0, 0);
	      if (!sigfp)
		{
		  printf(" unsigned, skipped\n");
		  fclose(fp);
		  break;
		}
	      if (sigfp)
		{
		  if (!sigpool)
		    sigpool = read_sigs();
		  if (!checksig(sigpool, fp, sigfp))
		    {
		      printf(" checksig failed, skipped\n");
		      fclose(sigfp);
		      fclose(fp);
		      break;
		    }
		  fclose(sigfp);
		}
	    }
	  repo_add_content(repo, fp, 0);
	  fclose(fp);
	  defvendor = repo_lookup_id(repo, SOLVID_META, SUSETAGS_DEFAULTVENDOR);
	  descrdir = repo_lookup_str(repo, SOLVID_META, SUSETAGS_DESCRDIR);
	  if (!descrdir)
	    descrdir = "suse/setup/descr";
	  filename = susetags_find(repo, "packages.gz", &filechksum, &filechksumtype);
          if (!filename)
	    filename = susetags_find(repo, "packages", &filechksum, &filechksumtype);
	  if (!filename)
	    {
	      printf(" no packages file entry, skipped\n");
	      break;
	    }
	  printf(" fetching\n");
	  if ((fp = curlfopen(cinfo, pool_tmpjoin(pool, descrdir, "/", filename), iscompressed(filename), filechksum, filechksumtype, &badchecksum)) == 0)
	    break;	/* hopeless */
	  repo_add_susetags(repo, fp, defvendor, 0, REPO_NO_INTERNALIZE|SUSETAGS_RECORD_SHARES);
	  fclose(fp);
	  /* add default language */
	  filename = susetags_find(repo, "packages.en.gz", &filechksum, &filechksumtype);
          if (!filename)
	    filename = susetags_find(repo, "packages.en", &filechksum, &filechksumtype);
	  if (filename)
	    {
	      if ((fp = curlfopen(cinfo, pool_tmpjoin(pool, descrdir, "/", filename), iscompressed(filename), filechksum, filechksumtype, &badchecksum)) != 0)
		{
		  repo_add_susetags(repo, fp, defvendor, 0, REPO_NO_INTERNALIZE|REPO_REUSE_REPODATA|REPO_EXTEND_SOLVABLES);
		  fclose(fp);
		}
	    }
          repo_internalize(repo);
	  data = repo_add_repodata(repo, 0);
	  susetags_add_ext(repo, data);
	  repodata_internalize(data);
	  if (!badchecksum)
	    writecachedrepo(repo, 0, 0, cinfo->cookie);
	  repodata_create_stubs(repo_last_repodata(repo));
	  break;

#ifdef DEBIAN
        case TYPE_DEBIAN:
	  printf("debian repo '%s':", cinfo->alias);
	  fflush(stdout);
	  filename = solv_dupjoin("dists/", cinfo->name, "/Release");
	  if ((fpr = curlfopen(cinfo, filename, 0, 0, 0, 0)) == 0)
	    {
	      printf(" no Release file, skipped\n");
	      repo_free(repo, 1);
	      cinfo->repo = 0;
	      free((char *)filename);
	      break;
	    }
	  solv_free((char *)filename);
	  if (cinfo->repo_gpgcheck)
	    {
	      filename = solv_dupjoin("dists/", cinfo->name, "/Release.gpg");
	      sigfp = curlfopen(cinfo, filename, 0, 0, 0, 0);
	      solv_free((char *)filename);
	      if (!sigfp)
		{
		  printf(" unsigned, skipped\n");
		  fclose(fpr);
		  break;
		}
	      if (!sigpool)
		sigpool = read_sigs();
	      if (!checksig(sigpool, fpr, sigfp))
		{
		  printf(" checksig failed, skipped\n");
		  fclose(sigfp);
		  fclose(fpr);
		  break;
		}
	      fclose(sigfp);
	    }
	  calc_checksum_fp(fpr, REPOKEY_TYPE_SHA256, cinfo->cookie);
	  if (usecachedrepo(repo, 0, cinfo->cookie, 1))
	    {
	      printf(" cached\n");
              fclose(fpr);
	      break;
	    }
	  printf(" fetching\n");
          for (j = 0; j < cinfo->ncomponents; j++)
	    {
	      if (!(filename = debian_find_component(cinfo, fpr, cinfo->components[j], &filechksum, &filechksumtype)))
		{
		  printf("[component %s not found]\n", cinfo->components[j]);
		  continue;
		}
	      if ((fp = curlfopen(cinfo, filename, iscompressed(filename), filechksum, filechksumtype, &badchecksum)) != 0)
		{
	          repo_add_debpackages(repo, fp, 0);
		  fclose(fp);
		}
	      solv_free((char *)filechksum);
	      solv_free((char *)filename);
	    }
	  fclose(fpr);
	  if (!badchecksum)
	    writecachedrepo(repo, 0, 0, cinfo->cookie);
	  break;
#endif

	default:
	  printf("unsupported repo '%s': skipped\n", cinfo->alias);
	  repo_free(repo, 1);
	  cinfo->repo = 0;
	  break;
	}
    }
  if (sigpool)
    pool_free(sigpool);
}
