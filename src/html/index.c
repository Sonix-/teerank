#include <stdio.h>
#include <stdlib.h>
#include <dirent.h>
#include <string.h>
#include <assert.h>
#include <limits.h>
#include <errno.h>

#include "config.h"
#include "player.h"
#include "html.h"

int main(int argc, char **argv)
{
	char name[HEXNAME_LENGTH];

	load_config();

	html_header(&CTF_TAB, "CTF", NULL);
	printf("<table><thead><tr><th></th><th>Name</th><th>Clan</th><th>Score</th></tr></thead><tbody>\n");

	scanf("%*u players");

	while (scanf(" %s", name) == 1) {
		struct player player;

		read_player(&player, name);
		html_print_player(&player, 1);
	}

	printf("</tbody></table>\n");
	html_footer();

	return EXIT_SUCCESS;
}
