This is yet another assembler to intermediate language translator. It is
roughly similar to [Dagger](http://dagger.repzret.org/) or
[McSema](https://github.com/trailofbits/mcsema). 

It leverages Qemu's internal intermediate language (TCG), which is then
translated to LLVM intermediate representation (IR). Unfortunately TCG is not a
sufficient representation of all assembler instructions for an architecture, as
more complex assembler commands are usually implemented as C helper functions.
For this reason, we use the neat trick of compiling those helper functions to
LLVM IR and then linking them to the generated code at run time (which I think
was first done by Vitaly Chipounov for [S2E](http://s2e.epfl.ch/)).  The
advantage of using Qemu to translate assembler to LLVM IR is first that a very
wide range of architectures are supported (aarch64, alpha, arm, cris, i386,
m68k, microblaze, mips, or32, ppc, s390x, sh4, sparc32plus, sparc64, sparc,
tilegx, unicore32, x86_64). Second, the translations are quite well-tested for
correctness, as the intermediate code is usually executed.
	
Of course there is still a lot missing in the current implementation. Right now,
there is not binary image parser which loads an executable from disk, and there
is no disassembler discovering assembler instructions in the binary. An awesome
idea would be to pair this translator with 
[fREedom](https://github.com/cseagle/fREedom) and use the generated LLVM IR to
resolve direct and indirect code references (e.g., with some constraint solving
or symbolic execution). Further, the LLVM IR could be translated to REIL and
fed into Binnavi to make it understand other processor architectures.

If you want to know how to use the library right now, have a look at the
[libqemu-test](https://github.com/zaddach/libqemu-test) repository. Memory
accesses (also to code memory) are handled through callbacks, which is a bit
awkward, but close Qemu's original working. In the near future, the function
to translate assembler instructions or basic blocks will also be exposed in
python through [llvmlite](https://github.com/numba/llvmlite).
