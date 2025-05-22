#include "algo.h"

#include <limits.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/time.h>

typedef struct
{
  board_vertex **positions;
  size_t size;
} vector;

void vector_create (vector *self)
{
  if (self == NULL)
    return;
  self->positions = NULL;
  self->size = 0;
}

void vector_destroy (vector *self)
{
  if (self == NULL)
    return;
  free (self->positions);
}

void vector_remove_at (vector *self, size_t index)
{
  if (self == NULL || index >= self->size || self->size == 0)
    return;
  for (size_t i = index; i < self->size - 1; i++)
    self->positions[i] = self->positions[i + 1];
  self->size--;
}

void vector_print (vector *self)
{
  if (self == NULL)
    return;
  for (size_t i = 0; i < self->size; i++)
    printf ("%zu\n", self->positions[i]->index);
  fflush (stdout);
}

typedef struct
{
  board b;
  vector cops;
  vector robbers;
  size_t remaining_turn;
  enum role r;
} game;

void game_create (game *self)
{
  if (self == NULL)
    return;
  board_create (&(self->b));
  vector_create (&(self->cops));
  vector_create (&(self->robbers));
  self->remaining_turn = 0;
  self->r = COPS;
}

void game_destroy (game *self)
{
  if (self == NULL)
    return;
  board_destroy (&(self->b));
  vector_destroy (&(self->cops));
  vector_destroy (&(self->robbers));
}

/*
 * Update positions of either cops or robbers and exit if the moves
 * are invalid
 */
void game_update_position (game *self, size_t *new)
{
  vector *current = self->r == COPS ? &(self->robbers) : &(self->cops);
  if (current->positions != NULL)
    {
      // Check if moves are valid
      for (size_t i = 0; i < current->size; i++)
        if (!board_is_valid_move (&(self->b), current->positions[i]->index,
                                  new[i]))
          {
            fprintf (stderr, "New position is invalid\n");
            exit (1);
          }
    }
  else
    current->positions = calloc (current->size, sizeof (*current->positions));
  for (size_t i = 0; i < current->size; i++)
    {
      current->positions[i] = self->b.vertices[new[i]];
    }
}

/*
 * Déplacacement des voleurs
 * On parcourt tous les voisins d’un voleur.
 * Pour chaque voisin, on regarde la distance minimale à tous les gendarmes.
 * On choisit le voisin le plus éloigné des gendarmes (selon la distance
minimale la plus grande).
*/
board_vertex *choose_best_escape_vertex (board *b, board_vertex *current_pos,
                                         const board_vertex **cop_positions,
                                         size_t cop_count)
{
  if (!b || !current_pos || !cop_positions || cop_count == 0)
    return current_pos;

  board_vertex *best = current_pos;
  int max_min_dist = -1;

  // Parcourir les voisins du voleur
  for (size_t i = 0; i < current_pos->degree; i++)
    {
      board_vertex *neighbor = current_pos->neighbors[i];
      int min_dist_to_cop = INT_MAX;

      // Pour chaque gendarme, on regarde la distance entre ce voisin et lui
      for (size_t j = 0; j < cop_count; j++)
        {
          size_t cop_index = cop_positions[j]->index;
          int dist = b->dist[neighbor->index][cop_index];
          if (dist < min_dist_to_cop)
            min_dist_to_cop = dist;
        }

      // On garde le voisin qui maximise la distance minimale au gendarme
      if (min_dist_to_cop > max_min_dist)
        {
          max_min_dist = min_dist_to_cop;
          best = neighbor;
        }
    }

  return best;
}

/*
 * Return the initial or next positions of either the cops or the
 * robbers
 */
vector *game_next_position (game *self)
{
  vector *current = self->r == COPS ? &(self->cops) : &(self->robbers);
  if (current->positions == NULL)       // premier positionnement
    {
      current->positions =
        calloc (current->size, sizeof (*current->positions));
      // Compute initial positions
      for (size_t i = 0; i < current->size; i++)
        {
          if (self->r == COPS)
            {                   // placement des gendarmes
              current->positions[i] = self->b.vertices[i];
            }
          else
            {                   // placement des voleurs
              current->positions[i] = self->b.vertices[self->b.size - i - 1];
            }
        }
    }
  else
    // Compute next positions
    for (size_t i = 0; i < current->size; i++)
      {
        if (self->r == COPS)
          {                     // deplacement des gendarmes
            current->positions[i] =
              self->b.
              vertices[board_next
                       (&(self->b), current->positions[i]->index,
                        self->b.size - i - 1)];
          }
        else
          {                     // deplacement des voleurs
            current->positions[i] =
              choose_best_escape_vertex (&(self->b), current->positions[i],
                                         (const board_vertex **) self->cops.
                                         positions, self->cops.size);
          }
      }
  return current;
}

/*
 * Remove robbers that are on same vertices as cops and return number
 * of remaining robbers (infinite if no cops)
 */
size_t game_capture_robbers (game *self)
{
  if (self->cops.positions == NULL || self->robbers.positions == NULL)
    return UINT_MAX;
  for (size_t i = 0; i < self->robbers.size; i++)
    for (size_t j = 0; j < self->cops.size; j++)
      if (self->robbers.positions[i] == self->cops.positions[j])
        {
          fprintf (stderr, "Captured robber at position %zu\n",
                   self->robbers.positions[i]->index);
          vector_remove_at (&(self->robbers), i);
          return game_capture_robbers (self);
        }
  return self->robbers.size;
}

size_t *read_positions (size_t len)
{
  size_t *pos = calloc (len, sizeof (*pos));
  for (size_t i = 0; i < len; i++)
    {
      char buffer[100];
      char *msg = fgets (buffer, sizeof buffer, stdin);
      if (msg == NULL || sscanf (buffer, "%zu", &pos[i]) != 1)
        {
          fprintf (stderr, "Error while reading new positions\n");
          exit (1);
        }
    }
  return pos;
}

int main (int argc, const char *argv[])
{
  struct timeval t1;
  gettimeofday (&t1, NULL);
  srand (t1.tv_usec * t1.tv_sec);
  // Initialize game
  game g;
  game_create (&g);

  // Initialize data structures
  if (argc != 3)
    {
      fprintf (stderr,
               "Incorrect number of arguments: ./game filename 0/1\n");
      exit (-1);
    }
  FILE *file = fopen (argv[1], "r");
  if (file == NULL)
    {
      fprintf (stderr, "Error opening input file");
      exit (-1);
    }
  bool success = board_read_from (&(g.b), file);
  fclose (file);
  if (!success)
    {
      fprintf (stderr, "Error parsing input file");
      exit (-1);
    }
  g.cops.size = g.b.cops;
  g.robbers.size = g.b.robbers;
  g.r = atoi (argv[2]);
  g.remaining_turn = g.b.max_turn + 2;

  // Play each turn
  enum role turn = COPS;
  while (game_capture_robbers (&g) != 0 && g.remaining_turn != 0)
    {
      if (g.remaining_turn > g.b.max_turn)
        fprintf (stderr, "Initial positions for %s\n",
                 turn == COPS ? "cops" : "robbers");
      else
        fprintf (stderr, "Turn for %s (remaining: %zu)\n",
                 turn == COPS ? "cops" : "robbers", g.remaining_turn);
      if (turn == g.r)
        {
          // This is the turn of this program to find new positions
          vector *pos = game_next_position (&g);
          vector_print (pos);
        }
      else
        {
          // This is the turn of the adversary program to find new
          // positions
          size_t len = g.r == COPS ? g.robbers.size : g.cops.size;
          size_t *pos = read_positions (len);
          game_update_position (&g, pos);
          free (pos);
        }
      turn = turn == COPS ? ROBBERS : COPS;
      g.remaining_turn--;
    }

  // Finalization
  if (g.robbers.size != 0)
    fprintf (stderr, "Robbers win!\n");
  else
    fprintf (stderr, "Cops win!\n");
  game_destroy (&g);
}
