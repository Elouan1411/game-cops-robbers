import matplotlib.pyplot as plt
import numpy as np


verts_pix = []  # Coordonnées pixel des sommets [(x,y), ...]
edges = []
mode = 1
current_edge_start = None

image_path = "cartes/carte_campus.png"
path_dest = "carte.png"


def normalize_coords(x, y, width, height):
    # Normaliser entre -1 et 1 avec origine au centre
    nx = (x - width / 2) / (width / 2)
    ny = (height / 2 - y) / (height / 2)  # y inversé car pixels (0 en haut)
    return nx, ny


def onclick(event):
    global mode, verts_pix, edges, current_edge_start

    if event.inaxes is None:
        return

    x, y = event.xdata, event.ydata

    if mode == 1:
        # Ajout sommet
        verts_pix.append((x, y))
        ax.plot(x, y, "ro")
        fig.canvas.draw()

    elif mode == 2:
        # Ajout arêtes par paire de clics
        if current_edge_start is None:
            # Premier clic -> on sélectionne le sommet de départ
            idx = find_closest_vertex(x, y)
            if idx is not None:
                current_edge_start = idx
                print(f"Arête début sur sommet {idx}")
        else:
            # Deuxième clic -> sommet d'arrivée
            idx = find_closest_vertex(x, y)
            if idx is not None and idx != current_edge_start:
                edges.append((current_edge_start, idx))
                # Tracer l'arête
                x0, y0 = verts_pix[current_edge_start]
                x1, y1 = verts_pix[idx]
                (line,) = ax.plot([x0, x1], [y0, y1], "b-")
                fig.canvas.draw()
                print(f"Arête ajoutée entre {current_edge_start} et {idx}")
            current_edge_start = None


def find_closest_vertex(x, y, threshold=10):
    # Trouve le sommet le plus proche dans un rayon threshold pixels
    min_dist = float("inf")
    min_idx = None
    for i, (vx, vy) in enumerate(verts_pix):
        dist = np.hypot(vx - x, vy - y)
        if dist < min_dist and dist <= threshold:
            min_dist = dist
            min_idx = i
    return min_idx


def on_key(event):
    global mode
    if event.key == "enter":
        if mode == 1 and len(verts_pix) > 1:
            mode = 2
            print("Mode arêtes activé : cliquez deux fois pour créer une arête")
        elif mode == 2:
            print("Fin de l'édition. Sauvegarde en cours...")
            save_to_file()
            plt.close()


def save_to_file():
    width, height = img.shape[1], img.shape[0]
    # Normaliser coordonnées entre -1 et 1
    coords_norm = [normalize_coords(x, y, width, height) for (x, y) in verts_pix]

    with open(path_dest, "w") as f:
        # Exemple de valeurs fixes pour le header
        f.write("Cops: 3\n")
        f.write("Robbers: 3\n")
        f.write("Max turn: 100\n")

        f.write(f"Vertices: {len(verts_pix)}\n")
        for nx, ny in coords_norm:
            f.write(f"{nx:.5f} {ny:.5f}\n")

        f.write(f"Edges: {len(edges)}\n")
        for i1, i2 in edges:
            f.write(f"{i1} {i2}\n")

    print(f"Fichier sauvegardé sous '{path_dest}'")


# Chargement image
img = plt.imread(image_path)

fig, ax = plt.subplots()
ax.imshow(img)
ax.set_title("Mode 1: Cliquez pour ajouter les sommets\nAppuyez sur Entrée quand fini")

fig.canvas.mpl_connect("button_press_event", onclick)
fig.canvas.mpl_connect("key_press_event", on_key)

plt.show()
