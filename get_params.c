int W_DIST_MIN;
int W_DEGREE;
int W_DIST_MOY;
int W_DIST_MOY_WITH_ROBBERS;

FILE *f = fopen("params_place_robbers.txt", "r");
if (!f) {
	perror("fopen");
	exit(EXIT_FAILURE);
}
if (fscanf(f, "%d %d %d %d", &W_DIST_MIN, &W_DEGREE, &W_DIST_MOY,
		   &W_DIST_MOY_WITH_ROBBERS) != 4) {
	fprintf(stderr, "Erreur de lecture des paramètres depuis %s\n",
			"params_place_robbers.txt");
	fclose(f);
	exit(EXIT_FAILURE);
}
fclose(f);
