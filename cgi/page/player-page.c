#include <stdlib.h>
#include <stdio.h>
#include <assert.h>
#include <string.h>

#include "config.h"
#include "html.h"
#include "player.h"

int main(int argc, char **argv)
{
	char name[HEXNAME_LENGTH], clan[NAME_LENGTH];
	const char *hexname;
	struct player player;
	int player_found;

	load_config();
	if (argc != 2) {
		fprintf(stderr, "usage: %s <player_name>\n", argv[0]);
		return EXIT_FAILURE;
	}

	hexname = argv[1];
	if (!is_valid_hexname(hexname)) {
		fprintf(stderr, "%s: Not a valid player name\n", hexname);
		return EXIT_NOT_FOUND;
	}

	player_found = player_exist(hexname);

	if (player_found) {
		init_player(&player);
		player_found = read_player(&player, hexname);
	}

	hexname_to_name(hexname, name);
	CUSTOM_TAB.name = name;
	CUSTOM_TAB.href = "";
	html_header(&CUSTOM_TAB, name, NULL);

	/* Print player logo, name, clan, rank and elo */
	if (player_found) {
		hexname_to_name(player.clan, clan);
		html("<header id=\"player_header\">");
		html("<img src=\"/images/player.png\"/>");
		html("<section>");
		html("<h1>%s</h1>", name);
		html("<p>%s</p>", clan);
		html("</section>");
		html("<p>#%u (%d ELO)</p>", player.rank, player.elo);
		html("</header>");
		html("");
		html("<h2>Historic</h2>");
		html("<object data=\"/players/%s/elo+rank.svg\" type=\"image/svg+xml\"></object>",
		     player.name);
	} else {
		html("<p>Player not found</p>");
	}

	html_footer();

	return EXIT_SUCCESS;
}
