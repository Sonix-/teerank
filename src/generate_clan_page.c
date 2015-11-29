#include <stdlib.h>
#include <stdio.h>
#include <limits.h>
#include <assert.h>
#include <libgen.h>
#include <string.h>

#include "io.h"

static char path[PATH_MAX];

static char *players_directory;

/*
 * Strictly speaking, INT_MIN is still a valid Elo.  But it just impossible
 * to get it.  And even if a player got it somehow, it will messup the whole
 * system because of underflow, so INT_MIN is safe to use.
 */
#define UNKNOWN_ELO INT_MIN

/*
 * Same as UNKNOWN_ELO but this time UINT_MAX is choosen so even if rank is
 * displayed as it is, players with unknown rank will be at the end since
 * players are sorted in decreasing order.
 */
#define UNKNOWN_RANK UINT_MAX

static int load_elo(const char *name)
{
	int ret, elo;

	assert(name != NULL);

	sprintf(path, "%s/%s/%s", players_directory, name, "elo");

	/* Failing at reading Elo is not fatal, we will just output '?' */
	ret = read_file(path, "%d", &elo);
	if (ret == 1)
		return elo;
	else if (ret == -1)
		perror(path);
	else
		fprintf(stderr, "%s: Cannot match Elo\n", path);

	return UNKNOWN_ELO;
}

static unsigned load_rank(const char *name)
{
	unsigned rank;
	int ret;

	assert(name != NULL);

	sprintf(path, "%s/%s/%s", players_directory, name, "rank");

	/* Failing at reading rank is not fatal, we will just output '?' */
	ret = read_file(path, "%u", &rank);
	if (ret == 1)
		return rank;
	else if (ret == -1)
		perror(path);
	else
		fprintf(stderr, "%s: Cannot match rank\n", path);

	return UNKNOWN_RANK;
}

struct player {
	char name[MAX_NAME_LENGTH], name_hex[MAX_NAME_LENGTH];
	int elo;
	unsigned rank;
};

static void load_player(struct player *player, char *name)
{
	assert(player != NULL);
	assert(name != NULL);

	strcpy(player->name_hex, name);
	hex_to_string(name, player->name);
	player->elo = load_elo(name);
	player->rank = load_rank(name);
}

struct player_array {
	unsigned length;
	struct player *players;
};

static void add_player(struct player_array *array, struct player *player)
{
	static const unsigned OFFSET = 1024;

	assert(array != NULL);
	assert(player != NULL);

	if (array->length % 1024 == 0) {
		void *tmp = realloc(array->players,
		                    (array->length + OFFSET) * sizeof(*player));
		if (!tmp)
			return perror("Reallocating players array");
		array->players = tmp;
	}

	array->players[array->length++] = *player;
}

static int extract_clan_string(char *clan_directory, char *clan)
{
	char *tmp;

	assert(clan_directory != NULL);
	assert(clan != NULL);

	tmp = basename(clan_directory);
	if (strlen(tmp) == MAX_NAME_LENGTH - 1) {
		fprintf(stderr, "%s: Invalid clan directory\n", clan_directory);
		return 0;
	}

	hex_to_string(tmp, clan);
	return 1;
}

static void print_file(char *path)
{
	FILE *file = NULL;
	int c;

	if (!(file = fopen(path, "r")))
		exit(EXIT_FAILURE);

	while ((c = fgetc(file)) != EOF)
		putchar(c);

	fclose(file);
}

static void print_player(struct player *player, char *clan)
{
	assert(player != NULL);
	assert(clan != NULL);

	printf("<tr>");

	if (player->rank == UNKNOWN_RANK)
		printf("<td>?</td>");
	else
		printf("<td>%u</td>", player->rank);

	printf("<td>%s</td><td>%s</td>", player->name, clan);

	if (player->elo == UNKNOWN_ELO)
		printf("<td>?</td>");
	else
		printf("<td>%d</td>", player->elo);

	printf("</tr>\n");
}

static const struct player_array PLAYER_ARRAY_ZERO;

static int cmp_player(const void *p1, const void *p2)
{
	const struct player *a = p1, *b = p2;

	/* We want them in reverse order */
	if (b->rank > a->rank)
		return -1;
	if (b->rank < a->rank)
		return 1;
	return 0;
}

int main(int argc, char **argv)
{
	FILE *file;
	char name[MAX_NAME_LENGTH], clan[MAX_NAME_LENGTH];
	struct player_array array = PLAYER_ARRAY_ZERO;
	unsigned i;

	if (argc != 3) {
		fprintf(stderr, "usage: %s <clan_directory> <players_directory>\n", argv[0]);
		return EXIT_FAILURE;
	}

	players_directory = argv[2];

	if (!extract_clan_string(argv[1], clan))
		return EXIT_FAILURE;

	/* Load players */

	sprintf(path, "%s/%s", argv[1], "members");
	if (!(file = fopen(path, "r")))
		return perror(path), EXIT_FAILURE;

	while (fscanf(file, " %s", name) == 1) {
		struct player player;
		load_player(&player, name);
		add_player(&array, &player);
	}

	fclose(file);

	/* Sort players */
	qsort(array.players, array.length, sizeof(*array.players), cmp_player);

	/* Finally, print them */
	print_file("html/header_clan.inc.html");
	printf("<h2>%s</h2>\n", clan);
	printf("<p>%u member(s)</p>\n", array.length);
	printf("<table><thead><tr><th></th><th>Name</th><th>Clan</th><th>Score</th></tr></thead>\n<tbody>\n");

	for (i = 0; i < array.length; i++)
		print_player(&array.players[i], clan);

	printf("</tbody></table>");
	print_file("html/footer_clan.inc.html");

	return EXIT_SUCCESS;
}
