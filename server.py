from subprocess import PIPE, DEVNULL, Popen
import multiprocessing
import re
from turtle import *
import sys


class Positions:
    def __init__(self, length, stdout, stdin, game):
        self.positions = [-1] * length
        self.stdout = stdout
        self.stdin = stdin
        self.game = game
        self.turtles = None

    def remove_captured(self):
        if self.positions[0] == -1:
            return
        positions, turtles = [], []
        for i, pos in enumerate(self.positions):
            if pos not in self.game.cops.positions:
                positions.append(pos)
                if self.game.graphic:
                    turtles.append(self.turtles[i])
            else:
                print("Captured robber at position " + str(pos))
                if self.game.graphic:
                    self.turtles[i].pendown()
                    self.turtles[i].color("green")
                    self.turtles[i].shape("square")
                    self.turtles[i].setposition(
                        [x + 0.1 for x in self.game.layout[pos]]
                    )
        self.positions = positions
        self.turtles = turtles

    def read_positions(self, return_vector):
        try:
            new = [int(self.stdout.readline()) for _ in range(len(return_vector))]
        except Exception as e:
            print(f"Error while parsing answer: {e}")
            return_vector[0] = None
            return
        for i in range(len(return_vector)):
            return_vector[i] = new[i]

    def new_positions(self):
        return_vector = multiprocessing.Manager().list(self.positions)
        p = multiprocessing.Process(target=self.read_positions, args=(return_vector,))
        p.start()
        p.join(1)
        if p.is_alive():
            p.kill()
            print("Timeout")
            return False
        if return_vector[0] is None:
            return False
        for i, j in zip(self.positions, return_vector):
            if i != -1 and not self.game.M[i][j]:
                print("Illegal move")
                return False
        self.positions = return_vector
        return True


class Game:
    def __init__(self, cops_prog, robbers_prog, filename, graphic):
        try:
            with open(filename) as inst:
                cops = int(*re.findall(r"\d+", inst.readline()))
                robbers = int(*re.findall(r"\d+", inst.readline()))
                self.max_turn = int(*re.findall(r"\d+", inst.readline()))
                vertices = int(*re.findall(r"\d+", inst.readline()))
                self.layout = [
                    [float(i) for i in re.findall(r"-?\d+\.?\d*", inst.readline())]
                    for _ in range(vertices)
                ]
                edges = int(*re.findall(r"\d+", inst.readline()))
                self.M = [[False] * vertices for _ in range(vertices)]
                for i in range(vertices):
                    self.M[i][i] = True
                for _ in range(edges):
                    i, j = map(int, re.findall(r"\d+", inst.readline()))
                    self.M[i][j] = True
                    self.M[j][i] = True
        except Exception as e:
            print(f"Error while parsing board file: {e}")
            exit(1)
        try:
            p_cops = Popen(
                [cops_prog, filename, "0"], stdout=PIPE, stdin=PIPE, stderr=DEVNULL
            )
        except Exception as e:
            print(f"Error with cops program: {e}")
            exit(1)
        try:
            p_robbers = Popen(
                [robbers_prog, filename, "1"], stdout=PIPE, stdin=PIPE, stderr=DEVNULL
            )
        except Exception as e:
            print(f"Error with robbers program: {e}")
            exit(1)
        self.cops = Positions(cops, p_cops.stdout, p_robbers.stdin, self)
        self.robbers = Positions(robbers, p_robbers.stdout, p_cops.stdin, self)
        self.remaining_turn = self.max_turn + 2
        self.cops_turn = True
        self.graphic = graphic == "1"
        if self.graphic:
            self.init_screen()

    def init_screen(self):
        self.screen = Screen()
        self.screen.tracer(0)
        self.screen.setworldcoordinates(-1.1, -1.1, 1.1, 1.1)
        # Print edges
        edges = Turtle()
        edges.hideturtle()
        edges.width(3)
        edges.penup()
        for i in range(len(self.M)):
            for j in range(len(self.M[i])):
                if self.M[i][j]:
                    edges.setposition(self.layout[i])
                    edges.pendown()
                    edges.setposition(self.layout[j])
                    edges.penup()
        # Print vertices
        for i in range(len(self.layout)):
            vertex = Turtle()
            vertex.shape("circle")
            vertex.resizemode("user")
            if len(self.layout) < 50:
                vertex.shapesize(3, 3)
            elif len(self.layout) < 150:
                vertex.shapesize(2, 2)
            else:
                vertex.shapesize(1, 1)
            vertex.fillcolor("white")
            vertex.penup()
            vertex.setposition(self.layout[i])
        # Prepare counter
        self.counter = Turtle()
        self.counter.hideturtle()
        self.counter.penup()
        self.counter.setposition(-0.95, 0.95)
        # Prepare cops
        self.screen.tracer(1)

    # Start iteration
    def print_start_iteration(self):
        self.pos = g.cops if g.cops_turn else g.robbers
        self.role = "cops" if g.cops_turn else "robbers"
        if g.remaining_turn > g.max_turn:
            msg = f"Initial positions for {self.role}"
        else:
            msg = f"Turn for {self.role} (remaining: " + str(g.remaining_turn) + ")"
        print(msg)
        # Update graphic counter
        if self.graphic:
            self.counter.clear()
            self.counter.write(msg, move=False, font=("Arial", 16, "normal"))

    # New positions
    def new_positions(self):
        # Read new positions from one program
        if not self.pos.new_positions():
            print(f"No new positions for {self.role}: DISQUALIFIED")
            if not g.cops_turn:
                g.robbers.positions = []
            return False
        return True

    def print_new_positions(self):
        print(self.pos.positions)
        if self.graphic:
            # Create cops and robbers turtles
            if self.pos.turtles is None:
                self.screen.tracer(0)
                # Prepare turtles
                self.pos.turtles = []
                for p in self.pos.positions:
                    t = Turtle()
                    t.penup()
                    if self.cops_turn:
                        t.color("orange")
                        t.shape("triangle")
                    else:
                        t.color("red")
                        t.shape("circle")
                    t.width(5)
                    t.setposition(self.layout[p])
                    self.pos.turtles.append(t)
                self.screen.tracer(1)
            else:
                for p, t in zip(self.pos.positions, self.pos.turtles):
                    t.setposition(self.layout[p])

    # Send positions to other program
    def transmit_new_positions(self):
        try:
            for p in self.pos.positions:
                self.pos.stdin.write(bytearray(str(p) + "\n", "utf8"))
            self.pos.stdin.flush()
        except:
            pass

    # Update turns and remove captured robbers
    def end_iteration(self):
        self.robbers.remove_captured()
        self.remaining_turn -= 1
        self.cops_turn = not self.cops_turn

    # End game result
    def end_game(self):
        if len(g.robbers.positions) == 0:
            msg = "Cops win!"
        else:
            msg = "Robbers win!"
        print(msg)
        # Update graphic counter
        if self.graphic:
            self.counter.clear()
            self.counter.write(msg, move=False, font=("Arial", 16, "normal"))
            g.screen.exitonclick()


if __name__ == "__main__":
    if len(sys.argv) != 5:
        print("Usage: python ./server.py cops robbers filename 0/1")
        exit(1)

    g = Game(sys.argv[1], sys.argv[2], sys.argv[3], sys.argv[4])

    # Main loop
    while len(g.robbers.positions) != 0 and g.remaining_turn != 0:
        g.print_start_iteration()
        if not g.new_positions():
            break
        g.print_new_positions()
        g.transmit_new_positions()
        g.end_iteration()

    g.end_game()
