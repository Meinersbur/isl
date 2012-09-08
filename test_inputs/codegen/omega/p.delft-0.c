if (P1 == P2 && P2 <= 3 && P2 >= 0)
  for (int c0 = 0; c0 <= min(2, -P2 + 4); c0 += 1)
    for (int c2 = -P2 - c0 + 3 * floord(P2 + c0 - 1, 3) + 3; c2 <= 3; c2 += 3)
      s0(c0, c0, c2, c2);
