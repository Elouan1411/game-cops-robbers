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
										   board_vertex **cops, size_t ncops,
										   board_vertex **robbers,
										   size_t nrobbers);
static void place_robbers(board *b, board_vertex **out_pos, size_t k,
						  board_vertex **cops, size_t ncops);
static int dist_moy_between_summit_and_all_cops(board *b, board_vertex *v,
												board_vertex **cops,
												size_t ncops);
static int score_pos_cops_for_one_summit(board *b, board_vertex *v,
										 board_vertex **cops, size_t ncops);
static int dist_moy_between_summit_and_all_summits(board *b, board_vertex *v);
static void move_cops(board *b, board_vertex **out_pos, size_t k,
					  board_vertex **robbers, size_t nrobbers);
static bool summit_is_occupied(board_vertex *v, board_vertex **role,
							   size_t nOfRole);
static int score_move_robber_for_one_neighbor(board *b, board_vertex *v,
											  board_vertex **robbers,
											  size_t nrobbers,
											  board_vertex **cops,
											  size_t ncops);
static int is_in_tab(board_vertex **tab, size_t n, board_vertex *v);
static board_vertex *get_best_candidate(board_vertex **candidates,
										size_t *votes, size_t n_candidates,
										board_vertex **cops, size_t ncops,
										board *b);
static board_vertex *get_target(board *b, board_vertex **cops, size_t ncops,
								board_vertex **robbers, size_t nrobbers);
static board_vertex *get_board_vertex_from_index(board *b, size_t index);
static board_vertex *get_2nd_best_neighbor(board *b, board_vertex *start,
										   board_vertex **used_positions,
										   size_t n_used_positions,
										   board_vertex *target);
static int dist_moy_between_summit_and_all_robbers(board *b, board_vertex *v,
												   board_vertex **robbers,
												   size_t nrobbers);

#include <stdarg.h>
void debug(const char *format, ...) {
	FILE *fichier = fopen("debug.log", "a");
	if (fichier == NULL) {
		perror("Erreur lors de l'ouverture du fichier");
		return;
	}

	va_list args;
	va_start(args, format);
	vfprintf(fichier, format, args);
	fprintf(fichier, "\n");
	va_end(args);

	fclose(fichier);
}

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
	for (size_t i = 0; i < self->size; i++) {
		printf("%zu\n", self->positions[i]->index);
	}
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

	// Cas où y a moins de case que de gendarmes
	if (b->size <= k) {
		for (size_t i = 0; i < k; i++) {
			out_pos[i] = b->vertices[i % b->size];
		}
		return;
	}

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

static void place_robbers(board *b, board_vertex **out_pos, size_t k,
						  board_vertex **cops, size_t ncops) {
	/* -- 1) S’assurer que l’on dispose des distances -- */
	if (!b->dist)				 /* dist==NULL → pas encore calculé  */
		board_Floyd_Warshall(b); /* calcule dist[][] et next[][]     */

	// Cas où y a moins de case que de voleurs
	if (b->size <= k) {
		for (size_t i = 0; i < k; i++) {
			out_pos[i] = b->vertices[i % b->size];
		}

		return;
	}

	bool *selected = calloc(b->size, sizeof(bool));

	for (size_t i = 0; i < k; i++) {
		int best_score = INT_MIN;
		int best_idx = -1;

		for (size_t j = 0; j < b->size; j++) {
			if (selected[j])
				continue;

			int score = score_pos_robber_for_one_summit(b, b->vertices[j], cops,
														ncops, out_pos, i);

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

static void move_cops(board *b, board_vertex **cops, size_t ncops,
					  board_vertex **robbers, size_t nrobbers) {
	/* -- 1) S’assurer que l’on dispose des distances -- */
	if (!b->dist) {				 /* dist==NULL → pas encore calculé  */
		board_Floyd_Warshall(b); /* calcule dist[][] et next[][]     */
	}
	// ne pas prendre les gendarmes qui ne peuvent pas bougé
	board_vertex **real_cops = malloc(ncops * sizeof(board_vertex *));
	int *num_real_cops = calloc(ncops, sizeof(int));
	size_t n_real_cops = 0;
	for (size_t i = 0; i < ncops; i++) { // TODO: il faudrait que le dernier
										 // prenne completement un autre chemin
		if (cops[i]->degree > 0) {
			num_real_cops[i] = i;
			real_cops[n_real_cops++] = cops[i];
		} else {
			num_real_cops[i] = -1;
		}
	}

	// Récupérer le voleur cible
	board_vertex *target =
		get_target(b, real_cops, n_real_cops, robbers, nrobbers);

	// tableau qui enregistre les positions pour ne pas que deux gendarmes se
	// retrouve sur la meme case
	board_vertex **used_positions =
		malloc(n_real_cops * sizeof(board_vertex *));
	size_t n_used_positions = 0;

	// Deplacer tout les gendarmes en direction de la cible
	// L'algo essaye de ne pas placer 2 gendarmes sur la meme case
	for (size_t i = 0; i < n_real_cops; i++) {
		size_t index_next = board_next(b, real_cops[i]->index, target->index);
		board_vertex *res = get_board_vertex_from_index(b, index_next);
		if (is_in_tab(used_positions, n_used_positions, res) != -1) {
			// Si la position est deja prise on regarde les voisins et on prend
			// la meilleur
			real_cops[i] = get_2nd_best_neighbor(
				b, real_cops[i], used_positions, n_used_positions, target);
			if (!real_cops[i]) {
				real_cops[i] = res;
			}
		} else {
			real_cops[i] = res;
		}
		used_positions[n_used_positions] = real_cops[i];
		n_used_positions++;
	}

	for (size_t i = 0; i < ncops; i++) {
		if (num_real_cops[i] != -1) {
			cops[num_real_cops[i]] = real_cops[i];
		}
	}

	free(real_cops);
	free(num_real_cops);
	free(used_positions);
}

static void move_robbers(board *b, board_vertex **robbers, size_t nrobbers,
						 board_vertex **cops, size_t ncops) {
	/* -- 1) S’assurer que l’on dispose des distances -- */
	if (!b->dist)				 /* dist==NULL → pas encore calculé  */
		board_Floyd_Warshall(b); /* calcule dist[][] et next[][]     */
	int score = 0;
	// Pour chaque position de gendarmes -> cops[i]
	for (size_t i = 0; i < nrobbers; i++) {
		board_vertex *best_move = NULL;
		int best_score = INT_MIN;

		// on boucle sur les voisins + la case actuelle
		for (size_t j = 0; j < robbers[i]->degree + 1; j++) {
			board_vertex *candidate;

			if (j == robbers[i]->degree) {
				candidate = robbers[i]; // rester sur place
			} else {
				candidate = robbers[i]->neighbors[j];
			}

			score = score_move_robber_for_one_neighbor(b, candidate, robbers,
													   nrobbers, cops, ncops);

			if (score > best_score) {
				best_score = score;
				best_move = candidate;
			}
		}

		robbers[i] = best_move;
	}
}

static board_vertex *get_2nd_best_neighbor(board *b, board_vertex *start,
										   board_vertex **used_positions,
										   size_t n_used_positions,
										   board_vertex *target) {
	// Si pas d'autre possibilité on va quand meme sur la meme case
	if (start->degree == 1) {
		return NULL;
	}

	// Sinon on choisit la case voisine (ou la case start) où la distance avec
	// le gendarme est la plus proche
	board_vertex *best_neighbor = NULL;
	size_t dist_best_neighbor = INT_MAX;
	board_vertex *current = NULL;
	for (size_t i = 0; i < start->degree; i++) {
		current = start->neighbors[i];
		if (is_in_tab(used_positions, n_used_positions, current) != -1) {
			current = start;
		}
		if (board_dist(b, current->index, target->index) < dist_best_neighbor) {
			dist_best_neighbor = board_dist(b, current->index, target->index);
			best_neighbor = current;
		}
	}
	return best_neighbor;
}

static board_vertex *get_board_vertex_from_index(board *b, size_t index) {
	for (size_t i = 0; i < b->size; i++) {
		if (b->vertices[i]->index == index) {
			return b->vertices[i];
		}
	}
	return NULL;
}

// retourne le voleur à prendre pour cible pour tout les gendarmes
// on prend le voleur le plus proche de la majorité des gendarmes
// si yen a plusieurs, on prend le voleur dont la distance moyenne avec tout les
// gendarmes est la plus faible
static board_vertex *get_target(board *b, board_vertex **cops, size_t ncops,
								board_vertex **robbers, size_t nrobbers) {
	// pour chaque gendarme, définir le voleur le plus proche
	// target = le plus proche pour la majorité des gendarmes
	// si égalité, choisir le voleur dont la distance moyenne avec tout les
	// gendarmes est la plus faible

	// Création d'un tableau de voleur candidat
	board_vertex **candidates = malloc(ncops * sizeof(board_vertex *));
	size_t n_candidates = 0;
	size_t *votes = calloc(ncops, sizeof(size_t));

	for (size_t i = 0; i < ncops; i++) { // pour chaque gendarme
		// trouver le voleur le plus proche
		int min_dist = INT_MAX;
		board_vertex *target = NULL;
		for (size_t j = 0; j < nrobbers; j++) {
			int dist = board_dist(b, cops[i]->index, robbers[j]->index);
			if (dist < min_dist) {
				min_dist = dist;
				target = robbers[j];
			}
		}
		// target est le voleur le plus proche du gendarme
		//
		// on compte le nombre de fois que le voleur est le plus proche d'un
		// gendarme on l'ajoute dans le tableau des candidats
		int candidate_index = is_in_tab(candidates, n_candidates, target);
		if (candidate_index == -1) {
			// Si le voleur n'est pas deja dans le tableau
			candidates[n_candidates] = target;
			votes[n_candidates] = 1;
			n_candidates++;
		} else {
			// si le voleur est deja dans le tableau
			votes[candidate_index]++;
		}
	}

	// On récupère le voleur le plus proche de la majorité des gendarmes
	board_vertex *best_candidate =
		get_best_candidate(candidates, votes, n_candidates, cops, ncops, b);

	free(candidates);
	free(votes);

	return best_candidate;
}

// prend en parametre le tableau des candidats et le tableau du nombre de fois
// que chaque candidat est le plus proche retourne le voleur le plus proche de
// la majorité des gendarmes si il y a égalité, on prend le voleur dont la
// distance moyenne avec tout les gendarmes est la plus faible
// Si il y a un seule candidat le plus proche, on le retourne
static board_vertex *get_best_candidate(board_vertex **candidates,
										size_t *votes, size_t n_candidates,
										board_vertex **cops, size_t ncops,
										board *b) {
	// On cherche le voleur le plus proche des gendarmes
	size_t max_count = 0;
	for (size_t i = 0; i < n_candidates; i++) {
		if (votes[i] > max_count) {
			max_count = votes[i];
		}
	}
	// On compte combien de voleurs ont le même score
	// et on les met dans un tableau
	board_vertex **best_candidates =
		malloc(n_candidates * sizeof(board_vertex *));
	size_t n_best_candidates = 0;
	for (size_t i = 0; i < n_candidates; i++) {
		if (votes[i] == max_count) {
			best_candidates[n_best_candidates] = candidates[i];
			n_best_candidates++;
		}
	}

	// On trie les voleurs par distance moyenne avec les gendarmes (si ya
	// plusieurs candidats)
	if (n_best_candidates == 1) {
		board_vertex *final_target = best_candidates[0];
		free(best_candidates);
		return final_target;
	} else {
		int best_avg = INT_MAX;
		board_vertex *best_candidate_avg = NULL;
		for (size_t i = 0; i < n_best_candidates; i++) {
			int avg = dist_moy_between_summit_and_all_cops(
				b, best_candidates[i], cops, ncops);
			if (avg < best_avg) {
				best_avg = avg;
				best_candidate_avg = best_candidates[i];
			}
		}
		free(best_candidates);
		return best_candidate_avg;
	}
}

// retourne -1 si pas dans tab, sinon retourne l'index
static int is_in_tab(board_vertex **tab, size_t n, board_vertex *v) {
	for (size_t i = 0; i < n; i++) {
		if (tab[i]->index == v->index) {
			return i;
		}
	}
	return -1;
}

static int score_move_robber_for_one_neighbor(board *b, board_vertex *v,
											  board_vertex **robbers,
											  size_t nrobbers,
											  board_vertex **cops,
											  size_t ncops) {
	/* Poids (peut etre a ajuster) */
	const int W_DIST_MAX =
		4; // distance_maximale (Éloignement des autres gendarmes déjà placés)
	const int W_DEGREE = 1;	  // mobilité
	const int W_DIST_MOY = 1; // Moyenne des distances vers tous les sommets ()
	const int PENALITY =
		15; // si case deja occupée par un voleur (but = dispersé)

	int dist_min = min_dist_between_summit_and_all_cops(b, v, cops, ncops);
	int degree = v->degree;
	int dist_moy = dist_moy_between_summit_and_all_summits(b, v);
	int penality = summit_is_occupied(v, robbers, nrobbers) ? -PENALITY : 0;

	/* Score pondéré */
	int score = W_DIST_MAX * dist_min + W_DEGREE * degree +
				W_DIST_MOY * dist_moy + penality;

	return score;
}

static int score_pos_cops_for_one_summit(board *b, board_vertex *v,
										 board_vertex **cops, size_t ncops) {
	/* Poids (peut etre a ajuster) */
	const int W_DIST_MAX =
		1; // distance_maximale (Éloignement des autres gendarmes déjà placés)

	const int W_DEGREE = 2;	  // mobilité
	const int W_DIST_MOY = 8; // Moyenne des distances vers tous les sommets

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
										   board_vertex **cops, size_t ncops,
										   board_vertex **robbers,
										   size_t nrobbers) {
	/* Poids (peut etre a ajuster) */
	const int W_DIST_MIN = 5; // distance minimale (le plus important)
	const int W_DEGREE = 2;	  // mobilité / échappatoires
	const int W_DIST_MOY = 6; // distance moyenne (utile si plusieurs gendarmes)
	const int W_DIST_MOY_WITH_ROBBERS = 5; // dispersé les gendarmes

	int dist_min = min_dist_between_summit_and_all_cops(b, v, cops, ncops);
	int degree = v->degree;
	int dist_moy = dist_moy_between_summit_and_all_cops(b, v, cops, ncops);
	int dist_moy_with_robber =
		dist_moy_between_summit_and_all_robbers(b, v, robbers, nrobbers);
	/* Score pondéré */
	int score = W_DIST_MIN * dist_min + W_DEGREE * degree +
				W_DIST_MOY * dist_moy +
				W_DIST_MOY_WITH_ROBBERS * dist_moy_with_robber;

	return score;
}

static int dist_moy_between_summit_and_all_robbers(board *b, board_vertex *v,
												   board_vertex **robbers,
												   size_t nrobbers) {
	return dist_moy_between_summit_and_all_cops(b, v, robbers, nrobbers);
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

static bool summit_is_occupied(board_vertex *v, board_vertex **role,
							   size_t nOfRole) {
	for (size_t i = 0; i < nOfRole; i++) {
		if (v->index == role[i]->index) {
			return true;
		}
	}
	return false;
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
		if (self->r == COPS) { // deplacement des gendarmes
			/* current->positions[i]  */
			move_cops(&(self->b), current->positions, current->size,
					  self->robbers.positions, self->robbers.size);
		} else { // deplacement des voleurs
			move_robbers(&(self->b), current->positions, current->size,
						 self->cops.positions, self->cops.size);
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
