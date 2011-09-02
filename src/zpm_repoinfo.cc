#include <dirent.h>
#include <stdlib.h>
#include <string.h>

#include "zpm_repoinfo.h"

extern "C" {
#include <solv/repo.h>
#include <solv/solver.h>
}

#define METADATA_EXPIRE (60 * 15)

static int
read_repoinfos_sort(const void *ap, const void *bp)
{
    const zpm_repoinfo *a = (zpm_repoinfo *) ap;
    const zpm_repoinfo *b = (zpm_repoinfo *) bp;
    return strcmp(a->alias, b->alias);
}

#ifndef DEBIAN

zpm_repoinfo *
zpm_read_repoinfos(Pool *pool, const char *reposdir, int *nrepoinfosp)
{
  char buf[4096];
  char buf2[4096], *kp, *vp, *kpe;
  DIR *dir;
  FILE *fp;
  struct dirent *ent;
  int l, rdlen;
  zpm_repoinfo *repoinfos = 0, *cinfo;
  int nrepoinfos = 0;

  rdlen = strlen(reposdir);
  dir = opendir(reposdir);
  if (!dir)
    {
      *nrepoinfosp = 0;
      return 0;
    }
  while ((ent = readdir(dir)) != 0)
    {
      if (ent->d_name[0] == '.')
	continue;
      l = strlen(ent->d_name);
      if (l < 6 || rdlen + 2 + l >= sizeof(buf) || strcmp(ent->d_name + l - 5, ".repo") != 0)
	continue;
      snprintf(buf, sizeof(buf), "%s/%s", reposdir, ent->d_name);
      if ((fp = fopen(buf, "r")) == 0)
	{
	  perror(buf);
	  continue;
	}
      cinfo = 0;
      while(fgets(buf2, sizeof(buf2), fp))
	{
	  l = strlen(buf2);
	  if (l == 0)
	    continue;
	  while (l && (buf2[l - 1] == '\n' || buf2[l - 1] == ' ' || buf2[l - 1] == '\t'))
	    buf2[--l] = 0;
	  kp = buf2;
	  while (*kp == ' ' || *kp == '\t')
	    kp++;
	  if (!*kp || *kp == '#')
	    continue;
#ifdef FEDORA
	  if (strchr(kp, '$'))
	    kp = yum_substitute(pool, kp);
#endif
	  if (*kp == '[')
	    {
	      vp = strrchr(kp, ']');
	      if (!vp)
		continue;
	      *vp = 0;
	      repoinfos = (zpm_repoinfo *) solv_extend(repoinfos, nrepoinfos, 1, sizeof(*repoinfos), 15);
	      cinfo = repoinfos + nrepoinfos++;
	      memset(cinfo, 0, sizeof(*cinfo));
	      cinfo->alias = strdup(kp + 1);
	      cinfo->type = TYPE_RPMMD;
	      cinfo->autorefresh = 1;
	      cinfo->priority = 99;
#ifndef FEDORA
	      cinfo->repo_gpgcheck = 1;
#endif
	      cinfo->metadata_expire = METADATA_EXPIRE;
	      continue;
	    }
	  if (!cinfo)
	    continue;
          vp = strchr(kp, '=');
	  if (!vp)
	    continue;
	  for (kpe = vp - 1; kpe >= kp; kpe--)
	    if (*kpe != ' ' && *kpe != '\t')
	      break;
	  if (kpe == kp)
	    continue;
	  vp++;
	  while (*vp == ' ' || *vp == '\t')
	    vp++;
	  kpe[1] = 0;
	  if (!strcmp(kp, "name"))
	    cinfo->name = strdup(vp);
	  else if (!strcmp(kp, "enabled"))
	    cinfo->enabled = *vp == '0' ? 0 : 1;
	  else if (!strcmp(kp, "autorefresh"))
	    cinfo->autorefresh = *vp == '0' ? 0 : 1;
	  else if (!strcmp(kp, "gpgcheck"))
	    cinfo->pkgs_gpgcheck = *vp == '0' ? 0 : 1;
	  else if (!strcmp(kp, "repo_gpgcheck"))
	    cinfo->repo_gpgcheck = *vp == '0' ? 0 : 1;
	  else if (!strcmp(kp, "baseurl"))
	    cinfo->baseurl = strdup(vp);
	  else if (!strcmp(kp, "mirrorlist"))
	    {
	      if (strstr(vp, "metalink"))
	        cinfo->metalink = strdup(vp);
	      else
	        cinfo->mirrorlist = strdup(vp);
	    }
	  else if (!strcmp(kp, "path"))
	    {
	      if (vp && strcmp(vp, "/") != 0)
	        cinfo->path = strdup(vp);
	    }
	  else if (!strcmp(kp, "type"))
	    {
	      if (!strcmp(vp, "yast2"))
	        cinfo->type = TYPE_SUSETAGS;
	      else if (!strcmp(vp, "rpm-md"))
	        cinfo->type = TYPE_RPMMD;
	      else if (!strcmp(vp, "plaindir"))
	        cinfo->type = TYPE_PLAINDIR;
	      else
	        cinfo->type = TYPE_UNKNOWN;
	    }
	  else if (!strcmp(kp, "priority"))
	    cinfo->priority = atoi(vp);
	  else if (!strcmp(kp, "keeppackages"))
	    cinfo->keeppackages = *vp == '0' ? 0 : 1;
	}
      fclose(fp);
      cinfo = 0;
    }
  closedir(dir);
  qsort(repoinfos, nrepoinfos, sizeof(*repoinfos), read_repoinfos_sort);
  *nrepoinfosp = nrepoinfos;
  return repoinfos;
}

#else

zpm_repoinfo *
read_repoinfos(Pool *pool, const char *reposdir, int *nrepoinfosp)
{
  FILE *fp;
  char buf[4096];
  char buf2[4096];
  int l;
  char *kp, *url, *distro;
  zpm_repoinfo *repoinfos = 0, *cinfo;
  int nrepoinfos = 0;
  DIR *dir = 0;
  struct dirent *ent;

  fp = fopen("/etc/apt/sources.list", "r");
  while (1)
    {
      if (!fp)
	{
	  if (!dir)
	    {
	      dir = opendir("/etc/apt/sources.list.d");
	      if (!dir)
		break;
	    }
	  if ((ent = readdir(dir)) == 0)
	    {
	      closedir(dir);
	      break;
	    }
	  if (ent->d_name[0] == '.')
	    continue;
	  l = strlen(ent->d_name);
	  if (l < 5 || strcmp(ent->d_name + l - 5, ".list") != 0)
	    continue;
	  snprintf(buf, sizeof(buf), "%s/%s", "/etc/apt/sources.list.d", ent->d_name);
	  if (!(fp = fopen(buf, "r")))
	    continue;
	}
      while(fgets(buf2, sizeof(buf2), fp))
	{
	  l = strlen(buf2);
	  if (l == 0)
	    continue;
	  while (l && (buf2[l - 1] == '\n' || buf2[l - 1] == ' ' || buf2[l - 1] == '\t'))
	    buf2[--l] = 0;
	  kp = buf2;
	  while (*kp == ' ' || *kp == '\t')
	    kp++;
	  if (!*kp || *kp == '#')
	    continue;
	  if (strncmp(kp, "deb", 3) != 0)
	    continue;
	  kp += 3;
	  if (*kp != ' ' && *kp != '\t')
	    continue;
	  while (*kp == ' ' || *kp == '\t')
	    kp++;
	  if (!*kp)
	    continue;
	  url = kp;
	  while (*kp && *kp != ' ' && *kp != '\t')
	    kp++;
	  if (*kp)
	    *kp++ = 0;
	  while (*kp == ' ' || *kp == '\t')
	    kp++;
	  if (!*kp)
	    continue;
	  distro = kp;
	  while (*kp && *kp != ' ' && *kp != '\t')
	    kp++;
	  if (*kp)
	    *kp++ = 0;
	  while (*kp == ' ' || *kp == '\t')
	    kp++;
	  if (!*kp)
	    continue;
	  repoinfos = solv_extend(repoinfos, nrepoinfos, 1, sizeof(*repoinfos), 15);
	  cinfo = repoinfos + nrepoinfos++;
	  memset(cinfo, 0, sizeof(*cinfo));
	  cinfo->baseurl = strdup(url);
	  cinfo->alias = solv_dupjoin(url, "/", distro);
	  cinfo->name = strdup(distro);
	  cinfo->type = TYPE_DEBIAN;
	  cinfo->enabled = 1;
	  cinfo->autorefresh = 1;
	  cinfo->repo_gpgcheck = 1;
	  cinfo->metadata_expire = METADATA_EXPIRE;
	  while (*kp)
	    {
	      char *compo;
	      while (*kp == ' ' || *kp == '\t')
		kp++;
	      if (!*kp)
		break;
	      compo = kp;
	      while (*kp && *kp != ' ' && *kp != '\t')
		kp++;
	      if (*kp)
		*kp++ = 0;
	      cinfo->components = solv_extend(cinfo->components, cinfo->ncomponents, 1, sizeof(*cinfo->components), 15);
	      cinfo->components[cinfo->ncomponents++] = strdup(compo);
	    }
	}
      fclose(fp);
      fp = 0;
    }
  qsort(repoinfos, nrepoinfos, sizeof(*repoinfos), read_repoinfos_sort);
  *nrepoinfosp = nrepoinfos;
  return repoinfos;
}

#endif

static void
free_repoinfos(zpm_repoinfo *repoinfos, int nrepoinfos)
{
  int i, j;
  for (i = 0; i < nrepoinfos; i++)
    {
      zpm_repoinfo *cinfo = repoinfos + i;
      solv_free(cinfo->name);
      solv_free(cinfo->alias);
      solv_free(cinfo->path);
      solv_free(cinfo->metalink);
      solv_free(cinfo->mirrorlist);
      solv_free(cinfo->baseurl);
      for (j = 0; j < cinfo->ncomponents; j++)
        solv_free(cinfo->components[j]);
      solv_free(cinfo->components);
    }
  solv_free(repoinfos);
}

