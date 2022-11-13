.section text
   # First, load in the stack pointer, then set the stack pointer to that value.
   lui   C00`16
   lac   a0
   setsp a0
   saci  0
   addi  C`16     # Add in the lower part of the address
   lac   a0       # Load address of input into a0
   lm    a0, a0   # Load the value at address a0 into a0, or load the input
   jal   relPrime # Run relPrime
   addi  0
   jal   End      # After running the program, jump over it

gcd: # 14
   sac   a0
   bnez  gcd_Loop # 21 - 15 = 6
   addi  0
   sac   a1
   jalr  ra
   lac   a0

gcd_Loop: # 21
   sac   a1
   bez   gcd_End # 40 - 22 = 18
   addi  0
   slt   a0
   bez   gcd_DecB # 34 - 26 = 8
   sac   a0
   sub   a1
   bnez  gcd_Loop # 21 - 30 = -9
   swp   a0, zro

gcd_DecB: # 34
   sac   a1
   sub   a0
   bnez  gcd_Loop # 21 - 36 = -15
   swp   a1, zro

gcd_End: # 40
   jalr  ra
   addi  0

relPrime: # 42
   push  ra
   push  s0
   push  s1
   saci  2
   swp   s1, a0
   lac   s0

relPrime_Loop: # 52
   sac   s1
   swp   a1, s0
   lac   a0
   jal   gcd # 14 - 56 = -42
   sac   a0
   addi -1
   bez   relPrime_End # 66 - 60 = 6
   sac   s1
   addi  1
   jal   relPrime_Loop # 52 - 64 = -12
   lac   s1

relPrime_End: # 66
   pop   s1
   pop   s0
   pop   ra
   addi  0
   addi  0
   jalr  ra
   lac   a0

End: # 74
   saci  0
   addi -2
   lac   s0
   sm    s0, a0