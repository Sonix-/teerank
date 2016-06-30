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
	struct player player;
	int player_found;

	load_config();
	if (argc != 2) {
		fprintf(stderr, "usage: %s <player_name>\n", argv[0]);
		return EXIT_FAILURE;
	}

	if (!is_valid_hexname(argv[1]))
		return EXIT_NOT_FOUND;

	init_player(&player);
	player_found = read_player(&player, argv[1]);

	hexname_to_name(argv[1], name);
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
		html("<h2>ELO points (%d)</h2>", player.elo);
		html("<object data=\"/players/%s/elo.svg\" type=\"image/svg+xml\"></object>", player.name);
		html("");
		html("<h2>Rank (%u)</h2>", player.rank);
		html("<object data=\"/players/%s/rank.svg\" type=\"image/svg+xml\"></object>", player.name);
	} else {
		html("<p>Player not found</p>");
	}

	html_footer();

	return EXIT_SUCCESS;
}
