#include <stdlib.h>
#include <stdio.h>
#include <assert.h>
#include <time.h>
#include <limits.h>
#include <errno.h>
#include <unistd.h>

#include "server.h"
#include "config.h"

static char *get_path(const char *sname)
{
	static char path[PATH_MAX];

	if (snprintf(path, PATH_MAX, "%s/servers/%s",
	             config.root, sname) >= PATH_MAX) {
		fprintf(stderr, "%s: Too long\n", config.root);
		return NULL;
	}

	return path;
}

static int read_server_meta(FILE *file, const char *path, struct server_state *state)
{
	int ret;

	assert(file != NULL);
	assert(path != NULL);
	assert(state != NULL);

	errno = 0;
	ret = fscanf(file, "last seen: %ju\nexpire: %ju\n",
	             &state->last_seen, &state->expire);

	if (ret == EOF && errno != 0) {
		perror(path);
		return 0;
	} else if (ret == EOF || ret == 0) {
		fprintf(stderr, "%s: Can't match 'last seen' field\n", path);
		return 0;
	} else if (ret == 1) {
		fprintf(stderr, "%s: Can't match 'expire' field\n", path);
		return 0;
	}

	return 1;
}

int read_server_state(struct server_state *state, char *sname)
{
	FILE *file = NULL;
	char *path;
	unsigned i;
	int ret;

	assert(state != NULL);
	assert(sname != NULL);

	if (!(path = get_path(sname)))
		goto fail;
	if (!(file = fopen(path, "r"))) {
		perror(path);
		goto fail;
	}

	if (!read_server_meta(file, path, state))
		goto fail;

	errno = 0;
	ret = fscanf(file, " %d", &state->num_clients);

	if (ret == EOF && errno != 0) {
		perror(path);
		goto fail;
	} else if (ret == EOF || ret == 0) {
		fprintf(stderr, "%s: Cannot match clients number\n", path);
		goto fail;
	}

	for (i = 0; i < state->num_clients; i++) {
		struct client *client = &state->clients[i];

		errno = 0;
		ret = fscanf(file, " %s %s %ld",
		             client->name, client->clan, &client->score);

		if (ret == EOF && errno != 0) {
			perror(path);
			goto fail;
		} else if (ret != 3) {
			fprintf(stderr, "%s: Only %d over %d clients matched\n",
			        path, ret, state->num_clients);
			goto fail;
		}
	}

	/* Assume for now that only CTF games are ranked */
	state->gametype = "CTF";

	fclose(file);
	return 1;

fail:
	if (file)
		fclose(file);
	return 0;
}

static int write_server_meta(FILE *file, const char *path, struct server_state *state)
{
	int ret;

	assert(file != NULL);
	assert(path != NULL);
	assert(state != NULL);

	ret = fprintf(file, "last seen: %ju\nexpire: %ju\n",
	              state->last_seen, state->expire);
	if (ret < 0) {
		perror(path);
		return 0;
	}

	return 1;
}

int write_server_state(struct server_state *state, const char *sname)
{
	FILE *file = NULL;
	char *path;
	unsigned i;

	assert(state != NULL);

	if (!(path = get_path(sname)))
		goto fail;
	if (!(file = fopen(path, "w"))) {
		perror(path);
		goto fail;
	}

	if (!write_server_meta(file, path, state))
		goto fail;

	if (fprintf(file, "%d\n", state->num_clients) <= 0) {
		perror(path);
		goto fail;
	}

	for (i = 0; i < state->num_clients; i++) {
		struct client *client = &state->clients[i];

		if (fprintf(file, "%s %s %ld\n",
		            client->name, client->clan, client->score) <= 0) {
			perror(path);
			goto fail;
		}
	}

	fclose(file);
	return 1;

fail:
	if (file)
		fclose(file);
	return 0;
}

int server_expired(struct server_state *state)
{
	time_t now;

	assert(state != NULL);

	now = time(NULL);

	if (now == (time_t)-1)
		return 1;

	return now > state->expire;
}

static unsigned min(unsigned a, unsigned b)
{
	return a < b ? a : b;
}

void mark_server_offline(struct server_state *state)
{
	time_t now;

	assert(state != NULL);

	/*
	 * We won't want to check an offline server too often, because it will
	 * add a (probably) unnecessary 3 seconds delay when polling.  However
	 * sometime the server is online but our UDP packets get lost 3 times
	 * in a row, in this case we don't want to delay too much the next poll.
	 *
	 * To meet the requirements above, we schedule the next poll to:
	 *
	 * 	now + min(now - state->last_seen, 2 hours)
	 *
	 * So for example if the server was seen 5 minutes ago, the next poll
	 * will be schedule in 5 minutes.  If the server is still offline 5
	 * minutes later, then we schedule the next poll in 10 minutes...  Up
	 * to a maximum of 2 hours.
	 */

	now = time(NULL);
	state->expire = now + min(now - state->last_seen, 2 * 3600);
}

void mark_server_online(struct server_state *state, int expire_now)
{
	time_t now;
	static int initialized = 0;

	assert(state != NULL);

	now = time(NULL);
	state->last_seen = now;

	if (expire_now) {
		state->expire = 0;
	} else {
		/*
		 * We just choose a random value between a half hour and
		 * one and a half hour, so that we do not have too much
		 * servers to update at the same time.
		 */
		if (!initialized) {
			initialized = 1;
			srand(now);
		}
		state->expire = now + 1800 + 3600 * ((double)rand() / (double)RAND_MAX);
	}
}

void remove_server(const char *name)
{
	const char *path;

	assert(name != NULL);

	path = get_path(name);
	if (!path)
		return;

	if (unlink(path) == -1)
		if (errno != ENOENT)
			perror(path);
}

int create_server(const char *sname)
{
	static const struct server_state SERVER_STATE_ZERO;
	struct server_state state = SERVER_STATE_ZERO;

	state.last_seen = time(NULL);

	return write_server_state(&state, sname);
}

int server_exist(const char *sname)
{
	const char *path;

	path = get_path(sname);
	if (!path)
		return 1;

	return access(path, F_OK) == 0;
}
