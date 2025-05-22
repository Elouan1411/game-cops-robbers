#include "algo.h"

#include <limits.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/time.h>

typedef struct {
	board_vertex **positions;
	size_t size;
} vector;

static int min_dist_between_summit_and_all_cops(board *b, board_vertex *v,
												board_vertex **cops,
												size_t ncops);
static void place_cops(board *b, board_vertex **out_pos, size_t k);
static int score_pos_robber_for_one_summit(board *b, board_vertex *v,
										   board_vertex **cops, size_t ncops);
static void place_robbers(board *b, board_vertex **out_pos, size_t k,
						  board_vertex **cops, size_t ncops);
static int dist_moy_between_summit_and_all_cops(board *b, board_vertex *v,
												board_vertex **cops,
												size_t ncops);
static int score_pos_cops_for_one_summit(board *b, board_vertex *v,
										 board_vertex **cops, size_t ncops);
static int dist_moy_between_summit_and_all_summits(board *b, board_vertex *v);

// Structure pour stocker une valeur et son indice
typedef struct {
	int value;
	int index;
} IndexedValue;

// Comparateur décroissant (plus grande valeur d'abord)
int compare_desc(const void *a, const void *b) {
	IndexedValue *ia = (IndexedValue *)a;
	IndexedValue *ib = (IndexedValue *)b;
	return ib->value - ia->value;
}

// Fonction qui donne k indexes dont les valeurs sont les plus grandes
int *top_k_indices(const int *tab, size_t n, size_t k) {
	if (k > n)
		return NULL; // sécurité

	// 1. Construire un tableau de structs avec valeur + index
	IndexedValue *indexed = malloc(n * sizeof(IndexedValue));
	for (size_t i = 0; i < n; ++i) {
		indexed[i].value = tab[i];
		indexed[i].index = i;
	}

	// 2. Trier en ordre décroissant
	qsort(indexed, n, sizeof(IndexedValue), compare_desc);

	// 3. Extraire les k meilleurs indices
	int *result = malloc(k * sizeof(int));
	for (size_t i = 0; i < k; ++i) {
		result[i] = indexed[i].index;
	}

	free(indexed);
	return result;
}

void vector_create(vector *self) {
	if (self == NULL)
		return;
	self->positions = NULL;
	self->size = 0;
}

void vector_destroy(vector *self) {
	if (self == NULL)
		return;
	free(self->positions);
}

void vector_remove_at(vector *self, size_t index) {
	if (self == NULL || index >= self->size || self->size == 0)
		return;
	for (size_t i = index; i < self->size - 1; i++)
		self->positions[i] = self->positions[i + 1];
	self->size--;
}

void vector_print(vector *self) {
	if (self == NULL)
		return;
	for (size_t i = 0; i < self->size; i++)
		printf("%zu\n", self->positions[i]->index);
	fflush(stdout);
}

typedef struct {
	board b;
	vector cops;
	vector robbers;
	size_t remaining_turn;
	enum role r;
} game;

void game_create(game *self) {
	if (self == NULL)
		return;
	board_create(&(self->b));
	vector_create(&(self->cops));
	vector_create(&(self->robbers));
	self->remaining_turn = 0;
	self->r = COPS;
}

void game_destroy(game *self) {
	if (self == NULL)
		return;
	board_destroy(&(self->b));
	vector_destroy(&(self->cops));
	vector_destroy(&(self->robbers));
}

/*
 * Update positions of either cops or robbers and exit if the moves
 * are invalid
 */
void game_update_position(game *self, size_t *new) {
	vector *current = self->r == COPS ? &(self->robbers) : &(self->cops);
	if (current->positions != NULL) {
		// Check if moves are valid
		for (size_t i = 0; i < current->size; i++)
			if (!board_is_valid_move(&(self->b), current->positions[i]->index,
									 new[i])) {
				fprintf(stderr, "New position is invalid\n");
				exit(1);
			}
	} else
		current->positions = calloc(current->size, sizeof(*current->positions));
	for (size_t i = 0; i < current->size; i++) {
		current->positions[i] = self->b.vertices[new[i]];
	}
}

/* ---------------------------------------------------------------------
 *  Dispersion maximale : choisir k sommets mutuellement éloignés (mais en
 * stratégie gloutonne)
 * b          : pointeur vers le plateau (board déjà initialisé)
 * out_pos[]  : tableau (déjà alloué) qui recevra les k positions
 * k : nombre de gendarmes à placer
 * -------------------------------------------------------------------*/
static void place_cops(board *b, board_vertex **out_pos, size_t k) {
	if (!b->dist)
		board_Floyd_Warshall(b);

	// Tableau pour suivre les sommets déjà sélectionnés
	bool *selected = calloc(b->size, sizeof(bool));

	for (size_t i = 0; i < k; i++) {
		int best_score = INT_MIN;
		int best_idx = -1;

		for (size_t j = 0; j < b->size; j++) {
			if (selected[j])
				continue;

			int score =
				score_pos_cops_for_one_summit(b, b->vertices[j], out_pos, i);
			if (score > best_score) {
				best_score = score;
				best_idx = j;
			}
		}

		if (best_idx != -1) {
			out_pos[i] = b->vertices[best_idx];
			selected[best_idx] = true;
		}
	}
	free(selected);
}
// TODO: faire une seule fonction pour les deux
static void place_robbers(board *b, board_vertex **out_pos, size_t k,
						  board_vertex **cops, size_t ncops) {
	/* -- 1) S’assurer que l’on dispose des distances -- */
	if (!b->dist)				 /* dist==NULL → pas encore calculé  */
		board_Floyd_Warshall(b); /* calcule dist[][] et next[][]     */

	int *scores = calloc(b->size, sizeof(int));
	for (size_t i = 0; i < b->size; i++) {
		scores[i] =
			score_pos_robber_for_one_summit(b, b->vertices[i], cops, ncops);
	}
	/* et garder les k meilleurs*/
	int *index_best = top_k_indices(scores, b->size, k);
	// remplir le tableau de sortie
	for (size_t i = 0; i < k; i++) {
		out_pos[i] = b->vertices[index_best[i]];
	}
	free(index_best);
	free(scores);
}

static int score_pos_cops_for_one_summit(board *b, board_vertex *v,
										 board_vertex **cops, size_t ncops) {
	/* Poids (peut etre a ajuster) */
	const int W_DIST_MAX =
		7; // distance_maximale (Éloignement des autres gendarmes déjà placés)
	const int W_DEGREE = 3;	  // mobilité
	const int W_DIST_MOY = 2; // Moyenne des distances vers tous les sommets

	int dist_min = min_dist_between_summit_and_all_cops(b, v, cops, ncops);
	int degree = v->degree;
	int dist_moy = dist_moy_between_summit_and_all_summits(b, v);

	/* Score pondéré */
	int score =
		W_DIST_MAX * dist_min + W_DEGREE * degree - W_DIST_MOY * dist_moy;

	return score;
}

/* ---------------------------------------------------------------------
 *  Calculer le score (haut = voleur en sécurité) d'une position
 *  b          : plateau de jeu (déjà initialisé)
 *  v          : sommet du voleur
 *  cops[]     : tableau des positions des gendarmes
 *  ncops      : nombre de gendarmes
 * -------------------------------------------------------------------*/
static int score_pos_robber_for_one_summit(board *b, board_vertex *v,
										   board_vertex **cops, size_t ncops) {
	/* Poids (peut etre a ajuster) */
	const int W_DIST_MIN = 5; // distance minimale (le plus important)
	const int W_DEGREE = 2;	  // mobilité / échappatoires
	const int W_DIST_MOY = 1; // distance moyenne (utile si plusieurs gendarmes)

	int dist_min = min_dist_between_summit_and_all_cops(b, v, cops, ncops);
	int degree = v->degree;
	int dist_moy = dist_moy_between_summit_and_all_cops(b, v, cops, ncops);
	/* Score pondéré */
	int score =
		W_DIST_MIN * dist_min + W_DEGREE * degree + W_DIST_MOY * dist_moy;

	return score;
}

static int min_dist_between_summit_and_all_cops(board *b, board_vertex *v,
												board_vertex **cops,
												size_t ncops) {
	if (ncops == 0) {
		return INT_MAX;
	}
	int min_dist = INT_MAX;
	for (size_t j = 0; j < ncops; j++) {
		int d = b->dist[v->index][cops[j]->index];
		if (d < min_dist) {
			min_dist = d;
		}
	}
	return min_dist;
}

static int dist_moy_between_summit_and_all_summits(board *b, board_vertex *v) {
	return dist_moy_between_summit_and_all_cops(b, v, b->vertices, b->size);
}

static int dist_moy_between_summit_and_all_cops(board *b, board_vertex *v,
												board_vertex **cops,
												size_t ncops) {
	int total_dist = 0;
	for (size_t j = 0; j < ncops; j++) {
		total_dist += b->dist[v->index][cops[j]->index];
	}
	return total_dist / ncops;
}

/*
 * Return the initial or next positions of either the cops or the
 * robbers
 */
vector *game_next_position(game *self) {
	vector *current = self->r == COPS ? &(self->cops) : &(self->robbers);
	if (current->positions == NULL) // premier positionnement
	{
		current->positions = calloc(current->size, sizeof(*current->positions));

		// Compute initial positions
		if (self->r == COPS) { /* placement (ou repositionnement) gendarmes */
			place_cops(&(self->b), current->positions, current->size);
		} else {
			/* placement des voleurs */
			place_robbers(&(self->b), current->positions, current->size,
						  self->cops.positions, self->cops.size);
		}

	} else
		// Compute next positions
		for (size_t i = 0; i < current->size; i++) {
			if (self->r == COPS) { // deplacement des gendarmes
				current->positions[i] = self->b.vertices[board_next(
					&(self->b), current->positions[i]->index,
					self->b.size - i - 1)];
			} else { // deplacement des voleurs
				current->positions[i] = self->b.vertices[board_next(
					&(self->b), current->positions[i]->index, i)];
			}
		}
	return current;
}

/*
 * Remove robbers that are on same vertices as cops and return number
 * of remaining robbers (infinite if no cops)
 */
size_t game_capture_robbers(game *self) {
	if (self->cops.positions == NULL || self->robbers.positions == NULL)
		return UINT_MAX;
	for (size_t i = 0; i < self->robbers.size; i++)
		for (size_t j = 0; j < self->cops.size; j++)
			if (self->robbers.positions[i] == self->cops.positions[j]) {
				fprintf(stderr, "Captured robber at position %zu\n",
						self->robbers.positions[i]->index);
				vector_remove_at(&(self->robbers), i);
				return game_capture_robbers(self);
			}
	return self->robbers.size;
}

size_t *read_positions(size_t len) {
	size_t *pos = calloc(len, sizeof(*pos));
	for (size_t i = 0; i < len; i++) {
		char buffer[100];
		char *msg = fgets(buffer, sizeof buffer, stdin);
		if (msg == NULL || sscanf(buffer, "%zu", &pos[i]) != 1) {
			fprintf(stderr, "Error while reading new positions\n");
			exit(1);
		}
	}
	return pos;
}

int main(int argc, const char *argv[]) {
	struct timeval t1;
	gettimeofday(&t1, NULL);
	srand(t1.tv_usec * t1.tv_sec);
	// Initialize game
	game g;
	game_create(&g);

	// Initialize data structures
	if (argc != 3) {
		fprintf(stderr, "Incorrect number of arguments: ./game filename 0/1\n");
		exit(-1);
	}
	FILE *file = fopen(argv[1], "r");
	if (file == NULL) {
		fprintf(stderr, "Error opening input file");
		exit(-1);
	}
	bool success = board_read_from(&(g.b), file);
	fclose(file);
	if (!success) {
		fprintf(stderr, "Error parsing input file");
		exit(-1);
	}
	g.cops.size = g.b.cops;
	g.robbers.size = g.b.robbers;
	g.r = atoi(argv[2]);
	g.remaining_turn = g.b.max_turn + 2;

	// Play each turn
	enum role turn = COPS;
	while (game_capture_robbers(&g) != 0 && g.remaining_turn != 0) {
		if (g.remaining_turn > g.b.max_turn)
			fprintf(stderr, "Initial positions for %s\n",
					turn == COPS ? "cops" : "robbers");
		else
			fprintf(stderr, "Turn for %s (remaining: %zu)\n",
					turn == COPS ? "cops" : "robbers", g.remaining_turn);
		if (turn == g.r) {
			// This is the turn of this program to find new positions
			vector *pos = game_next_position(&g);
			vector_print(pos);
		} else {
			// This is the turn of the adversary program to find new
			// positions
			size_t len = g.r == COPS ? g.robbers.size : g.cops.size;
			size_t *pos = read_positions(len);
			game_update_position(&g, pos);
			free(pos);
		}
		turn = turn == COPS ? ROBBERS : COPS;
		g.remaining_turn--;
	}

	// Finalization
	if (g.robbers.size != 0)
		fprintf(stderr, "Robbers win!\n");
	else
		fprintf(stderr, "Cops win!\n");
	game_destroy(&g);
}
