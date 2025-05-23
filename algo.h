#ifndef ALGO_H
#define ALGO_H

#include <stdbool.h>
#include <stdio.h>

enum role { COPS, ROBBERS };

typedef struct sboard_vertex {
  size_t index;
  size_t degree;
  struct sboard_vertex **neighbors;
  bool *optim;
} board_vertex;

typedef struct {
  size_t size;
  board_vertex **vertices;
  size_t cops;
  size_t robbers;
  size_t max_turn;
  unsigned int **dist;
  size_t **next;
} board;

/*
 * Create an empty board by initializing each member
 */
void board_create(board *self);

/*
 * Create board from parsing a file and return false if file is
 * incorrect
 */
bool board_read_from(board *self, FILE *file);

/*
 * Destroy a board by freeing all memory used by its members
 */
void board_destroy(board *self);

/*
 * Check if there is an edge between source and destination
 */
bool board_is_valid_move(board *self, size_t source, size_t dest);

/*
 * Floyd-Warshall algorithm to determine the smallest number of edges
 * from any vertex to any other vertex
 */
void board_Floyd_Warshall(board *self);

/*
 * Return shortest number of edges between vertex source and vertex
 * dest
 */
size_t board_dist(board *self, size_t source, size_t dest);

/*
 * Return next vertex on shortest path from vertex source to vertex
 * dest
 */
size_t board_next(board *self, size_t source, size_t dest);

#endif // ALGO_H
