#include "algo.h"

#include <limits.h>
#include <stdio.h>
#include <stdlib.h>

#define mu_assert(message, test)                                               \
	do {                                                                       \
		if (!(test))                                                           \
			return message;                                                    \
	} while (0)
#define mu_run_test(test)                                                      \
	do {                                                                       \
		const char *message = test();                                          \
		if (message)                                                           \
			printf("Test %d failed: %s\n", tests_index, message);              \
		else {                                                                 \
			tests_pass++;                                                      \
		}                                                                      \
		tests_run++;                                                           \
	} while (0)
int tests_pass, tests_run, tests_index;

static char *test_board_Floyd_Warshall_chain ()
{
  board b;
  board_create (&b);

  char data[] = "Cops: 1\nRobbers: 1\nMax turn: 1\n"
    "Vertices: 3\n0 0\n0 0\n0 0\n" "Edges: 2\n0 1\n1 2\n";
  FILE *file = tmpfile ();
  fputs (data, file);
  rewind (file);

  bool read = board_read_from (&b, file);
  mu_assert ("error, failure reading board", read == true);
  mu_assert ("error, incorrect distance",
             board_dist (&b, 0, 0) == 0 && board_dist (&b, 0, 1) == 1 &&
             board_dist (&b, 0, 2) == 2 && board_dist (&b, 1, 2) == 1);
  mu_assert ("error, incorrect next vertex",
             board_next (&b, 0, 0) == 0 && board_next (&b, 0, 1) == 1 &&
             board_next (&b, 0, 2) == 1 && board_next (&b, 1, 2) == 2);

  board_destroy (&b);

  return NULL;
}

static char *test_board_single_node ()
{
  board b;
  board_create (&b);

  char data[] = "Cops: 0\nRobbers: 0\nMax turn: 0\n"
    "Vertices: 1\n0 0\n" "Edges: 0\n";
  FILE *file = tmpfile ();
  fputs (data, file);
  rewind (file);

  bool read = board_read_from (&b, file);
  mu_assert ("Lecture échouée (1 nœud)", read == true);
  mu_assert ("Taille incorrecte", b.size == 1);
  mu_assert ("Distance 0->0 invalide", board_dist (&b, 0, 0) == 0);
  mu_assert ("Next 0->0 invalide", board_next (&b, 0, 0) == 0);
  mu_assert ("Déplacement 0->0 doit être autorisé",
             board_is_valid_move (&b, 0, 0));

  board_destroy (&b);
  return NULL;
}

static char *test_board_two_nodes_disconnected ()
{
  board b;
  board_create (&b);

  char data[] = "Cops: 0\nRobbers: 0\nMax turn: 0\n"
    "Vertices: 2\n0 0\n0 0\n" "Edges: 0\n";
  FILE *file = tmpfile ();
  fputs (data, file);
  rewind (file);

  bool read = board_read_from (&b, file);
  mu_assert ("Lecture échouée (2 nœuds)", read == true);
  mu_assert ("Distance 0->1 devrait être INT_MAX",
             board_dist (&b, 0, 1) == (size_t) INT_MAX);
  mu_assert ("Déplacement 0->1 invalide", !board_is_valid_move (&b, 0, 1));

  board_destroy (&b);
  return NULL;
}

static char *test_board_triangle ()
{
  board b;
  board_create (&b);

  char data[] = "Cops: 0\nRobbers: 0\nMax turn: 0\n"
    "Vertices: 3\n0 0\n0 0\n0 0\n" "Edges: 3\n0 1\n1 2\n2 0\n";
  FILE *file = tmpfile ();
  fputs (data, file);
  rewind (file);

  bool read = board_read_from (&b, file);
  mu_assert ("Lecture échouée (triangle)", read == true);

  // Vérification des distances
  for (size_t i = 0; i < 3; i++)
    {
      for (size_t j = 0; j < 3; j++)
        {
          if (i == j)
            {
              mu_assert ("Distance à soi-même incorrecte",
                         board_dist (&b, i, j) == 0);
            }
          else
            {
              mu_assert ("Distance entre voisins incorrecte",
                         board_dist (&b, i, j) == 1);
            }
        }
    }

  // Vérification des next
  mu_assert ("Next 0->1 incorrect", board_next (&b, 0, 1) == 1);
  mu_assert ("Next 0->2 incorrect", board_next (&b, 0, 2) == 2);
  mu_assert ("Next 1->2 incorrect", board_next (&b, 1, 2) == 2);

  board_destroy (&b);
  return NULL;
}

static char *test_board_multiple_paths ()
{
  board b;
  board_create (&b);

  char data[] = "Cops: 0\nRobbers: 0\nMax turn: 0\n"
    "Vertices: 4\n0 0\n0 0\n0 0\n0 0\n" "Edges: 4\n0 1\n0 2\n1 3\n2 3\n";
  FILE *file = tmpfile ();
  fputs (data, file);
  rewind (file);

  bool read = board_read_from (&b, file);
  mu_assert ("Lecture échouée (chemins multiples)", read == true);

  mu_assert ("Distance 0->3 incorrecte", board_dist (&b, 0, 3) == 2);
  mu_assert ("Next 0->3 devrait être 1", board_next (&b, 0, 3) == 1);

  board_destroy (&b);
  return NULL;
}

static char *test_board_invalid_file ()
{
  board b;
  board_create (&b);

  // Fichier sans la section Edges
  char data[] = "Cops: 0\nRobbers: 0\nMax turn: 0\n"
    "Vertices: 2\n0 0\n0 0\n";
  FILE *file = tmpfile ();
  fputs (data, file);
  rewind (file);

  bool read = board_read_from (&b, file);
  mu_assert ("Lecture aurait dû échouer", read == false);

  board_destroy (&b);
  return NULL;
}

static char *test_board_read_null_file ()
{
  board b;
  board_create (&b);
  bool read = board_read_from (&b, NULL);
  mu_assert ("Doit échouer avec FILE* NULL", read == false);
  board_destroy (&b);
  return NULL;
}

static char *test_board_invalid_edge_vertices ()
{
  board b;
  board_create (&b);

  char data[] = "Cops: 0\nRobbers: 0\nMax turn: 0\n" "Vertices: 2\n0 0\n0 0\n" "Edges: 1\n0 5\n";       // Vertex 5 n'existe pas
  FILE *file = tmpfile ();
  fputs (data, file);
  rewind (file);

  bool read = board_read_from (&b, file);
  mu_assert ("Lecture devrait échouer", read == false);
  board_destroy (&b);
  return NULL;
}

static char *test_board_read_from_fewer_edges_than_declared ()
{
  board b;
  board_create (&b);

  char data[] = "Cops: 0\nRobbers: 0\nMax turn: 0\n"
    "Vertices: 3\n0 0\n1 1\n2 2\n" "Edges: 2\n0 1 5\n";
  FILE *file = tmpfile ();
  fputs (data, file);
  rewind (file);

  bool read = board_read_from (&b, file);
  mu_assert
    ("board_read_from devrait échouer si moins d'arêtes sont fournies "
     "que déclarées", read == false);
  board_destroy (&b);
  fclose (file);
  return NULL;
}

static char *test_board_read_from_more_edges_than_declared ()
{
  board b;
  board_create (&b);

  char data[] = "Cops: 0\nRobbers: 0\nMax turn: 0\n" "Vertices: 3\n0 0\n1 1\n2 2\n" "Edges: 1\n0 1 5\n1 2 6\n"; // Deux arêtes au lieu d'une
  FILE *file = tmpfile ();
  fputs (data, file);
  rewind (file);

  bool read = board_read_from (&b, file);
  mu_assert
    ("board_read_from devrait échouer si plus d'arêtes sont fournies "
     "que déclarées", read == false);
  board_destroy (&b);
  fclose (file);
  return NULL;
}

static char *test_board_read_from_edge_with_invalid_vertex ()
{
  board b;
  board_create (&b);

  char data[] = "Cops: 0\nRobbers: 0\nMax turn: 0\n" "Vertices: 3\n0 0\n1 1\n2 2\n" "Edges: 1\n0 3 5\n";        // Sommet 3 n'existe pas
  FILE *file = tmpfile ();
  fputs (data, file);
  rewind (file);

  bool read = board_read_from (&b, file);
  mu_assert
    ("board_read_from devrait échouer si une arête fait référence à "
     "un sommet inexistant", read == false);
  board_destroy (&b);
  fclose (file);
  return NULL;
}

char *(*tests_functions[]) () = { test_board_Floyd_Warshall_chain,
  test_board_single_node,
  test_board_two_nodes_disconnected,
  test_board_triangle,
  test_board_multiple_paths,
  test_board_invalid_file,
  test_board_read_null_file,
  test_board_invalid_edge_vertices,
  test_board_read_from_fewer_edges_than_declared,
  test_board_read_from_more_edges_than_declared,
  test_board_read_from_edge_with_invalid_vertex
};

int main (int argc, const char *argv[])
{
  size_t n = sizeof (tests_functions) / sizeof (tests_functions[0]);
  if (argc == 1)
    {
      for (tests_index = 0; (size_t) tests_index < n; tests_index++)
        mu_run_test (tests_functions[tests_index]);
      if (tests_run == tests_pass)
        printf ("All %d tests passed\n", tests_run);
      else
        printf ("Tests passed/run: %d/%d\n", tests_pass, tests_run);
    }
  else
    {
      tests_index = atoi (argv[1]);
      if (tests_index < 0)
        printf ("%zu\n", n);
      else if ((size_t) tests_index < n)
        {
          mu_run_test (tests_functions[tests_index]);
          if (tests_run == tests_pass)
            printf ("Test %d passed\n", tests_index);
        }
    }
}
