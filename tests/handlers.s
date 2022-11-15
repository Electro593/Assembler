.section text

handle_io:
   push a0
   lac  a0
   push a0
   
   lsp 4
   lac a0
   
   
   
   
   
   
   
   handle_io_end:
   pop   a0
   sac   a0
   pop   a0
   addsp 2
   jalr  ra
   pop   ra