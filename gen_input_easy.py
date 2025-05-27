import matplotlib.pyplot as plt
import numpy as np
import sys

pts = []  # sommets
liens = []
etat = 1  # 1 =sommet, 2=arrete
depart = None  # memoriser premier sommet de l'arrete

img_path = sys.argv[1]
img = plt.imread(img_path)


# transformé une coordonnée en un nombre entre -1 et 1
def norm(x, y, w, h):
    return (x - w / 2) / (w / 2), (h / 2 - y) / (h / 2)


def clic(e):
    global etat, depart

    if e.inaxes is None:  # pas dans l'image
        return

    # recup clic
    x, y = e.xdata, e.ydata

    if etat == 1:
        pts.append((x, y))
        ax.plot(x, y, "ro")  # ro == point rouge
        fig.canvas.draw()

    elif etat == 2:
        i = proche(x, y)
        if i is not None:
            if depart is None:
                depart = i
            elif i != depart:
                liens.append((depart, i))
                x0, y0 = pts[depart]
                x1, y1 = pts[i]
                ax.plot([x0, x1], [y0, y1], "b-")  # trait bleu
                fig.canvas.draw()
                depart = None
        else:
            print("Aucun sommet détecté à cette position...")


# trouver le sommet le plus proche du clic
def proche(x, y, r=10):
    for i, (px, py) in enumerate(pts):
        if np.hypot(px - x, py - y) <= r:
            return i
    return None


def touche(e):
    global etat
    if e.key == "enter":
        if etat == 1 and len(pts) > 1:
            etat = 2
            print("Mode arêtes")

            ax.set_title("Clique sur 2 sommets pour les reliés. Entrée pour continuer.")

            fig.canvas.draw_idle()
        elif etat == 2:

            ax.set_title("Rendez-vous dans le terminal pour terminer")

            fig.canvas.draw_idle()
            print("Fini.")
            sauvegarde()
            plt.close()


def sauvegarde():
    h, w = img.shape[:2]  # recupere largeur hauteur
    norm_pts = [norm(x, y, w, h) for x, y in pts]

    cops = input("Nombre de gendarmes ? ")
    robbers = input("Nombre de voleurs ? ")
    turns = input("Nombre de tours max ? ")

    with open("carte.txt", "w") as f:
        f.write(f"Cops: {cops}\n")
        f.write(f"Robbers: {robbers}\n")
        f.write(f"Max turn: {turns}\n")

        f.write(f"Vertices: {len(pts)}\n")
        for x, y in norm_pts:
            f.write(f"{x:.5f} {y:.5f}\n")

        f.write(f"Edges: {len(liens)}\n")
        for a, b in liens:
            f.write(f"{a} {b}\n")

    print("Fichier carte.txt sauvegardé.")


fig, ax = plt.subplots()
ax.imshow(img)
ax.set_title("Clique pour ajouter les sommets. Entrée pour continuer.")

fig.canvas.mpl_connect("button_press_event", clic)
fig.canvas.mpl_connect("key_press_event", touche)

plt.show()
