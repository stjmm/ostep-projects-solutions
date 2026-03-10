# My solutions to OSTEP projects (stjmm)
This repo contains source code for completed and uncompleted (must pass all testt) [ostep-projects](https://github.com/remzi-arpacidusseau/ostep-projects) (I plan to do them all)

### Completed:

- **Initial Projects**
    - [initial-utilities](./initial-utilities/): solutions in wcat.c, wgrep.c, wzip.c, wunzip.c
    - [initial-reverse](./initial-reverse/reverse.c): solution in reverse.c
    - [initial-kv](./initial-kv/kv.c): using hash table
- **Processes Shell**
    - [processes-shell](./processes-shell/wish.c)


## xv6 Projects

For OSTEP projects using xv6, you must use [x86 xv6](https://github.com/mit-pdos/xv6-public).  

> **Hint:** If it does not run on a modern Linux machine, use an emulator with an older distro (I used Ubuntu 16.04 and it works for me).

All xv6 projects are provided as `.patch` files.  

To run a project:

1. Clone `xv6-public` into a `src` directory inside the project folder.
2. Apply the patch:

```bash
cd src
patch -p1 < ../patch.patch
```

- **xv6 projects**
    - [initial-xv6](./initial-xv6/getreadcount.patch)
    - [initial-xv6-tracer](./initial-xv6-tracer/trace.patch)

