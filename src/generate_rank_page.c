#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <limits.h>
#include <dirent.h>

#include "io.h"

struct player {
	char name[MAX_NAME_LENGTH];
	char clan[MAX_NAME_LENGTH], clan_hex[MAX_NAME_LENGTH];
	int elo;
	unsigned rank;
};

struct page {
	unsigned number, total;

	unsigned length;
	struct player *players;
};

static void add_player(struct page *page, struct player *player)
{
	static const unsigned OFFSET = 1024;

	assert(page != NULL);
	assert(player != NULL);

	if (page->length % OFFSET == 0) {
		struct player *players;
		players = realloc(page->players, (page->length + OFFSET) * sizeof(*player));
		if (!players)
			return perror("Re-allocating page players array");
		page->players = players;
	}

	page->players[page->length++] = *player;
}

static const int UNKNOWN_ELO = INT_MIN;
static const unsigned UNKNOWN_RANK = UINT_MAX;

static char *root;

static void load_player(struct player *player, char *name)
{
	static char path[PATH_MAX];

	assert(player != NULL);

	/* Name */
	hex_to_string(name, player->name);

	/* Clan */
	sprintf(path, "%s/%s/%s", root, name, "clan");
	if (read_file(path, "%s", player->clan_hex) == 1)
		hex_to_string(player->clan_hex, player->clan);
	else
		player->clan[0] = '\0';

	/* Elo */
	sprintf(path, "%s/%s/%s", root, name, "elo");
	if (read_file(path, "%d", &player->elo) != 1)
		player->elo = UNKNOWN_ELO;

	/* Rank */
	sprintf(path, "%s/%s/%s", root, name, "rank");
	if (read_file(path, "%u", &player->rank) != 1)
		player->rank = UNKNOWN_RANK;
}

static void load_page(struct page *page)
{
	char name[MAX_NAME_LENGTH];

	assert(page != NULL);

	if (scanf("page %u/%u ", &page->number, &page->total) != 2) {
		fprintf(stderr, "Cannot match page header\n");
		exit(EXIT_FAILURE);
	}

	while (scanf("%s", name) == 1) {
		struct player player;
		load_player(&player, name);
		add_player(page, &player);
	}
}

static void print_page(struct page *page)
{
	unsigned i;

	assert(page != NULL);

	for (i = 0; i < page->length; i++) {
		struct player *player = &page->players[i];

		printf("<tr>");

		/* Rank */
		if (player->rank == UNKNOWN_RANK)
			printf("<td>?</td>");
		else
			printf("<td>%u</td>", player->rank);

		/* Name */
		printf("<td>");
		html(player->name);
		printf("</td>");

		/* Clan */
		printf("<td>");
		if (player->clan[0] != '\0') {
			printf("<a href=\"/clans/%s.html\">", player->clan_hex);
			html(player->clan);
			printf("</a>");
		}
		printf("</td>");

		/* Elo */
		if (player->elo == UNKNOWN_ELO)
			printf("<td>?</td>");
		else
			printf("<td>%d</td>", player->elo);

		printf("</tr>\n");
	}
}

static unsigned min(unsigned a, unsigned b)
{
	return (a < b) ? a : b;
}

static void print_nav(struct page *page)
{
	/* Number of pages shown before and after the current page */
	static const unsigned PAGE_SHOWN = 3;
	unsigned i;

	assert(page != NULL);

	printf("<nav class=\"pages\">");
	if (page->number == 1)
		printf("<a class=\"previous\">Previous</a>");
	else
		printf("<a class=\"previous\" href=\"/pages/%u.html\">Previous</a>",
		       page->number - 1);

	if (page->number > PAGE_SHOWN + 1)
		printf("<a href=\"/pages/1.html\">1</a>");
	if (page->number > PAGE_SHOWN + 2)
		printf("<span>...</span>");

	for (i = min(PAGE_SHOWN, page->number - 1); i > 0; i--)
		printf("<a href=\"/pages/%u.html\">%u</a>",
		       page->number - i, page->number - i);

	printf("<a class=\"current\">%u</a>", page->number);

	for (i = 1; i <= min(PAGE_SHOWN, page->total - page->number); i++)
		printf("<a href=\"/pages/%u.html\">%u</a>",
		       page->number + i, page->number + i);

	if (page->number + PAGE_SHOWN + 1 < page->total)
		printf("<span>...</span>");
	if (page->number + PAGE_SHOWN < page->total)
		printf("<a href=\"/pages/%u.html\">%u</a>",
		       page->total, page->total);

	if (page->number == page->total)
	printf("<a class=\"next\">Next</a>");
	else
		printf("<a class=\"next\" href=\"/pages/%u.html\">Next</a>",
		       page->number + 1);
	printf("</nav>");
}

static const struct page PAGE_ZERO;
enum mode {
	FULL_PAGE, ONLY_ROWS
};

int main(int argc, char **argv)
{
	struct page page = PAGE_ZERO;
	enum mode mode;

	if (argc != 3) {
		fprintf(stderr, "usage: %s [full-page|only-rows] <players_directory>\n", argv[0]);
		return EXIT_FAILURE;
	}

	root = argv[2];
	if (!strcmp(argv[1], "full-page"))
		mode = FULL_PAGE;
	else if (!strcmp(argv[1], "only-rows"))
		mode = ONLY_ROWS;
	else {
		fprintf(stderr, "First argument must be either \"full-page\" or \"only-rows\"\n");
		return EXIT_FAILURE;
	}

	load_page(&page);

	if (mode == FULL_PAGE) {
		print_header();
		print_nav(&page);

		printf("<table><thead><tr><th></th><th>Name</th><th>Clan</th><th>Score</th></tr></thead><tbody>\n");
	}

	print_page(&page);

	if (mode == FULL_PAGE) {
		printf("</tbody></table>");
		print_footer();
	}

	return EXIT_SUCCESS;
}
