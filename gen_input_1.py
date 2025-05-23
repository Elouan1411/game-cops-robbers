import math


def print_input(filename, positions, edges):
    ROBBERS = 3
    with open(filename, "w") as f:
        f.write(f"Cops: 3\n")
        f.write(f"Robbers: {ROBBERS}\n")
        f.write(f"Max turn: {2 * ROBBERS * int(math.sqrt(len(positions)))}\n")
        f.write(f"Vertices: {len(positions)}\n")
        for x, y in positions:
            f.write(f"{x:.3f} {y:.3f}\n")
        f.write(f"Edges: {len(edges)}\n")
        for i, j in edges:
            f.write(f"{i} {j}\n")


def gen_indep(vertex):
    positions = []
    t = 0
    while t < 2 * math.pi:
        positions.append((math.cos(t), math.sin(t)))
        t += 2 * math.pi / vertex

    edges = []
    return positions, edges


def gen_circle(vertex):
    positions = []
    t = 0
    while t < 2 * math.pi:
        positions.append((math.cos(t), math.sin(t)))
        t += 2 * math.pi / vertex

    edges = []
    for i in range(vertex):
        edges.append((i, (i + 1) % vertex))

    return positions, edges


def gen_circular_chain(vertex):
    half = vertex // 2
    positions = []
    t1 = 0
    while t1 < 2 * math.pi:
        positions.append((math.cos(t1), math.sin(t1)))
        t1 += 2 * math.pi / half
    t2 = math.pi / half
    while t2 < 2 * math.pi:
        positions.append((0.8 * math.cos(t2), 0.8 * math.sin(t2)))
        t2 += 2 * math.pi / half

    edges = []
    for i in range(half):
        edges.append((i, (i + 1) % half))
        edges.append((i, half + (i - 1) % half))
        edges.append((i, half + i % half))
        edges.append((half + i, half + (i + 1) % half))

    return positions, edges


def gen_hexa(left, dist, existing, edges):
    height = round(0.9 * dist / 2 * math.sin(math.pi / 3))
    p1 = (left[0] + round(dist / 4), left[1] + height)
    p2 = (left[0] + round(3 * dist / 4), left[1] + height)
    p3 = (left[0] + dist, left[1])
    p4 = (left[0] + round(3 * dist / 4), left[1] - height)
    p5 = (left[0] + round(dist / 4), left[1] - height)
    index = []
    for p in [left, p1, p2, p3, p4, p5]:
        if p not in existing:
            existing.append(p)
        index.append(existing.index(p))
    for i in range(len(index)):
        edges.append((index[i], index[(i + 1) % 6]))
    ee = set(tuple(sorted(i)) for i in edges)
    edges.clear()
    edges.extend(ee)
    return p2, p4


def gen_hexa_order(order):
    RESOLUTION = 1000
    positions = []
    edges = []
    start = [(-1 * RESOLUTION, 0)]
    for _ in range(order):
        next = set()
        while len(start) != 0:
            p2, p4 = gen_hexa(
                start.pop(),
                round(RESOLUTION * 2 / (0.25 + 3 * order / 4)),
                positions,
                edges,
            )
            next.add(p2)
            next.add(p4)
        start = next
    for i in range(len(positions)):
        positions[i] = (positions[i][0] / RESOLUTION, positions[i][1] / RESOLUTION)
    positions.append((0, 0.9))
    positions.append((0, -0.9))
    NW = len(positions) - 2
    SW = len(positions) - 1
    edges.append((1, NW))
    edges.append((5, SW))
    highest = 0
    lowest = 0
    for i, (x, y) in enumerate(positions):
        if (
            y > positions[highest][1]
            or x < positions[highest][0]
            and y == positions[highest][1]
        ):
            highest = i
        if (
            y < positions[lowest][1]
            or x < positions[lowest][0]
            and y == positions[lowest][1]
        ):
            lowest = i
    edges.append((highest, NW))
    edges.append((lowest, SW))
    return positions, edges


print_input("indep1.txt", *gen_indep(3))
print_input("indep3.txt", *gen_indep(3))
print_input("indep4.txt", *gen_indep(4))
print_input("indep20.txt", *gen_indep(20))
print_input("circle20.txt", *gen_circle(20))
print_input("circular20.txt", *gen_circular_chain(20))
print_input("circular50.txt", *gen_circular_chain(50))
print_input("hexa3.txt", *gen_hexa_order(3))
print_input("hexa10.txt", *gen_hexa_order(10))
print_input("hexa20.txt", *gen_hexa_order(20))
