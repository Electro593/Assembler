gcd:
  sac  a0
  bnez gcd_Loop
  sac  a1
  lac  a0
  jalr ra
gcd_Loop:
  sac  a1
  bez  gcd_End
  slt  a0
  bez  gdc_DecB
  sac  a0
  sub  a1
  swp  a0, zro
  bez  gcd_Loop
gcd_DecB:
  sac  a1
  sub  a0
  swp  a1, zro
  bez  gcd_Loop
gdc_End:
  jalr ra


relPrime:
  push ra
  push s0
  push s1
  saci 2
  swp  s1, a0
  lac  s0
relPrime_Loop:
  sac  s1
  swp  a1, s0
  lac  a0
  jal  gcd
  addi -1
  bez  relPrime_End
  sac  s1
  addi 1
  lac  s1
  jal  relPrime_Loop
relPrime_End:
  sac  s1
  lac  a0
  pop  s1
  pop  s0
  pop  ra
  jalr ra

