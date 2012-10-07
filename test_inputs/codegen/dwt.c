for (int c0 = 0; c0 < Ncl; c0 += 1)
  if (c0 >= 1 && Ncl >= c0 + 2) {
    S(c0, 28);
  } else if (c0 == 0) {
    S(0, 26);
  } else
    S(Ncl - 1, 27);
