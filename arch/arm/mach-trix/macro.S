
#ifndef END
#define END(symbol)
#	.end symbol
#endif

#ifndef ENTRY
#define ALIGN	.align 4,0x90
#define ENTRY(name) \
  .globl name; \
name:
#endif

#ifndef ENDPROC
#define ENDPROC(name) \
  END(name)
#endif

