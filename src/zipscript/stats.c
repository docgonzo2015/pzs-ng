#include <unistd.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/uio.h>

#include "objects.h"
#include "macros.h"
#include "convert.h"
#include "zsfunctions.h"
#include "race-file.h"

/*#ifdef _SunOS_
#include "scandir.h"
#endif*/

#include "zsconfig.h"
#include "zsconfig.defaults.h"

#include "stats.h"

#include <glufile.h>

/*void
updatestats_free(GLOBAL *g)
{
	int n;

	for (n = 0; n < g->v.total.users; n++)
		ng_free(g->ui[n]);
	ng_free(g->ui);

	for (n = 0; n < g->v.total.groups; n++)
		ng_free(g->gi[n]);
	ng_free(g->gi);
}*/

/*
 * Modified   : 01.27.2002 Author     : Dark0n3
 * 
 * Description: Updates existing entries in userI and groupI or creates new, if
 * old doesnt exist
 */
void 
updatestats(struct VARS *raceI, struct USERINFO *userI, struct GROUPINFO *groupI, char *usern, char *group, char *usertag, off_t filesize, unsigned int speed, unsigned int start_time)
{
	int		u_no = -1;
	int		g_no = -1;
	int		n;
	double		speedD = filesize * speed;

	for (n = 0; n < raceI->total.users; n++) {
		if (strncmp(userI[n].name, usern, 24) == 0) {
			u_no = n;
			g_no = userI[n].group;
			break;
		}
	}

	if (u_no == -1) {
		if (!raceI->total.users) {
			raceI->total.start_time = start_time;
			/*
			 * to prevent a possible floating point exception in
			 * convert,
			 */
			/* if first entry in racefile is not the oldest one           */
			if ((int)(raceI->total.stop_time - raceI->total.start_time) < 1)
				raceI->total.stop_time = raceI->total.start_time + 1;
		}
		u_no = raceI->total.users++;
		memcpy(userI[u_no].name, usern, 24);
		memcpy(userI[u_no].tagline, usertag, 64);
		userI[u_no].files = 0;

		for (n = 0; n < raceI->total.groups; n++) {
			if (strncmp(groupI[n].name, group, 24) == 0) {
				g_no = n;
				break;
			}
		}

		if (g_no == -1) {
			g_no = raceI->total.groups++;
			memcpy(groupI[g_no].name, group, 24);
		}
		userI[u_no].group = g_no;
	}
	userI[u_no].bytes += filesize;
	groupI[g_no].bytes += filesize;
	raceI->total.size += filesize;

	userI[u_no].speed += speedD;
	groupI[g_no].speed += speedD;
	raceI->total.speed += speedD;

	userI[u_no].files++;
	groupI[g_no].files++;
	raceI->total.files_missing--;

	speed >>= 10;

	if (speed > (unsigned int)raceI->misc.fastest_user[0]) {
		raceI->misc.fastest_user[1] = u_no;
		raceI->misc.fastest_user[0] = speed;
	}
	if (speed < (unsigned int)raceI->misc.slowest_user[0]) {
		raceI->misc.slowest_user[1] = u_no;
		raceI->misc.slowest_user[0] = speed;
	}
}

/*
 * Modified   : 01.17.2002 Author     : Dark0n3
 * 
 * Description: Sorts userI and groupI
 * Modified by psxc 09.25.2004 - nasty bug
 * Modified by psxc 11.22.2004 - added support for list of ppl against leader.
 * Modified by psxc 12.01.2004 - fixed this routine! :)
 */
void 
sortstats(struct VARS *raceI, struct USERINFO *userI, struct GROUPINFO *groupI)
{
	int		n;
	int		n2;
	int		t;
	int		t2 = 0, t3 = 0;
	int            *p_array = NULL;
	char           *r_list;
	char           *t_list;

	p_array = (int *)ng_realloc(p_array, raceI->total.users * sizeof(int), 1, 1, 1);
	r_list = raceI->misc.racer_list;
	t_list = raceI->misc.total_racer_list;

	for (n = 0; n < raceI->total.users; n++) {
		t = p_array[n];
		for (n2 = n + 1; n2 < raceI->total.users; n2++) {
			if (userI[n].bytes > userI[n2].bytes) {
				p_array[n2]++;
			} else {
				t++;
			}
		}
		userI[t].pos = n;
#if ( get_competitor_list == TRUE )
		if ( (strncmp(raceI->user.name, userI[n].name, (int)strlen(raceI->user.name) < (int)strlen(userI[n].name) ? (int)strlen(raceI->user.name) : (int)strlen(userI[n].name))) || (((int)strlen(raceI->user.name) != (int)strlen(userI[n].name)) && (!strncmp(raceI->user.name, userI[n].name, (int)strlen(raceI->user.name) < (int)strlen(userI[n].name) ? (int)strlen(raceI->user.name) : (int)strlen(userI[n].name))))) {
			if (t2 > 0) {
				if ((int)strlen(racersplit)) {
					r_list += sprintf(r_list, " %s %s", racersplit, convert2(raceI, &userI[n], groupI, racersmsg, t));
				} else {
					r_list += sprintf(r_list, " %s", convert2(raceI, &userI[n], groupI, racersmsg, t));
				}
			} else if ((int)strlen(racersplit_prior)) {
				t2 = 1;
				r_list += sprintf(r_list, "%s %s", racersplit_prior, convert2(raceI, &userI[n], groupI, racersmsg, t));
			} else {
				t2 = 1;
				r_list += sprintf(r_list, "%s", convert2(raceI, &userI[n], groupI, racersmsg, t));
			}
		} else {
			raceI->user.pos = n;
		}
#else
		if (!strcmp(raceI->user.name, userI[n].name))
			raceI->user.pos = n;
#endif
	}
	for (n = 1; n < raceI->total.users; n++) {
		if (t3 > 0) {
			if ((int)strlen(racersplit)) {
				t_list += sprintf(t_list, " %s %s", racersplit, convert2(raceI, &userI[userI[n].pos], groupI, racersmsg, n));
			} else {
				t_list += sprintf(t_list, " %s", convert2(raceI, &userI[userI[n].pos], groupI, racersmsg, n));
			}
		} else if ((int)strlen(racersplit_prior)) {
			t3 = 1;
			t_list += sprintf(t_list, "%s %s", racersplit_prior, convert2(raceI, &userI[userI[n].pos], groupI, racersmsg, n));
		} else {
			t3 = 1;
			t_list += sprintf(t_list, "%s", convert2(raceI, &userI[userI[n].pos], groupI, racersmsg, n));
		}
	}
	bzero(p_array, raceI->total.groups * sizeof(int));

	for (n = 0; n < raceI->total.groups; n++) {
		t = p_array[n];
		for (n2 = n + 1; n2 < raceI->total.groups; n2++) {
			if (groupI[n].bytes >= groupI[n2].bytes) {
				p_array[n2]++;
			} else {
				t++;
			}
		}
		groupI[t].pos = n;
	}
	ng_free(p_array);
}

void 
showstats(struct VARS *raceI, struct USERINFO *userI, struct GROUPINFO *groupI)
{
	int		cnt;

#if ( show_user_info == TRUE )
	d_log(1, "showstats: Showing realtime user stats ...\n");
	if (realtime_user_header != DISABLED) {
		d_log(1, "showstats:   - printing realtime_user_header ...\n");
		printf("%s", convert(raceI, userI, groupI, realtime_user_header));
	}
	if (realtime_user_body != DISABLED) {
		d_log(1, "showstats:   - printing realtime_user_body ...\n");
		for (cnt = 0; cnt < raceI->total.users; cnt++) {
			printf("%s", convert2(raceI, &userI[userI[cnt].pos], groupI, realtime_user_body, cnt));
		}
	}
	if (realtime_user_footer != DISABLED) {
		d_log(1, "showstats:   - printing realtime_user_footer ...\n");
		printf("%s", convert(raceI, userI, groupI, realtime_user_footer));
	}
#endif
#if ( show_group_info == TRUE )
	d_log(1, "showstats: Showing realtime user stats ...\n");
	if (realtime_group_header != DISABLED) {
		d_log(1, "showstats:   - printing realtime_group_header ...\n");
		printf("%s", convert(raceI, userI, groupI, realtime_group_header));
	}
	if (realtime_group_body != DISABLED) {
		d_log(1, "showstats:   - printing realtime_group_body ...\n");
		for (cnt = 0; cnt < raceI->total.groups; cnt++) {
			printf("%s", convert3(raceI, &groupI[groupI[cnt].pos], realtime_group_body, cnt));
		}
	}
	if (realtime_group_footer != DISABLED) {
		d_log(1, "showstats:   - printing realtime_group_footer ...\n");
		printf("%s", convert(raceI, userI, groupI, realtime_group_footer));
	}
#endif
}

/*
 * Created	: 01.15.2002 Modified	: 01.17.2002 Author	: Dark0n3
 * 
 * Description	: Reads transfer stats for all users from userfiles and
 * creates which is used to set dayup/weekup/monthup/allup variables for
 * every user in race.
 */
void 
get_stats(struct VARS *raceI, struct USERINFO *userI)
{
	int		u1, n = 0, m, users = 0, userbuf=20, o = 0;
	char		t_buf[PATH_MAX]; /* target buf */
	char		*lineptr, **strptr;
	static char	*upstrs[] = { "DAYUP", "WKUP", "MONTHUP", "ALLUP", 0 };

	struct userdata	*user = 0;
	struct stat	fileinfo;

	DIR		*dir;
	struct dirent	*dp;
	FILE		*ufile;

	ASECTION	sect;

	if (!(dir = opendir(gl_userfiles))) {
		d_log(1, "get_stats: opendir(%s): %s\n", gl_userfiles, strerror(errno));
		return; 
	}
	
	d_log(1, "get_stats: reading stats..\n");

	while ((dp = readdir(dir))) {
		
		sprintf(t_buf, "%s/%s", gl_userfiles, dp->d_name);

		if (!(ufile = fopen(t_buf, "r"))) {
			d_log(1, "get_stats: fopen(%s): %s\n", t_buf, strerror(errno));
			continue;
		}
		
		if (stat(t_buf, &fileinfo) == -1) {
			d_log(1, "get_stats: stat(%i): %s\n", t_buf, strerror(errno));
			continue;
		}

		if (S_ISDIR(fileinfo.st_mode) == 0) {

			if (o * userbuf <= n) {
				o++;
				user = ng_realloc(user, sizeof(struct userdata)*((o * userbuf) + 1), 0, 1, 0);
				bzero(&user[n], (sizeof(struct userdata)) * userbuf);
			}
			sect.n = 0; sect.s = 0;

			for (strptr = upstrs; *strptr; strptr++) {
				
				if (!(lineptr = get_statline(ufile, *strptr))) {
					d_log(1, "get_stats: Bad/corrupt user file %s\n", t_buf);
					break;
				}
					
				ufile_section(&sect, lineptr);
				
				if (raceI->section <= sect.n)
					user[n].bytes[strptr-upstrs] = sect.s[raceI->section].kilobytes;
					
			}

			if (sect.s)
				free(sect.s);
			
			fclose(ufile);
			
			user[n].name = -1;

			for (m = 0; m < raceI->total.users; m++) {
				if (strcmp(dp->d_name, userI[m].name) != 0)
					continue;
				if (!strcmp(raceI->user.name, userI[m].name)) {
					user[n].bytes[0] += raceI->file.size >> 10;
					user[n].bytes[1] += raceI->file.size >> 10;
					user[n].bytes[2] += raceI->file.size >> 10;
					user[n].bytes[3] += raceI->file.size >> 10;
				}
				user[n].name = m;
				break;
			}
			n++;
		}
	}
	closedir(dir);

	d_log(1, "get_stats: initializing stats..\n");
	users = n;
	for (m = 0; m < raceI->total.users; m++) {
		userI[m].dayup =
			userI[m].wkup =
			userI[m].monthup =
			userI[m].allup = users;
	}

	d_log(1, "get_stats: sorting stats..\n");
	for (n = 0; n < users; n++) {
		if ((u1 = user[n].name) == -1)
			continue;
		for (m = 0; m < users; m++)
			if (m != n) {
				if (user[n].bytes[0] >= user[m].bytes[0])
					userI[u1].dayup--;
				if (user[n].bytes[1] >= user[m].bytes[1])
					userI[u1].wkup--;
				if (user[n].bytes[2] >= user[m].bytes[2])
					userI[u1].monthup--;
				if (user[n].bytes[3] >= user[m].bytes[3])
					userI[u1].allup--;
			}
	}
	ng_free(user);
	d_log(1, "get_stats: done.\n");
}

char *
get_statline(FILE *ufile, const char *linetype)
{
	static int len;
	static char buf[512];

	len = strlen(linetype);

	rewind(ufile);
	
	bzero(buf, sizeof(buf));
	while (fgets(buf, sizeof(buf), ufile))
		if (strncmp(buf, linetype, len) == 0)
			return buf+len;
	
	return NULL;
}