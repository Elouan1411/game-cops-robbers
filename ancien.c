#include "algo.h"

#include <limits.h>
#include <stdlib.h>

void board_create (board *self)
{
  if (!self)
    return;
  self->size = 0;
  self->vertices = NULL;
  self->cops = 0;
  self->robbers = 0;
  self->max_turn = 0;
  self->dist = NULL;
  self->next = NULL;
}

/*
 * Auxiliary function to factorize code
 */
void board_add_edge_uni (board_vertex *source, board_vertex *destination)
{
  source->degree++;
  source->neighbors = realloc (source->neighbors,
                               source->degree *
                               sizeof (*(source->neighbors)));
  source->neighbors[source->degree - 1] = destination;
}

bool board_read_from (board *self, FILE *file)
{
  if (!self || !file)
    return false;
  char line[128];

  if (!fgets (line, sizeof (line), file))
    return false;
  sscanf (line, "Cops: %zu", &(self->cops));

  if (!fgets (line, sizeof (line), file))
    return false;
  sscanf (line, "Robbers: %zu", &(self->robbers));

  if (!fgets (line, sizeof (line), file))
    return false;
  sscanf (line, "Max turn: %zu", &(self->max_turn));

  if (!fgets (line, sizeof (line), file))
    return false;
  sscanf (line, "Vertices: %zu", &(self->size));

  self->vertices = calloc (self->size, sizeof (*self->vertices));
  if (!self->vertices)
    return false;

  for (size_t i = 0; i < self->size; i++)
    {
      self->vertices[i] = calloc (1, sizeof (**self->vertices));
      if (!self->vertices[i])
        return false;
      self->vertices[i]->index = i;
      self->vertices[i]->degree = 0;
      self->vertices[i]->neighbors = NULL;

      if (!fgets (line, sizeof (line), file))
        return false;
    }

  size_t edges = 0;
  if (!fgets (line, sizeof (line), file))
    return false;
  sscanf (line, "Edges: %zu", &edges);

  for (size_t i = 0; i < edges; i++)
    {
      size_t v1, v2;
      if (!fgets (line, sizeof (line), file))
        return false;
      if (sscanf (line, "%zu %zu", &v1, &v2) != 2 || v1 >= self->size ||
          v2 >= self->size)
        {
          return false;
        }
      // Appeler deux fois pour faire dans les deux sens
      board_add_edge_uni (self->vertices[v1], self->vertices[v2]);
      board_add_edge_uni (self->vertices[v2], self->vertices[v1]);
    }

  if (fgets (line, sizeof (line), file))
    {
      size_t dummy1, dummy2;
      if (sscanf (line, "%zu %zu", &dummy1, &dummy2) == 2)
        {
          // Une arête supplémentaire a été détectée
          return false;
        }
    }

  return true;
}

void board_destroy (board *self)
{
  if (!self)
    return;
  for (size_t i = 0; i < self->size; i++)
    {
      /* for (size_t j = 0; j < self->vertices[i]->degree; j++) { */
      /*   free(self->vertices[i]->neighbors[j]); */
      /* } */
      free (self->vertices[i]->optim);
      free (self->vertices[i]->neighbors);
      free (self->vertices[i]);
    }
  free (self->vertices);
  if (self->dist)
    {
      for (size_t i = 0; i < self->size; i++)
        free (self->dist[i]);
      free (self->dist);
    }

  if (self->next)
    {
      for (size_t i = 0; i < self->size; i++)
        free (self->next[i]);
      free (self->next);
    }
}

bool board_is_valid_move (board *self, size_t source, size_t dest)
{
  if (!self)
    return false;
  if (source >= self->size || dest >= self->size)
    return false;
  if (source == dest)
    {
      return true;
    }
  for (size_t i = 0; i < self->vertices[source]->degree; i++)
    {
      if (self->vertices[source]->neighbors[i]->index == dest)
        {
          return true;
        }
    }
  return false;
}

void board_Floyd_Warshall (board *self)
{
  if (!self || self->size == 0)
    return;
  self->dist = malloc (self->size * sizeof (*self->dist));
  self->next = malloc (self->size * sizeof (*self->next));
  for (size_t u = 0; u < self->size; u++)
    {
      self->dist[u] = malloc (self->size * sizeof (**self->dist));
      self->next[u] = malloc (self->size * sizeof (**self->next));
      for (size_t v = 0; v < self->size; v++)
        {
          self->dist[u][v] = INT_MAX;
          self->next[u][v] = INT_MAX;
        }
    }

  // Initialiser les distances pour les voisins directs
  for (size_t u = 0; u < self->size; u++)
    {
      for (size_t i = 0; i < self->vertices[u]->degree; i++)
        {
          size_t v = self->vertices[u]->neighbors[i]->index;    // Index du voisin
          self->dist[u][v] = 1; // car il y a une arête entre u et v
          self->next[u][v] = v; // Le prochain sommet est v
        }
    }

  for (size_t v = 0; v < self->size; v++)
    {
      self->dist[v][v] = 0;     // Distance de chaque sommet à lui-même = 0
      self->next[v][v] = v;     // Le prochain sommet est lui-même
    }

  for (size_t w = 0; w < self->size; w++)
    {
      for (size_t u = 0; u < self->size; u++)
        {
          for (size_t v = 0; v < self->size; v++)
            {
              if (self->dist[u][v] > self->dist[u][w] + self->dist[w][v])
                {
                  self->dist[u][v] = self->dist[u][w] + self->dist[w][v];
                  self->next[u][v] = self->next[u][w];
                }
            }
        }
    }
}

size_t board_dist (board *self, size_t source, size_t dest)
{
  /* if (!self || self->size == 0) */
  /*      return -1; */
  /* if (source >= self->size || dest >= self->size) */
  /*      return -1; */
  /* if (self->dist == NULL || self->next == NULL) */
  /*      return -1; */
  if (!self->dist)
    {
      board_Floyd_Warshall (self);
    }
  /* if (self->dist[source][dest] == UINT_MAX) { */
  /*      return -1; */
  /* } */
  return self->dist[source][dest];
}

size_t board_next (board *self, size_t source, size_t dest)
{
  /* if (!self || self->size == 0) */
  /*      return -1; */
  /* if (source >= self->size || dest >= self->size) */
  /*      return -1; */
  /* if (self->dist == NULL || self->next == NULL) */
  /*      return -1; */
  if (!self->next)
    {
      board_Floyd_Warshall (self);
    }
  /* if (self->next[source][dest] == UINT_MAX) { */
  /*      return -1; */
  /* } */
  return self->next[source][dest];
}
