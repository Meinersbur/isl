if (P2 >= 0 && P2 <= 3 && P1 == P2)
  for (int c0 = 0, c0_end = min(2, -P2 + 4); c0 <= c0_end; c0 += 1)
    for (int c2 = (-P2 - c0 + 6) % 3, c2_end = 3; c2 <= c2_end; c2 += 3)
      s0(c0, c0, c2, c2);
