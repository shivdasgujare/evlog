/*
 * Linux Event Logging for the Enterprise
 * Copyright (c) International Business Machines Corp., 2001
 *
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
 *
 *  Please send e-mail to lkessler@users.sourceforge.net if you have
 *  questions or comments.
 *
 *  Project Website:  http://evlog.sourceforge.net/
 *
 */
 
#include <stdio.h>
#include <dlfcn.h>
#include <stdlib.h>
#include <string.h>
#include <dirent.h>
#include "callback.h"

#include "config.h"

#define MAX_BACKENDS	10
#define BACKENDS_DIR 	EVLOG_PLUGIN_DIR
#define SMALL_BUF_SIZE	256

#ifdef DEBUG2
#define TRACE(fmt, args...)             fprintf(stderr, fmt, ##args)
#else
#define TRACE(fmt, args...)             /* fprintf(stderout, fmt, ##args) */
#endif

typedef struct backend {
	void *plugin;
	be_desc_t *desc;
	int state; 
} backend_t;

static int be_add(void *plugin, be_desc_t *desc);
static int be_release(backend_t *);

/* Backends table */
static backend_t be_table[MAX_BACKENDS]; 
static int be_cnt = 0;

/*
 * dlopens the shared lib whose name is name. Returns the handle returned by
 * dlopen().
 */
void * getPlugInHandle(char * name)
{
	void *hdle;
	char lib_path[256];
	char *ext;

	ext = (char *) strrchr(name, '.');

	/* Skip . and .. */
	if (!strcmp(name, ".") || !strcmp(name, ".."))
		return (void *)NULL;
	/* Skip if extension is not .so */
	if (ext == NULL || strcmp(ext, ".so"))
		return (void*)NULL;

	sprintf(lib_path, BACKENDS_DIR "/%s", name);	
	/* Load plugin */
	if (!(hdle = dlopen(lib_path, RTLD_LAZY))) {
		fprintf(stderr, "%s\n", dlerror());
		return (void *)NULL;
	}
	return hdle; 
}

be_desc_t * getPlugInInterface(void * handle)
{
  be_desc_t * d;
	int status;

	/* Get the back end interface functions for this plugin */
	if (!(d = (be_desc_t *)dlsym(handle, "_evlPluginDesc"))) {
		fprintf(stderr, "%s\n", dlerror());
		return NULL;
	}
	/* 
	 * Initialize the plugin. We give the 
	 * callback inf so it can invoke some of
	 * evlogd functions.
	 */
	status = d->init(evl_callback);
	
	if (status == -1) {
		dlclose(handle);
		return NULL;
	}

	return d;
}

/* Called during evlogd setup */
int 
be_init()
{
	struct dirent **plugins;
	int n;
	static void * handle;

	n = scandir(BACKENDS_DIR, &plugins, 0, 0);
	if (n < 0) {
		//perror("scandir");
	} else {
		while(n--) {
			be_desc_t *d;
			
			handle = (void *)getPlugInHandle((char *)(plugins[n]->d_name));
			free(plugins[n]);
			if (!handle)
				continue;
			d = getPlugInInterface(handle);
			if (!d)
				continue;
			
			be_add(handle, d);
		}
		free(plugins);
	}
  return be_cnt;
}

void
be_cleanup()
{
	int i;
	TRACE("be_cleanup invoked...\n");
	for (i = 0; i < MAX_BACKENDS; i++) {
		if (be_table[i].plugin != NULL) {
			dlclose(be_table[i].plugin);
			TRACE("be_cleanup plugin index #%d is set to NULL...\n", i);
			be_table[i].plugin = NULL;
			be_table[i].desc = NULL;
		}
	}
}

int
be_add(void *plugin, be_desc_t *d)
{
	int i;
	if (be_cnt >= MAX_BACKENDS) {
		fprintf(stderr, "No more room for your plugin.\n");
		return -1;
	}

	for (i = 0; i < MAX_BACKENDS; i++) {
		if (be_table[i].desc == NULL) {
			be_table[i].desc = d;	
			be_table[i].plugin = plugin;
			be_cnt++;
			break;
		}
	}
	return 0;
}

static int
be_release(backend_t *be)
{
	be->desc = NULL;
	dlclose(be->plugin);
	be->plugin = NULL;
	be_cnt--;
	return 0;
}

/*
 * Execute all registered functions in the function table
 * (also pass its arguments to those funtions).
 */ 
void 
be_run(const char *data1, const char *data2, evl_callback_t callback)
{
	int i;
	int rc;

	for (i = 0; i < MAX_BACKENDS; i++) {
		if (be_table[i].desc != NULL) {
			rc = be_table[i].desc->process(data1, data2, evl_callback);
			if (rc != 0) {
				/* release the backend */
				be_release(&be_table[i]);
			}
		}
	}
}

#if 0
int 
main(char *argv[], int argc)
{
	/* Initialize backends */
	be_init();
	/* Pass data to back end functions, and run them */
	be_run("data1 is here", "data2 is also here", evl_callback);
	printf("\nclean up...");
	be_cleanup();
	printf("done.\n");
	return 0;
}

#endif

