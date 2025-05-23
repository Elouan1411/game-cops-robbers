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


def gen_triangle(left, dist, existing, edges):
    height = round(0.9 * dist / 2 * math.sin(math.pi / 3))
    p1 = (left[0] + round(dist / 4), left[1] + height)
    p2 = (left[0] + round(3 * dist / 4), left[1] + height)
    p3 = (left[0] + dist, left[1])
    p4 = (left[0] + round(3 * dist / 4), left[1] - height)
    p5 = (left[0] + round(dist / 4), left[1] - height)
    pc = (left[0] + dist / 2, left[1])
    index = []
    for p in [left, p1, p2, p3, p4, p5]:
        if p not in existing:
            existing.append(p)
        index.append(existing.index(p))
    existing.append(pc)
    for i in range(len(index)):
        edges.append((index[i], index[(i + 1) % 6]))
        edges.append((index[i], existing.index(pc)))
    ee = set(tuple(sorted(i)) for i in edges)
    edges.clear()
    edges.extend(ee)
    return p2, p4


def gen_triangle_order(order):
    RESOLUTION = 1000
    positions = []
    edges = []
    start = [(-1 * RESOLUTION, 0)]
    for _ in range(order):
        next = set()
        while len(start) != 0:
            p2, p4 = gen_triangle(
                start.pop(),
                round((RESOLUTION * 2 / (0.25 + 3 * order / 4)) * 0.9),
                positions,
                edges,
            )
            next.add(p2)
            next.add(p4)
        start = next
    for i in range(len(positions)):
        positions[i] = (positions[i][0] / RESOLUTION, positions[i][1] / RESOLUTION)
    positions.append((0, 0.8))
    positions.append((0, -0.8))
    NW = len(positions) - 2
    SW = len(positions) - 1
    edges.append((1, NW))
    edges.append((5, SW))
    highest = 0
    lowest = 0
    east_high = 0
    east_low = 0
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
        if (
            x > positions[east_high][0]
            or x == positions[east_high][0]
            and y > positions[east_high][1]
        ):
            east_high = i
        if (
            x > positions[east_low][0]
            or x == positions[east_low][0]
            and y < positions[east_low][1]
        ):
            east_low = i
    edges.append((highest, NW))
    edges.append((lowest, SW))
    positions.append((1, 0))
    E = len(positions) - 1
    edges.append((east_high, E))
    edges.append((east_low, E))
    return positions, edges


print_input("triangle3.txt", *gen_triangle_order(3))
print_input("triangle10.txt", *gen_triangle_order(10))
print_input("triangle15.txt", *gen_triangle_order(15))
