for (int c0 = 0, c0_end = Ncl; c0 < c0_end; c0 += 1) {
  if (Ncl >= c0 + 2 && c0 >= 1) {
    S(c0, 28);
  } else if (c0 == 0) {
    S(0, 26);
  } else
    S(Ncl - 1, 27);
}
