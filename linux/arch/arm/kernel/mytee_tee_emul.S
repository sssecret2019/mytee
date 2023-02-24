#include <linux/linkage.h>

ENTRY(mytee_tee_mem_emul)
	smc #2
	ret
ENDPROC(mytee_tee_mem_emul)
