.section text
   # Initialize the stack pointer
   lui   C00`16   # 00
   lac   a0       # 02
   setsp a0       # 03
   
   lui   FF8`16   # 05  Load the address of input
   lac   a0       # 07  Load address of input into a0
   lm    a0, a0   # 08  Load the value at address a0 into a0, or load the input
   jal   relPrime # 0A  Run relPrime
   addi  0        # 0C
   jal   End      # 0D  After running the program, jump over it

gcd:
   sac   a0       # 0F
   bnez  gcd_Loop # 10
   addi  0        # 12
   sac   a1       # 13
   jalr  ra       # 14
   lac   a0       # 15

gcd_Loop:
   sac   a1       # 16
   bez   gcd_End  # 17
   addi  0        # 19
   slt   a0       # 1A
   bez   gcd_DecB # 1B
   sac   a0       # 1D
   sub   a1       # 1E
   bnez  gcd_Loop # 1F
   swp   a0, zro  # 21

gcd_DecB:
   sac   a1       # 23
   sub   a0       # 24
   bnez  gcd_Loop # 25
   swp   a1, zro  # 27

gcd_End:
   jalr  ra       # 29
   addi  0        # 2A

relPrime:
   push  ra       # 2B
   push  s0       # 2D
   push  s1       # 2F
   saci  2        # 31
   swp   s1, a0   # 32
   lac   s0       # 34

relPrime_Loop:
   sac   s1       # 35
   swp   a1, s0   # 36
   lac   a0       # 38
   jal   gcd      # 39
   sac   a0       # 3B
   addi -1        # 3C
   bez   relPrime_End # 3D
   sac   s1       # 3F
   addi  1        # 40
   jal   relPrime_Loop # 41
   lac   s1       # 43

relPrime_End:
   pop   s1       # 44
   pop   s0       # 46
   pop   ra       # 48
   jalr  ra       # 4A
   lac   a0       # 4B

End:
   saci  0        # 4C
   addi -4        # 4D
   lac   s0       # 4E
   sm    s0, a0   # 4F
   
   jal   0        # 51

# .include "handlers.s"